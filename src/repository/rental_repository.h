#pragma once

#include <Poco/Exception.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/MongoDB/Array.h>
#include <Poco/MongoDB/Document.h>
#include <Poco/MongoDB/Element.h>
#include <Poco/MongoDB/OpMsgCursor.h>
#include <Poco/MongoDB/OpMsgMessage.h>
#include <Poco/Timestamp.h>

#include "../db/database.h"
#include "../db/mongo_helpers.h"
#include "car_repository.h"

#include <optional>
#include <string>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace repository {

struct RentalDocument {
    Poco::Int64 id{0};
    Poco::Int64 userId{0};
    Poco::Int64 carId{0};
    std::string startDate;
    std::string endDate;
    std::string status;
    double totalPrice{0.0};
    std::string paymentId;
    std::string pickupLocation;
    std::string returnLocation;
    bool insurance{false};
    std::vector<Poco::Int64> additionalDrivers;
    std::string createdAt;
};

namespace {

using namespace Poco::MongoDB;

} // namespace

class RentalRepository {
public:
    Poco::JSON::Object::Ptr create(const Poco::JSON::Object::Ptr& payload) {
        RentalDocument rental = fromPayload(payload);
        
        CarRepository carRepo;
        auto car = carRepo.getById(rental.carId);
        if (!car.has_value()) {
            throw Poco::InvalidArgumentException("Car not found");
        }
        if (!car->available || car->status != "active") {
            throw Poco::InvalidArgumentException("Car is not available for rental");
        }
        
        // Calculate days between dates
        int days = calculateDays(rental.startDate, rental.endDate);
        rental.totalPrice = days * car->pricePerDay;
        if (rental.insurance) {
            rental.totalPrice += days * 15.0;
        }
        
        const Poco::Int64 id = db::Database::instance().nextSequence("rentals");
        rental.id = id;
        rental.status = "active";
        
        // Get current timestamp as string
        auto now = std::time(nullptr);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&now), "%Y-%m-%dT%H:%M:%SZ");
        rental.createdAt = ss.str();
        
        carRepo.updateAvailability(rental.carId, false);
        
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("rentals");
        req->setCommandName(OpMsgMessage::CMD_INSERT);
        
        Document::Ptr doc = new Document();
        doc->add("id", id);
        doc->add("userId", rental.userId);
        doc->add("carId", rental.carId);
        doc->add("startDate", rental.startDate);
        doc->add("endDate", rental.endDate);
        doc->add("status", rental.status);
        doc->add("totalPrice", rental.totalPrice);
        doc->add("paymentId", rental.paymentId);
        doc->add("pickupLocation", rental.pickupLocation);
        doc->add("returnLocation", rental.returnLocation);
        doc->add("insurance", rental.insurance);
        doc->add("createdAt", rental.createdAt);
        
        req->documents().push_back(doc);
        
        OpMsgMessage resp;
        d.send(*req, resp);
        if (!resp.responseOk()) {
            throw Poco::RuntimeException("Rental insert failed: " + resp.body().toString());
        }
        
        return toJson(rental);
    }
    
    Poco::JSON::Array::Ptr getByUserId(Poco::Int64 userId, bool activeOnly = false) {
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        const std::string dbName = mdb.name();
        
        d.withLockedConnection([&](Poco::MongoDB::Connection& conn) {
            Poco::MongoDB::OpMsgCursor cursor(dbName, "rentals");
            cursor.setBatchSize(1000);
            cursor.query().setCommandName(OpMsgMessage::CMD_FIND);
            Poco::MongoDB::Document& body = cursor.query().body();
            body.addNewDocument("sort").add("startDate", -1);
            
            Poco::MongoDB::Document& filter = body.addNewDocument("filter");
            filter.add("userId", userId);
            
            if (activeOnly) {
                filter.add("status", "active");
            }
            
            Poco::MongoDB::OpMsgMessage& cresp = cursor.next(conn);
            while (true) {
                db::mongoEnsureOk(cresp, "rental get by user");
                for (const auto& doc : cresp.documents()) {
                    RentalDocument rental;
                    rental.id = doc->getInteger("id");
                    rental.userId = doc->getInteger("userId");
                    rental.carId = doc->getInteger("carId");
                    rental.startDate = doc->get<std::string>("startDate");
                    rental.endDate = doc->get<std::string>("endDate");
                    rental.status = doc->get<std::string>("status");
                    rental.totalPrice = doc->get<double>("totalPrice");
                    rental.paymentId = doc->get<std::string>("paymentId");
                    rental.pickupLocation = doc->get<std::string>("pickupLocation");
                    rental.returnLocation = doc->get<std::string>("returnLocation");
                    rental.insurance = doc->get<bool>("insurance");
                    rental.createdAt = doc->get<std::string>("createdAt");
                    
                    result->add(toJson(rental));
                }
                if (cursor.cursorID() == 0) break;
                cresp = cursor.next(conn);
            }
        });
        return result;
    }
    
    Poco::JSON::Object::Ptr completeRental(Poco::Int64 id, bool early = false) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        
        std::string newStatus = early ? "early_completed" : "completed";
        
        auto now = std::time(nullptr);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&now), "%Y-%m-%dT%H:%M:%SZ");
        
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("rentals");
        req->setCommandName(OpMsgMessage::CMD_UPDATE);
        Document::Ptr spec = new Document();
        spec->addNewDocument("q").add("id", id);
        Document& setDoc = spec->addNewDocument("u").addNewDocument("$set");
        setDoc.add("status", newStatus);
        setDoc.add("actualEndDate", ss.str());
        spec->add("multi", false);
        req->documents().push_back(spec);
        
        OpMsgMessage resp;
        d.send(*req, resp);
        if (!resp.responseOk()) {
            throw Poco::RuntimeException("Rental complete failed: " + resp.body().toString());
        }
        
        // Free the car
        auto rental = getById(id);
        if (rental.has_value()) {
            CarRepository carRepo;
            carRepo.updateAvailability(rental->carId, true);
        }
        
        Poco::JSON::Object::Ptr result = new Poco::JSON::Object();
        result->set("id", id);
        result->set("status", newStatus);
        result->set("message", "Rental completed successfully");
        
        return result;
    }
    
    Poco::JSON::Object::Ptr cancelRental(Poco::Int64 id) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("rentals");
        req->setCommandName(OpMsgMessage::CMD_UPDATE);
        Document::Ptr spec = new Document();
        spec->addNewDocument("q").add("id", id);
        Document& setDoc = spec->addNewDocument("u").addNewDocument("$set");
        setDoc.add("status", "cancelled");
        spec->add("multi", false);
        req->documents().push_back(spec);
        
        OpMsgMessage resp;
        d.send(*req, resp);
        if (!resp.responseOk()) {
            throw Poco::RuntimeException("Rental cancel failed: " + resp.body().toString());
        }
        
        // Free the car
        auto rental = getById(id);
        if (rental.has_value()) {
            CarRepository carRepo;
            carRepo.updateAvailability(rental->carId, true);
        }
        
        Poco::JSON::Object::Ptr result = new Poco::JSON::Object();
        result->set("id", id);
        result->set("status", "cancelled");
        result->set("message", "Rental cancelled successfully");
        
        return result;
    }
    
    std::optional<RentalDocument> getById(Poco::Int64 id) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("rentals");
        req->setCommandName(OpMsgMessage::CMD_FIND);
        req->body().add("limit", 1).addNewDocument("filter").add("id", id);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "rental getById");
        if (resp.documents().empty()) return std::nullopt;
        
        auto doc = resp.documents()[0];
        RentalDocument rental;
        rental.id = doc->getInteger("id");
        rental.userId = doc->getInteger("userId");
        rental.carId = doc->getInteger("carId");
        rental.startDate = doc->get<std::string>("startDate");
        rental.endDate = doc->get<std::string>("endDate");
        rental.status = doc->get<std::string>("status");
        rental.totalPrice = doc->get<double>("totalPrice");
        rental.paymentId = doc->get<std::string>("paymentId");
        rental.pickupLocation = doc->get<std::string>("pickupLocation");
        rental.returnLocation = doc->get<std::string>("returnLocation");
        rental.insurance = doc->get<bool>("insurance");
        rental.createdAt = doc->get<std::string>("createdAt");
        
        return rental;
    }
    
    static Poco::JSON::Object::Ptr toJson(const RentalDocument& rental) {
        Poco::JSON::Object::Ptr out = new Poco::JSON::Object();
        out->set("id", rental.id);
        out->set("userId", rental.userId);
        out->set("carId", rental.carId);
        out->set("startDate", rental.startDate);
        out->set("endDate", rental.endDate);
        out->set("status", rental.status);
        out->set("totalPrice", rental.totalPrice);
        out->set("paymentId", rental.paymentId);
        out->set("pickupLocation", rental.pickupLocation);
        out->set("returnLocation", rental.returnLocation);
        out->set("insurance", rental.insurance);
        
        return out;
    }
    
private:
    RentalDocument fromPayload(const Poco::JSON::Object::Ptr& payload) {
        RentalDocument rental;
        rental.userId = payload->getValue<Poco::Int64>("userId");
        rental.carId = payload->getValue<Poco::Int64>("carId");
        rental.startDate = payload->getValue<std::string>("startDate");
        rental.endDate = payload->getValue<std::string>("endDate");
        rental.paymentId = payload->optValue<std::string>("paymentId", "");
        rental.pickupLocation = payload->optValue<std::string>("pickupLocation", "Main Branch");
        rental.returnLocation = payload->optValue<std::string>("returnLocation", "Main Branch");
        rental.insurance = payload->optValue<bool>("insurance", false);
        
        return rental;
    }
    
    int calculateDays(const std::string& startStr, const std::string& endStr) {
        struct tm tm_start = {}, tm_end = {};
        strptime(startStr.c_str(), "%Y-%m-%dT%H:%M:%S", &tm_start);
        strptime(endStr.c_str(), "%Y-%m-%dT%H:%M:%S", &tm_end);
        
        time_t start = timegm(&tm_start);
        time_t end = timegm(&tm_end);
        
        return static_cast<int>((end - start) / (24 * 60 * 60));
    }
};

} // namespace repository