use("car_rental_db");

print("========== CRUD ОПЕРАЦИИ ==========\n");

print("1. CREATE - Вставка данных");
db.cars.insertOne({
    id: 5011,
    brand: "Volkswagen",
    model: "Jetta",
    licensePlate: "K123VW",
    carClass: "economy",
    available: true,
    pricePerDay: 48,
    year: 2024,
    color: "Blue",
    features: ["Bluetooth"],
    location: "Downtown",
    mileage: 100,
    status: "active"
});
print("  Добавлен автомобиль Volkswagen Jetta");

db.rentals.insertOne({
    id: 8011,
    userId: 1001,
    carId: 5011,
    startDate: new Date("2024-04-01"),
    endDate: new Date("2024-04-05"),
    status: "active",
    totalPrice: 192,
    paymentId: "pay_011",
    insurance: true,
    createdAt: new Date()
});
print("  Добавлена аренда #8011\n");

print("2. READ - Поиск документов");
print("  Автомобили класса luxury:");
db.cars.find({ carClass: { $eq: "luxury" } }).forEach(printjson);
print("\n  Автомобили дороже 100$:");
db.cars.find({ pricePerDay: { $gt: 100 } }).forEach(printjson);
print("\n  Доступные автомобили класса luxury:");
db.cars.find({ $and: [{ available: true }, { carClass: "luxury" }] }).forEach(printjson);
print("\n  Активные аренды пользователя 1001:");
db.rentals.find({ userId: 1001, status: "active" }).forEach(printjson);
print("\n  Автомобили по убыванию цены:");
db.cars.find({ available: true }).sort({ pricePerDay: -1 }).forEach(printjson);

print("\n3. UPDATE - Обновление данных");
db.cars.updateOne({ id: 5001 }, { $set: { available: false, mileage: 16000 } });
print("  Автомобиль 5001 - статус updated");

db.cars.updateOne({ id: 5005 }, { $push: { features: "Heated Steering Wheel" } });
print("  Автомобиль 5005 - добавлена фича");

db.rentals.updateMany({ status: "active", endDate: { $lt: new Date() } }, { $set: { status: "completed" } });
print("  Просроченные аренды завершены\n");

print("4. DELETE - Удаление данных");
db.rentals.deleteOne({ id: 8011 });
print("  Аренда 8011 удалена");

var activeRentals = db.rentals.countDocuments({ carId: 5011, status: "active" });
if (activeRentals === 0) {
    db.cars.deleteOne({ id: 5011 });
    print("  Автомобиль 5011 удален (нет активных аренд)\n");
}

print("5. ДОПОЛНИТЕЛЬНЫЕ ЗАПРОСЫ");
print("  Поиск пользователя по логину:");
db.users.findOne({ login: "john_customer" });

print("\n  Поиск пользователей по маске имени и фамилии:");
db.users.find({ firstName: { $regex: "^J", $options: "i" }, lastName: { $regex: "^S", $options: "i" } }).forEach(printjson);

print("\n  Активные аренды с деталями автомобиля:");
db.rentals.aggregate([
    { $match: { userId: 1001, status: "active" } },
    { $lookup: { from: "cars", localField: "carId", foreignField: "id", as: "car" } },
    { $unwind: "$car" },
    { $project: { rentalId: "$id", carModel: "$car.model", carBrand: "$car.brand", startDate: 1, endDate: 1, totalPrice: 1 } }
]).forEach(printjson);

print("\n========== СТАТИСТИКА ==========");
print("Users: " + db.users.countDocuments());
print("Cars: " + db.cars.countDocuments());
print("Rentals: " + db.rentals.countDocuments());
print("\nГотово!");