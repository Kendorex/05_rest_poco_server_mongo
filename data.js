db = db.getSiblingDB("car_rental_db");

var usersCount = 0;
try {
    if (db.getCollectionNames().indexOf("users") !== -1) {
        usersCount = db.users.find().count();
    }
} catch(e) {
    usersCount = 0;
}

if (usersCount > 0) {
    print("Database already initialized (" + usersCount + " users found). Skipping seed data.");
    quit();
}

db.users.drop();
db.cars.drop();
db.rentals.drop();
db.counters.drop();

db.createCollection("users");
db.createCollection("cars");
db.createCollection("rentals");
db.createCollection("counters");


db.users.createIndex({ id: 1 }, { unique: true });
db.users.createIndex({ login: 1 }, { unique: true });
db.users.createIndex({ email: 1 }, { unique: true });

db.cars.createIndex({ id: 1 }, { unique: true });
db.cars.createIndex({ licensePlate: 1 }, { unique: true });
db.rentals.createIndex({ id: 1 }, { unique: true });
db.rentals.createIndex({ userId: 1 });
db.rentals.createIndex({ status: 1 });

db.counters.insertMany([
    { _id: "users", seq: 1000 },
    { _id: "cars", seq: 5000 },
    { _id: "rentals", seq: 8000 }
]);


db.users.insertMany([
    { id: 1001, login: "john_customer", passwordHash: "hash123", firstName: "John", lastName: "Smith", role: "customer", email: "john@example.com", createdAt: "2024-01-15T10:00:00Z" },
    { id: 1002, login: "jane_customer", passwordHash: "hash456", firstName: "Jane", lastName: "Doe", role: "customer", email: "jane@example.com", createdAt: "2024-01-20T10:00:00Z" },
    { id: 1003, login: "bob_manager", passwordHash: "hash789", firstName: "Bob", lastName: "Wilson", role: "fleetManager", email: "bob@example.com", createdAt: "2024-02-01T10:00:00Z" },
    { id: 1004, login: "alice_admin", passwordHash: "hash000", firstName: "Alice", lastName: "Brown", role: "admin", email: "alice@example.com", createdAt: "2024-02-10T10:00:00Z" },
    { id: 1005, login: "charlie_customer", passwordHash: "hash111", firstName: "Charlie", lastName: "Davis", role: "customer", email: "charlie@example.com", createdAt: "2024-02-15T10:00:00Z" },
    { id: 1006, login: "diana_customer", passwordHash: "hash222", firstName: "Diana", lastName: "Miller", role: "customer", email: "diana@example.com", createdAt: "2024-02-20T10:00:00Z" },
    { id: 1007, login: "eve_manager", passwordHash: "hash333", firstName: "Eve", lastName: "Taylor", role: "fleetManager", email: "eve@example.com", createdAt: "2024-03-01T10:00:00Z" },
    { id: 1008, login: "frank_customer", passwordHash: "hash444", firstName: "Frank", lastName: "Anderson", role: "customer", email: "frank@example.com", createdAt: "2024-03-05T10:00:00Z" },
    { id: 1009, login: "grace_customer", passwordHash: "hash555", firstName: "Grace", lastName: "Thomas", role: "customer", email: "grace@example.com", createdAt: "2024-03-10T10:00:00Z" },
    { id: 1010, login: "henry_customer", passwordHash: "hash666", firstName: "Henry", lastName: "Jackson", role: "customer", email: "henry@example.com", createdAt: "2024-03-15T10:00:00Z" }
]);

db.cars.insertMany([
    { id: 5001, brand: "Toyota", model: "Camry", licensePlate: "A123BC", carClass: "standard", available: true, pricePerDay: 65.0, year: 2023, color: "Silver", features: ["GPS", "Bluetooth"], location: "Airport", mileage: 15420, status: "active" },
    { id: 5002, brand: "Honda", model: "Civic", licensePlate: "B456DE", carClass: "economy", available: true, pricePerDay: 45.0, year: 2024, color: "Red", features: ["Bluetooth"], location: "Downtown", mileage: 3200, status: "active" },
    { id: 5003, brand: "Tesla", model: "Model 3", licensePlate: "C789FG", carClass: "luxury", available: false, pricePerDay: 120.0, year: 2023, color: "White", features: ["Autopilot", "GPS"], location: "Airport", mileage: 8900, status: "active" },
    { id: 5004, brand: "Ford", model: "Explorer", licensePlate: "D012HI", carClass: "suv", available: true, pricePerDay: 95.0, year: 2022, color: "Black", features: ["Third Row", "GPS"], location: "Airport", mileage: 45200, status: "active" },
    { id: 5005, brand: "BMW", model: "X5", licensePlate: "E345JK", carClass: "luxury", available: true, pricePerDay: 150.0, year: 2024, color: "Blue", features: ["Leather", "Panoramic Roof"], location: "Downtown", mileage: 1200, status: "active" },
    { id: 5006, brand: "Chevrolet", model: "Malibu", licensePlate: "F678LM", carClass: "standard", available: true, pricePerDay: 60.0, year: 2021, color: "Gray", features: ["GPS"], location: "Airport", mileage: 67800, status: "active" },
    { id: 5007, brand: "Nissan", model: "Altima", licensePlate: "G901NO", carClass: "standard", available: false, pricePerDay: 62.0, year: 2022, color: "White", features: ["Bluetooth"], location: "Downtown", mileage: 34200, status: "maintenance" },
    { id: 5008, brand: "Hyundai", model: "Elantra", licensePlate: "H234PQ", carClass: "economy", available: true, pricePerDay: 40.0, year: 2024, color: "Silver", features: ["USB"], location: "Airport", mileage: 850, status: "active" },
    { id: 5009, brand: "Mercedes", model: "C-Class", licensePlate: "I567RS", carClass: "luxury", available: true, pricePerDay: 140.0, year: 2023, color: "Black", features: ["Leather", "GPS"], location: "Downtown", mileage: 15400, status: "active" },
    { id: 5010, brand: "Kia", model: "Sportage", licensePlate: "J890TU", carClass: "suv", available: true, pricePerDay: 85.0, year: 2023, color: "Green", features: ["GPS", "Roof Rails"], location: "Airport", mileage: 18700, status: "active" }
]);

db.rentals.insertMany([
    { id: 8001, userId: 1001, carId: 5001, startDate: "2024-03-01T10:00:00Z", endDate: "2024-03-05T10:00:00Z", status: "active", totalPrice: 260.0, paymentId: "pay_001", insurance: true, pickupLocation: "Airport", returnLocation: "Airport", createdAt: "2024-02-25T10:00:00Z" },
    { id: 8002, userId: 1002, carId: 5003, startDate: "2024-02-15T09:00:00Z", endDate: "2024-02-20T09:00:00Z", actualEndDate: "2024-02-19T15:00:00Z", status: "early_completed", totalPrice: 600.0, paymentId: "pay_002", insurance: true, pickupLocation: "Airport", returnLocation: "Downtown", createdAt: "2024-02-10T10:00:00Z" },
    { id: 8003, userId: 1001, carId: 5005, startDate: "2024-02-10T14:00:00Z", endDate: "2024-02-12T14:00:00Z", actualEndDate: "2024-02-12T14:00:00Z", status: "completed", totalPrice: 300.0, paymentId: "pay_003", insurance: false, pickupLocation: "Downtown", returnLocation: "Downtown", createdAt: "2024-02-05T10:00:00Z" },
    { id: 8004, userId: 1003, carId: 5004, startDate: "2024-01-20T08:00:00Z", endDate: "2024-01-27T08:00:00Z", actualEndDate: "2024-01-27T08:00:00Z", status: "completed", totalPrice: 665.0, paymentId: "pay_004", insurance: true, pickupLocation: "Airport", returnLocation: "Airport", createdAt: "2024-01-15T10:00:00Z" },
    { id: 8005, userId: 1002, carId: 5002, startDate: "2024-03-05T11:00:00Z", endDate: "2024-03-07T11:00:00Z", status: "active", totalPrice: 90.0, paymentId: "pay_005", insurance: false, pickupLocation: "Downtown", returnLocation: "Downtown", createdAt: "2024-03-01T10:00:00Z" },
    { id: 8006, userId: 1005, carId: 5008, startDate: "2024-02-01T12:00:00Z", endDate: "2024-02-03T12:00:00Z", actualEndDate: "2024-02-02T18:00:00Z", status: "early_completed", totalPrice: 80.0, paymentId: "pay_006", insurance: true, pickupLocation: "Airport", returnLocation: "Airport", createdAt: "2024-01-28T10:00:00Z" },
    { id: 8007, userId: 1006, carId: 5009, startDate: "2024-02-25T09:00:00Z", endDate: "2024-03-01T09:00:00Z", status: "cancelled", totalPrice: 0.0, paymentId: null, insurance: false, pickupLocation: "Downtown", returnLocation: "Downtown", createdAt: "2024-02-20T10:00:00Z" },
    { id: 8008, userId: 1001, carId: 5010, startDate: "2024-03-10T10:00:00Z", endDate: "2024-03-15T10:00:00Z", status: "active", totalPrice: 425.0, paymentId: "pay_008", insurance: true, pickupLocation: "Airport", returnLocation: "Airport", createdAt: "2024-03-05T10:00:00Z" },
    { id: 8009, userId: 1002, carId: 5006, startDate: "2024-01-05T08:00:00Z", endDate: "2024-01-10T08:00:00Z", actualEndDate: "2024-01-10T08:00:00Z", status: "completed", totalPrice: 300.0, paymentId: "pay_009", insurance: false, pickupLocation: "Airport", returnLocation: "Airport", createdAt: "2024-01-01T10:00:00Z" },
    { id: 8010, userId: 1008, carId: 5001, startDate: "2024-03-20T09:00:00Z", endDate: "2024-03-22T09:00:00Z", status: "active", totalPrice: 130.0, paymentId: "pay_010", insurance: true, pickupLocation: "Airport", returnLocation: "Airport", createdAt: "2024-03-15T10:00:00Z" }
]);
