#pragma once

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>

#include "auth_handler.h"
#include "car_handler.h"
#include "hello_world_handler.h"
#include "helloworld_jwt_handler.h"
#include "metrics_handler.h"
#include "not_found_handler.h"
#include "rental_handler.h"
#include "swagger_handler.h"
#include "user_handler.h"

namespace handlers {

class RouterFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request) override {
        const std::string uri = request.getURI();
        
        // Auth
        if (uri == "/api/v1/auth") {
            return new AuthHandler();
        }
        // Users
        if (uri == "/api/v1/users/register" || uri.find("/api/v1/users/search") == 0) {
            return new UserHandler();
        }
        // Cars
        if (uri == "/api/v1/cars" || uri.find("/api/v1/cars/search") == 0) {
            return new CarHandler();
        }
        // Rentals
        if (uri == "/api/v1/rentals" || 
            uri == "/api/v1/rentals/active" || 
            uri == "/api/v1/rentals/history" ||
            uri.find("/api/v1/rentals/") != std::string::npos) {
            return new RentalHandler();
        }
        // Metrics
        if (uri == "/metrics") {
            return new MetricsHandler();
        }
        // Swagger
        if (uri == "/swagger.yaml") {
            return new SwaggerHandler();
        }
        // Hello World endpoints
        if (uri == "/api/v1/helloworld") {
            return new HelloWorldHandler();
        }
        if (uri == "/api/v1/helloworld_jwt") {
            return new HelloWorldJwtHandler();
        }
        
        return new NotFoundHandler();
    }
};

} // namespace handlers