# Документная модель MongoDB

## Коллекции

### users

```
{
    id: Long,              // уникальный ID
    login: String,         // логин (уникальный)
    passwordHash: String,  // хеш пароля
    firstName: String,     // имя
    lastName: String,      // фамилия
    role: String,          // customer/fleetManager/admin
    email: String,         // email (уникальный)
    createdAt: String      // дата создания
}
```

### cars

```
{
    id: Long,              // уникальный ID
    brand: String,         // марка
    model: String,         // модель
    licensePlate: String,  // госномер (уникальный)
    carClass: String,      // economy/standard/luxury/suv
    available: Boolean,    // доступен ли
    pricePerDay: Double,   // цена за день
    year: Int,             // год выпуска
    color: String,         // цвет
    features: Array,       // доп. оборудование
    location: String,      // Airport/Downtown
    mileage: Int,          // пробег
    status: String         // active/maintenance
}
```

### rentals

```
{
    id: Long,              // уникальный ID
    userId: Long,          // ссылка на users.id
    carId: Long,           // ссылка на cars.id
    startDate: String,     // дата начала
    endDate: String,       // дата окончания
    status: String,        // active/completed/cancelled
    totalPrice: Double,    // итоговая цена
    paymentId: String,     // ID платежа
    insurance: Boolean,    // страховка
    pickupLocation: String,
    returnLocation: String,
    additionalDrivers: Array, // доп. водители (embedded)
    createdAt: String
}
```

### counters

```
{ _id: String, seq: Int }
```

---

## Выбор embedded / references

| Связь                       | Тип       | Причина                         |
| --------------------------- | --------- | ------------------------------- |
| users → rentals             | Reference | Один пользователь → много аренд |
| cars → rentals              | Reference | Один автомобиль → много аренд   |
| rentals → additionalDrivers | Embedded  | Маленький массив                |
| cars → features             | Embedded  | Полностью принадлежит авто      |

---

## Схема связей

```
users (1) ──┬── rentals (M)
cars  (1) ──┘
```

