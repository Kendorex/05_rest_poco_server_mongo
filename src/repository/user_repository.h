// user_repository.h
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
#include <ctime>
#include <sstream>
#include <iomanip>

namespace repository {

struct User {
    Poco::Int64 id{0};
    std::string login;
    std::string passwordHash;
    std::string firstName;
    std::string lastName;
    std::string role;
    std::string email;
    std::string createdAt;
};

namespace {

using namespace Poco::MongoDB;

inline RegularExpression::Ptr regexI(const std::string& pattern) {
    return new RegularExpression(pattern, "i");
}

inline User userFromDoc(const Document::Ptr& doc) {
    User user;
    user.id = doc->getInteger("id");
    user.login = doc->get<std::string>("login");
    user.passwordHash = doc->get<std::string>("passwordHash");
    user.firstName = doc->get<std::string>("firstName");
    user.lastName = doc->get<std::string>("lastName");
    user.role = doc->get<std::string>("role");
    user.email = doc->get<std::string>("email");
    user.createdAt = doc->get<std::string>("createdAt");
    
    return user;
}

} // namespace

class UserRepository {
public:
    Poco::JSON::Object::Ptr create(const Poco::JSON::Object::Ptr& payload) {
        User user = fromPayload(payload);
        
        if (existsByUsername(user.login)) {
            throw Poco::InvalidArgumentException("Username already exists");
        }
        if (existsByEmail(user.email)) {
            throw Poco::InvalidArgumentException("Email already exists");
        }
        
        user.id = db::Database::instance().nextSequence("users");
        
        auto now = std::time(nullptr);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&now), "%Y-%m-%dT%H:%M:%SZ");
        user.createdAt = ss.str();
        
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        
        Document::Ptr doc = new Document();
        doc->add("id", user.id);
        doc->add("login", user.login);
        doc->add("passwordHash", user.passwordHash);
        doc->add("firstName", user.firstName);
        doc->add("lastName", user.lastName);
        doc->add("role", user.role);
        doc->add("email", user.email);
        doc->add("createdAt", user.createdAt);
        
        Poco::SharedPtr<OpMsgMessage> insertReq = mdb.createOpMsgMessage("users");
        insertReq->setCommandName(OpMsgMessage::CMD_INSERT);
        insertReq->documents().push_back(doc);
        
        OpMsgMessage resp;
        d.send(*insertReq, resp);
        
        if (!resp.responseOk()) {
            throw Poco::RuntimeException("User insert failed: " + resp.body().toString());
        }
        
        return toJson(user);
    }
    
    Poco::JSON::Array::Ptr list() {
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        const std::string dbName = mdb.name();
        
        d.withLockedConnection([&](Poco::MongoDB::Connection& conn) {
            Poco::MongoDB::OpMsgCursor cursor(dbName, "users");
            cursor.setBatchSize(1000);
            cursor.query().setCommandName(OpMsgMessage::CMD_FIND);
            Poco::MongoDB::Document& body = cursor.query().body();
            body.addNewDocument("sort").add("id", 1);
            body.addNewDocument("filter");
            
            Poco::MongoDB::OpMsgMessage& cresp = cursor.next(conn);
            while (true) {
                db::mongoEnsureOk(cresp, "user list");
                for (const auto& doc : cresp.documents()) {
                    result->add(toJson(userFromDoc(doc)));
                }
                if (cursor.cursorID() == 0) break;
                cresp = cursor.next(conn);
            }
        });
        return result;
    }
    
    std::optional<User> getByUsername(const std::string& login) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_FIND);
        req->body().add("limit", 1).addNewDocument("filter").add("login", login);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "user getByUsername");
        if (resp.documents().empty()) return std::nullopt;
        return userFromDoc(resp.documents()[0]);
    }
    
    Poco::JSON::Array::Ptr searchByNameMask(const std::string& firstNameMask, const std::string& lastNameMask) {
        Poco::JSON::Array::Ptr result = new Poco::JSON::Array();
        
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        const std::string dbName = mdb.name();
        
        d.withLockedConnection([&](Poco::MongoDB::Connection& conn) {
            Poco::MongoDB::OpMsgCursor cursor(dbName, "users");
            cursor.setBatchSize(1000);
            cursor.query().setCommandName(OpMsgMessage::CMD_FIND);
            Poco::MongoDB::Document& body = cursor.query().body();
            body.addNewDocument("sort").add("id", 1);
            
            Poco::MongoDB::Document& filter = body.addNewDocument("filter");
            
            bool hasFirst = !firstNameMask.empty();
            bool hasLast = !lastNameMask.empty();
            
            if (hasFirst && hasLast) {
                Poco::MongoDB::Array& andArr = filter.addNewArray("$and");
                Document::Ptr c1 = new Document();
                c1->add("firstName", regexI(firstNameMask));
                andArr.add(c1);
                Document::Ptr c2 = new Document();
                c2->add("lastName", regexI(lastNameMask));
                andArr.add(c2);
            } else if (hasFirst) {
                filter.add("firstName", regexI(firstNameMask));
            } else if (hasLast) {
                filter.add("lastName", regexI(lastNameMask));
            } else {
                return;
            }
            
            Poco::MongoDB::OpMsgMessage& cresp = cursor.next(conn);
            while (true) {
                db::mongoEnsureOk(cresp, "user search by name mask");
                for (const auto& doc : cresp.documents()) {
                    result->add(toJson(userFromDoc(doc)));
                }
                if (cursor.cursorID() == 0) break;
                cresp = cursor.next(conn);
            }
        });
        return result;
    }
    
    std::optional<User> getById(Poco::Int64 id) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_FIND);
        req->body().add("limit", 1).addNewDocument("filter").add("id", id);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "user getById");
        if (resp.documents().empty()) return std::nullopt;
        return userFromDoc(resp.documents()[0]);
    }
    
    std::optional<User> update(Poco::Int64 id, const Poco::JSON::Object::Ptr& payload) {
        User user = fromPayload(payload);
        
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_UPDATE);
        Document::Ptr spec = new Document();
        spec->addNewDocument("q").add("id", id);
        Document& setDoc = spec->addNewDocument("u").addNewDocument("$set");
        setDoc.add("login", user.login);
        setDoc.add("passwordHash", user.passwordHash);
        setDoc.add("firstName", user.firstName);
        setDoc.add("lastName", user.lastName);
        setDoc.add("role", user.role);
        setDoc.add("email", user.email);
        
        spec->add("multi", false);
        req->documents().push_back(spec);
        
        OpMsgMessage resp;
        d.send(*req, resp);
        if (!resp.responseOk()) {
            throw Poco::RuntimeException("User update failed: " + resp.body().toString());
        }
        
        const Poco::Int64 matched = resp.body().getInteger("n");
        if (matched == 0) return std::nullopt;
        return getById(id);
    }
    
    bool remove(Poco::Int64 id) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_DELETE);
        Document::Ptr del = new Document();
        del->add("limit", 1).addNewDocument("q").add("id", id);
        req->documents().push_back(del);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "user delete");
        return resp.body().getInteger("n") > 0;
    }
    
    static Poco::JSON::Object::Ptr toJson(const User& user) {
        Poco::JSON::Object::Ptr out = new Poco::JSON::Object();
        out->set("id", user.id);
        out->set("login", user.login);
        out->set("firstName", user.firstName);
        out->set("lastName", user.lastName);
        out->set("role", user.role);
        out->set("email", user.email);
        out->set("createdAt", user.createdAt);
        return out;
    }
    
private:
    User fromPayload(const Poco::JSON::Object::Ptr& payload) {
        User user;
        user.login = payload->getValue<std::string>("login");
        user.passwordHash = payload->getValue<std::string>("password");
        user.firstName = payload->getValue<std::string>("firstName");
        user.lastName = payload->getValue<std::string>("lastName");
        user.role = payload->optValue<std::string>("role", "customer");
        user.email = payload->getValue<std::string>("email");
        return user;
    }
    
    bool existsByUsername(const std::string& login) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_FIND);
        req->body().add("limit", 1).addNewDocument("filter").add("login", login);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "user check login");
        return !resp.documents().empty();
    }
    
    bool existsByEmail(const std::string& email) {
        db::Database& d = db::Database::instance();
        Poco::MongoDB::Database& mdb = d.mongoDb();
        Poco::SharedPtr<OpMsgMessage> req = mdb.createOpMsgMessage("users");
        req->setCommandName(OpMsgMessage::CMD_FIND);
        req->body().add("limit", 1).addNewDocument("filter").add("email", email);
        OpMsgMessage resp;
        d.send(*req, resp);
        db::mongoEnsureOk(resp, "user check email");
        return !resp.documents().empty();
    }
};

} // namespace repository