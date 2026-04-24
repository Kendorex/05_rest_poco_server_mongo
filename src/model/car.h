#pragma once

#include <string>
#include <regex>
#include <stdexcept>
#include <chrono>
#include <sstream>

namespace model {

// Класс автомобиля
class Car {
public:
    std::string id;              // Уникальный ID
    std::string model;           // Модель
    std::string brand;           // Бренд/Марка
    std::string licensePlate;    // Госномер
    std::string carClass;        // Класс: economy, compact, midsize, standard, fullsize, luxury, suv
    bool available;              // Доступен ли для аренды
    double pricePerDay;          // Цена за день в USD
    int year;                    // Год выпуска
    std::string color;           // Цвет
    
    // Конструктор (без brand - используем model как brand)
    Car(const std::string& mdl,
        const std::string& plate,
        const std::string& cls,
        double price = 50.0,
        int yr = 2020,
        const std::string& clr = "White")
        : model(mdl)
        , brand(mdl)  // По умолчанию brand = model
        , licensePlate(plate)
        , carClass(cls)
        , available(true)
        , pricePerDay(price)
        , year(yr)
        , color(clr)
    {
        // Генерируем ID из госномера + timestamp
        id = "CAR_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count() % 10000
        );
        
        if (model.empty() || licensePlate.empty()) {
            throw std::invalid_argument("Model and license plate are required");
        }
        
        if (pricePerDay <= 0) {
            throw std::invalid_argument("Price must be positive");
        }
    }
    
    // Конструктор с явным указанием brand
    Car(const std::string& mdl,
        const std::string& brnd,
        const std::string& plate,
        const std::string& cls,
        double price,
        int yr,
        const std::string& clr)
        : model(mdl)
        , brand(brnd)
        , licensePlate(plate)
        , carClass(cls)
        , available(true)
        , pricePerDay(price)
        , year(yr)
        , color(clr)
    {
        id = "CAR_" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count() % 10000
        );
        
        if (model.empty() || brand.empty() || licensePlate.empty()) {
            throw std::invalid_argument("Model, brand and license plate are required");
        }
        
        if (pricePerDay <= 0) {
            throw std::invalid_argument("Price must be positive");
        }
    }
    
    // Конструктор по умолчанию
    Car() : available(true), pricePerDay(0), year(0) {}
    
    // Конструктор для загрузки из БД (с установкой всех полей)
    Car(const std::string& _id,
        const std::string& _model,
        const std::string& _brand,
        const std::string& _licensePlate,
        const std::string& _carClass,
        bool _available,
        double _pricePerDay,
        int _year,
        const std::string& _color)
        : id(_id)
        , model(_model)
        , brand(_brand)
        , licensePlate(_licensePlate)
        , carClass(_carClass)
        , available(_available)
        , pricePerDay(_pricePerDay)
        , year(_year)
        , color(_color)
    {}
    
    // Метод для проверки доступности
    bool isAvailable() const {
        return available;
    }
    
    // Метод для бронирования автомобиля
    void reserve() {
        if (!available) {
            throw std::runtime_error("Car is already reserved");
        }
        available = false;
    }
    
    // Метод для освобождения автомобиля
    void release() {
        available = true;
    }
    
    // Проверка соответствия классу
    bool matchesClass(const std::string& className) const {
        if (className.empty()) return true;
        return carClass == className;
    }
    
    // Проверка соответствия маске модели
    bool matchesModelMask(const std::string& mask) const {
        if (mask.empty()) return true;
        
        // Простое сравнение с учетом * и ?
        auto buildRegex = [](const std::string& m) -> std::regex {
            std::string pattern;
            for (char c : m) {
                if (c == '*') pattern += ".*";
                else if (c == '?') pattern += ".";
                else pattern += c;
            }
            return std::regex(pattern, std::regex::icase);
        };
        
        return std::regex_match(model, buildRegex(mask));
    }
    
    // Получение информации для JSON
    std::string toJson() const {
        std::stringstream ss;
        ss << "{"
           << "\"id\":\"" << id << "\","
           << "\"model\":\"" << model << "\","
           << "\"brand\":\"" << brand << "\","
           << "\"licensePlate\":\"" << licensePlate << "\","
           << "\"class\":\"" << carClass << "\","
           << "\"available\":" << (available ? "true" : "false") << ","
           << "\"pricePerDay\":" << pricePerDay << ","
           << "\"year\":" << year << ","
           << "\"color\":\"" << color << "\""
           << "}";
        return ss.str();
    }
};

} // namespace model