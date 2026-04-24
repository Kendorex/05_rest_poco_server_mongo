#pragma once

#include <Poco/Exception.h>
#include <Poco/JSON/Object.h>
#include <Poco/Logger.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Timestamp.h>

#include "../repository/rental_repository.h"
#include "request_counter.h"
#include "rest_utils.h"

namespace handlers {

class RentalHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override {
        Poco::Timestamp start;
        if (g_httpRequests) g_httpRequests->inc();
        auto& logger = Poco::Logger::get("Server");
        
        try {
            repository::RentalRepository repository;
            const std::string uri = request.getURI();
            const std::string method = request.getMethod();
            
            // Извлекаем user_id из заголовка или параметра (в реальном проекте из JWT)
            Poco::Int64 currentUserId = getCurrentUserId(request);
            
            // POST /api/v1/rentals - создание новой аренды
            if (method == "POST" && uri == "/api/v1/rentals") {
                auto payload = parseJsonBody(request);
                payload->set("userId", currentUserId); // ID текущего пользователя
                sendJson(response, Poco::Net::HTTPResponse::HTTP_CREATED, repository.create(payload));
            }
            // GET /api/v1/rentals/active - активные аренды текущего пользователя
            else if (method == "GET" && uri == "/api/v1/rentals/active") {
                sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, repository.getByUserId(currentUserId, true));
            }
            // GET /api/v1/rentals/history - история аренд текущего пользователя
            else if (method == "GET" && uri == "/api/v1/rentals/history") {
                sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, repository.getByUserId(currentUserId, false));
            }
            // PUT /api/v1/rentals/{id}/complete - досрочное завершение
            else if (method == "PUT" && uri.find("/complete") != std::string::npos) {
                auto id = extractIdFromUri(uri, "/api/v1/rentals/");
                if (id.has_value()) {
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, repository.completeRental(id.value(), true));
                } else {
                    Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                    err->set("error", "rental id required");
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST, err);
                    if (g_httpErrors) g_httpErrors->inc();
                }
            }
            else {
                Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                err->set("error", "method not allowed");
                sendJson(response, Poco::Net::HTTPResponse::HTTP_METHOD_NOT_ALLOWED, err);
                if (g_httpErrors) g_httpErrors->inc();
            }
        } catch (const Poco::Exception& e) {
            Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
            err->set("error", e.displayText());
            sendJson(response, Poco::Net::HTTPResponse::HTTP_BAD_REQUEST, err);
            if (g_httpErrors) g_httpErrors->inc();
        } catch (const std::exception& e) {
            Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
            err->set("error", e.what());
            sendJson(response, Poco::Net::HTTPResponse::HTTP_INTERNAL_SERVER_ERROR, err);
            if (g_httpErrors) g_httpErrors->inc();
        }
        
        Poco::Timespan elapsed = Poco::Timestamp() - start;
        double seconds = static_cast<double>(elapsed.totalMicroseconds()) / 1000000.0;
        if (g_httpDuration) g_httpDuration->observe(seconds);
        logger.information("%d %s %s from %s, %.2f ms",
                           static_cast<int>(response.getStatus()),
                           request.getMethod(),
                           request.getURI(),
                           request.clientAddress().toString(),
                           seconds * 1000.0);
    }
    
private:
    Poco::Int64 getCurrentUserId(Poco::Net::HTTPServerRequest& request) {
        // В реальном проекте извлекать user_id из JWT токена
        // Для демо используем заголовок X-User-Id или параметр
        std::string userIdStr = getQueryParam(request, "user_id");
        if (userIdStr.empty()) {
            // Временно: для тестов используем user_id=1
            return 1;
        }
        return std::stoll(userIdStr);
    }
    
    std::optional<Poco::Int64> extractIdFromUri(const std::string& uri, const std::string& prefix) {
        std::string remaining = uri.substr(prefix.length());
        size_t slashPos = remaining.find('/');
        if (slashPos != std::string::npos) {
            remaining = remaining.substr(0, slashPos);
        }
        try {
            return std::stoll(remaining);
        } catch (...) {
            return std::nullopt;
        }
    }
};

} // namespace handlers