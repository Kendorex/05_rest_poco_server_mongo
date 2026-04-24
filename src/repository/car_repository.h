// car_repository.h
#pragma once

#include <Poco/Exception.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/MongoDB/Array.h>
#include <Poco/MongoDB/Document.h>
#include <Poco/MongoDB/Element.h>
#include <Poco/MongoDB/OpMsgCursor.h>
#include <Poco/MongoDB/OpMsgMessage.h>
#include <Poco/MongoDB/RegularExpression.h>
#include <Poco/Nullable.h>

#include "../db/database.h"
#include "../db/mongo_helpers.h"

#include <optional>
#include <string>
#include <vector>

namespace repository {

struct CarDocument {
    Poco::Int64 id{0};
    std::string brand;
    std::string model;
    std::string licensePlate;
    std::string carClass;
    bool available{true};
    double pricePerDay{0.0};
    int year{0};
    std::string color;
    std::vector<std::string> features;
    std::string location;
    int mileage{0};
    std::string status;
};

namespace {

using namespace Poco::MongoDB;

// Безопасное извлечение Int64 из документа
inline Poco::Int64 getInt64Safe(const Document::Ptr& doc, const std::string& field) {
    if (!doc->exists(field)) return 0;
    try {
        return doc->getInteger(field);
    } catch (...) {
        return 0;
    }
}

// Безопасное извлечение double
inline double getDoubleSafe(const Document::Ptr& doc, const std::string& field) {
    if (!doc->exists(field)) return 0.0;
    try {
        return doc->get<double>(field);
    } catch (...) {
        return 0.0;
    }
}

// Безопасное извлечение int
inline int getIntSafe(const Document::Ptr& doc, const std::string& field) {
    if (!doc->exists(field)) return 0;
    try {
        return doc->get<int>(field);
    } catch (...) {
        return 0;
    }
}

// Безопасное извлечение bool
inline bool getBoolSafe(const Document::Ptr& doc, const std::string& field) {
    if (!doc->exists(field)) return false;
    try {
        return doc->get<bool>(field);
    } catch (...) {
        return false;
    }
}

// Безопасное извлечение string
inline std::string getStringSafe(const Document::Ptr& doc, const std::string& field) {
    if (!doc->exists(field)) return "";
    try {
        return doc->get<std::string>(field);
    } catch (...) {
        return "";
    }
}

inline CarDocument carFromDoc(const Document::Ptr& doc) {
    CarDocument car;
    car.id = getInt64Safe(doc, "id");
    car.brand = getStringSafe(doc, "brand");
    car.model = getStringSafe(doc, "model");
    car.licensePlate = getStringSafe(doc, "licensePlate");
    car.carClass = getStringSafe(doc, "carClass");
    car.available = getBoolSafe(doc, "available");
    car.pricePerDay = getDoubleSafe(doc, "pricePerDay");
    car.year = getIntSafe(doc, "year");
    car.color = getStringSafe(doc, "color");
    car.location = getStringSafe(doc, "location");
    car.mileage = getIntSafe(doc, "mileage");
    car.status = getStringSafe(doc, "status");
    
    if (doc->exists("features") && !doc->isType<NullValue>("features")) {
        try {
            Array::Ptr featuresArr = doc->get<Array::Ptr>("features");
            for (std::size_t i = 0; i < featuresArr->size(); ++i) {
                car.features.push_back(featuresArr->get<std::string>(static_cast<int>(i)));
            }
        } catch (...) {
            // Игнорируем ошибки при чтении features
        }
    }
    
    return car;
}

} // namespace

class CarRepository {
public:
    Poco::JSON::Object::Ptr create(const Poco::JSON::Object::Ptr& payload) {
        CarDocument car = fromPayload(payload);
        
        if (existsByLicensePlate(car.licensePlate)) {
            throw Poco::InvalidArgumentException("License plate already exists");
        }
        
        const Poco::Int64 id = db::Database::instance().nextSequence("cars");
        car.id = id;
        
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("cars");
        req->setCommandName(OpMsgMessage::CMD_INSERT);
        
        Document::Ptr doc = new Document();
        doc->add("id", static_cast<Poco::Int64>(car.id));
        doc->add("brand", car.brand);
        doc->add("model", car.model);
        doc->add("licensePlate", car.licensePlate);
        doc->add("carClass", car.carClass);
        doc->add("available", car.available);
        doc->add("pricePerDay", car.pricePerDay);
        doc->add("year", car.year);
        doc->add("color", car.color);
        doc->add("location", car.location);
        doc->add("mileage", car.mileage);
        doc->add("status", car.status);
        
        Array::Ptr featuresArr = new Array();
        for (const auto& f : car.features) {
            featuresArr->add(f);
        }
        doc->add("features", featuresArr);
        
        req->documents().push_back(doc);
        
        OpMsgMessage resp;
        d.send(*req, resp);
        if (!resp.responseOk()) {
            throw Poco::RuntimeException("Car insert failed: " + resp.body().toString());
        }
        
        return toJson(car);
    }
    
    Poco::JSON::Array::Ptr listAvailable(const std::string& carClass = "") {
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        const std::string dbName = mdb.name();
        
        d.withLockedConnection([&](Poco::MongoDB::Connection& conn) {
            Poco::MongoDB::OpMsgCursor cursor(dbName, "cars");
            cursor.setBatchSize(1000);
            cursor.query().setCommandName(OpMsgMessage::CMD_FIND);
            Poco::MongoDB::Document& body = cursor.query().body();
            body.addNewDocument("sort").add("id", 1);
            
            Poco::MongoDB::Document& filter = body.addNewDocument("filter");
            filter.add("available", true);
            filter.add("status", "active");
            
            if (!carClass.empty()) {
                filter.add("carClass", carClass);
            }
            
            Poco::MongoDB::OpMsgMessage& cresp = cursor.next(conn);
            while (true) {
                db::mongoEnsureOk(cresp, "car list available");
                for (const auto& doc : cresp.documents()) {
                    result->add(toJson(carFromDoc(doc)));
                }
                if (cursor.cursorID() == 0) break;
                cresp = cursor.next(conn);
            }
        });
        return result;
    }
    
    std::optional<CarDocument> getById(Poco::Int64 id) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("cars");
        req->setCommandName(OpMsgMessage::CMD_FIND);
        req->body().add("limit", 1).addNewDocument("filter").add("id", id);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "car getById");
        if (resp.documents().empty()) return std::nullopt;
        return carFromDoc(resp.documents()[0]);
    }
    
    bool updateAvailability(Poco::Int64 id, bool available) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("cars");
        req->setCommandName(OpMsgMessage::CMD_UPDATE);
        Document::Ptr spec = new Document();
        spec->addNewDocument("q").add("id", id);
        Document& setDoc = spec->addNewDocument("u").addNewDocument("$set");
        setDoc.add("available", available);
        spec->add("multi", false);
        req->documents().push_back(spec);
        
        OpMsgMessage resp;
        d.send(*req, resp);
        if (!resp.responseOk()) {
            throw Poco::RuntimeException("Car availability update failed: " + resp.body().toString());
        }
        
        return resp.body().getInteger("n") > 0;
    }
    
    static Poco::JSON::Object::Ptr toJson(const CarDocument& car) {
        Poco::JSON::Object::Ptr out = new Poco::JSON::Object();
        out->set("id", car.id);
        out->set("brand", car.brand);
        out->set("model", car.model);
        out->set("licensePlate", car.licensePlate);
        out->set("class", car.carClass);
        out->set("available", car.available);
        out->set("pricePerDay", car.pricePerDay);
        out->set("year", car.year);
        out->set("color", car.color);
        out->set("location", car.location);
        out->set("mileage", car.mileage);
        out->set("status", car.status);
        
        Poco::JSON::Array::Ptr featuresArr = new Poco::JSON::Array();
        for (const auto& f : car.features) {
            featuresArr->add(f);
        }
        out->set("features", featuresArr);
        
        return out;
    }
    
private:
    CarDocument fromPayload(const Poco::JSON::Object::Ptr& payload) {
        CarDocument car;
        car.brand = payload->getValue<std::string>("brand");
        car.model = payload->getValue<std::string>("model");
        car.licensePlate = payload->getValue<std::string>("licensePlate");
        car.carClass = payload->getValue<std::string>("class");
        car.available = payload->optValue<bool>("available", true);
        car.pricePerDay = payload->getValue<double>("pricePerDay");
        car.year = payload->getValue<int>("year");
        car.color = payload->optValue<std::string>("color", "White");
        car.location = payload->optValue<std::string>("location", "Main Branch");
        car.mileage = payload->optValue<int>("mileage", 0);
        car.status = payload->optValue<std::string>("status", "active");
        
        if (payload->has("features") && payload->isArray("features")) {
            Poco::JSON::Array::Ptr featuresArr = payload->getArray("features");
            for (size_t i = 0; i < featuresArr->size(); ++i) {
                car.features.push_back(featuresArr->getElement<std::string>(i));
            }
        }
        
        return car;
    }
    
    bool existsByLicensePlate(const std::string& licensePlate) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("cars");
        req->setCommandName(OpMsgMessage::CMD_FIND);
        req->body().add("limit", 1).addNewDocument("filter").add("licensePlate", licensePlate);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "car check license plate");
        return !resp.documents().empty();
    }
};

} // namespace repository