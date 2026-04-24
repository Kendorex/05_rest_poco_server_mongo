#pragma once

#include <Poco/Exception.h>
#include <Poco/JSON/Object.h>
#include <Poco/Logger.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Timestamp.h>
#include <Poco/URI.h>

#include "../repository/car_repository.h"
#include "request_counter.h"
#include "rest_utils.h"

namespace handlers {

class CarHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response) override {
        Poco::Timestamp start;
        if (g_httpRequests) g_httpRequests->inc();
        auto& logger = Poco::Logger::get("Server");
        
        try {
            repository::CarRepository repository;
            const std::string uri = request.getURI();
            const std::string method = request.getMethod();

            if (method == "GET" && (uri == "/api/v1/cars" || uri == "/api/v1/cars/")) {
                auto result = repository.listAvailable();
                sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, result);
            }
            else if (method == "GET" && uri.find("/api/v1/cars/search") != std::string::npos) {
                Poco::URI uriObj(request.getURI());
                auto params = uriObj.getQueryParameters();
                
                std::string carClass, brand, model;
                
                for (const auto& p : params) {
                    if (p.first == "class") {
                        carClass = p.second;
                    } else if (p.first == "brand") {
                        brand = p.second;
                    } else if (p.first == "model") {
                        model = p.second;
                    }
                }
                
                auto result = repository.listAvailable(carClass);
                sendJson(response, Poco::Net::HTTPResponse::HTTP_OK, result);
            }
            else if (method == "POST" && uri == "/api/v1/cars") {
                auto payload = parseJsonBody(request);
                auto result = repository.create(payload);
                sendJson(response, Poco::Net::HTTPResponse::HTTP_CREATED, result);
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