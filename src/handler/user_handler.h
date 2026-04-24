#pragma once

#include <Poco/Exception.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/Logger.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Timestamp.h>
#include <Poco/URI.h>

#include "../repository/user_repository.h"
#include "request_counter.h"
#include "rest_utils.h"

namespace handlers {

class UserHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override {
        Poco::Timestamp start;
        if (g_httpRequests) g_httpRequests->inc();
        auto& logger = Poco::Logger::get("Server");

        try {
            repository::UserRepository repository;
            const std::string uri = request.getURI();
            const std::string method = request.getMethod();

            // POST /api/v1/users/register
            if (method == "POST" && uri == "/api/v1/users/register") {
                auto payload = parseJsonBody(request);
                auto result = repository.create(payload);
                sendJson(response, Poco::Net::HTTPResponse::HTTP_CREATED, result);
            }
            // GET /api/v1/users/search
            else if (method == "GET" && uri.find("/api/v1/users/search") != std::string::npos) {
                Poco::URI uriObj(request.getURI());
                auto params = uriObj.getQueryParameters();
                
                std::string login, firstName, lastName;
                
                // Парсим все параметры
                for (const auto& p : params) {
                    if (p.first == "login" || p.first == "username") {
                        login = p.second;
                    }
                    else if (p.first == "firstName" || p.first == "firstname" || p.first == "name") {
                        firstName = p.second;
                    }
                    else if (p.first == "lastName" || p.first == "lastname" || p.first == "surname") {
                        lastName = p.second;
                    }
                }
                
                if (!login.empty()) {
                    // Поиск по логину
                    auto user = repository.getByUsername(login);
                    if (!user.has_value()) {
                        Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                        err->set("error", "user not found");
                        sendJson(response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND, err);
                        if (g_httpErrors) g_httpErrors->inc();
                    } else {
                        Poco::JSON::Array::Ptr arr = new Poco::JSON::Array();
                        arr->add(repository::UserRepository::toJson(*user));
                        sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, arr);
                    }
                } 
                else if (!firstName.empty() || !lastName.empty()) {
                    // Поиск по маске имени/фамилии
                    auto result = repository.searchByNameMask(firstName, lastName);
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, result);
                }
                else {
                    // Если нет параметров - возвращаем всех пользователей
                    auto result = repository.list();
                    sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, result);
                }
            }
            else {
                Poco::JSON::Object::Ptr err = new Poco::JSON::Object();
                err->set("error", "endpoint not found");
                err->set("uri", uri);
                err->set("method", method);
                sendJson(response, Poco::Net::HTTPResponse::HTTP_NOT_FOUND, err);
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
};

} // namespace handlers