use("car_rental_db");

print("========== ВАЛИДАЦИЯ СХЕМЫ ==========\n");

print("1. Создание валидации для коллекции cars");

db.runCommand({
    collMod: "cars",
    validator: {
        $jsonSchema: {
            bsonType: "object",
            title: "Car Validation",
            required: ["id", "brand", "model", "licensePlate", "carClass", "pricePerDay", "year", "status"],
            properties: {
                id: {
                    bsonType: "long",
                    description: "Уникальный ID автомобиля"
                },
                brand: {
                    bsonType: "string",
                    minLength: 1,
                    maxLength: 50,
                    description: "Марка автомобиля"
                },
                model: {
                    bsonType: "string",
                    minLength: 1,
                    maxLength: 50,
                    description: "Модель автомобиля"
                },
                licensePlate: {
                    bsonType: "string",
                    pattern: "^[A-Z0-9]{6,8}$",
                    description: "Госномер (6-8 символов, буквы и цифры)"
                },
                carClass: {
                    enum: ["economy", "standard", "luxury", "suv"],
                    description: "Класс автомобиля"
                },
                available: {
                    bsonType: "bool",
                    description: "Доступность для аренды"
                },
                pricePerDay: {
                    bsonType: "double",
                    minimum: 0,
                    exclusiveMinimum: true,
                    description: "Цена за день (>0)"
                },
                year: {
                    bsonType: "int",
                    minimum: 2000,
                    maximum: 2025,
                    description: "Год выпуска (2000-2025)"
                },
                color: {
                    bsonType: "string",
                    description: "Цвет автомобиля"
                },
                mileage: {
                    bsonType: "int",
                    minimum: 0,
                    description: "Пробег (неотрицательный)"
                },
                status: {
                    enum: ["active", "maintenance"],
                    description: "Статус автомобиля"
                }
            }
        }
    },
    validationLevel: "strict",
    validationAction: "error"
});

print("  Валидация для коллекции cars добавлена\n");

print("2. Тест валидации - вставка корректных данных:");
try {
    db.cars.insertOne({
        id: NumberLong(5020),
        brand: "TestBrand",
        model: "TestModel",
        licensePlate: "TEST123",
        carClass: "standard",
        available: true,
        pricePerDay: 99.99,
        year: 2022,
        color: "Red",
        mileage: 0,
        status: "active"
    });
    print("  Корректная вставка: УСПЕШНО");
    db.cars.deleteOne({ id: 5020 });
} catch(e) {
    print("  Ошибка: " + e);
}

print("\n3. Тест валидации - вставка НЕКОРРЕКТНЫХ данных:");
print("  Попытка вставить авто с ценой -10 (должна упасть):");
try {
    db.cars.insertOne({
        id: NumberLong(5021),
        brand: "Bad",
        model: "Bad",
        licensePlate: "BAD123",
        carClass: "standard",
        pricePerDay: -10,
        year: 2022,
        status: "active"
    });
    print("  Ошибка: вставка прошла, хотя должна была упасть!");
} catch(e) {
    print("  Успешно отклонено: " + e.message.split("\n")[0]);
}

print("\n4. Проверка существующей валидации:");
const validator = db.getCollectionInfos({ name: "cars" })[0].options.validator;
if (validator) {
    print("  Валидация активна для коллекции cars");
} else {
    print("  Валидация не найдена");
}

print("\n========== ГОТОВО ==========");