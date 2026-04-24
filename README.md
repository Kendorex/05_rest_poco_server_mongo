# Система управления арендой автомобилей

## Описание

REST API для аренды автомобилей.

Технологии:

* MongoDB
* C++ (POCO)
* Docker

---

## Запуск

```
docker-compose up -d
```

---

## Подключение к MongoDB

```
docker exec -it $(docker ps -qf "name=mongodb") mongosh
use car_rental_db
```

---

## Запуск скриптов

```
$id = docker ps -qf "name=mongodb"
docker cp queries.js "${id}:/queries.js"

mongosh
  load("queries.js")
```

---

## API эндпоинты

| Метод | Эндпоинт                                        | Описание              |
| ----- | ----------------------------------------------- | --------------------- |
| POST  | /api/v1/users/register                          | Регистрация           |
| GET   | /api/v1/users/search?login=xxx                  | Поиск по логину       |
| GET   | /api/v1/users/search?firstName=xxx&lastName=xxx | Поиск по ФИО          |
| GET   | /api/v1/cars                                    | Список доступных авто |
| GET   | /api/v1/cars/search?class=xxx                   | Фильтр по классу      |
| POST  | /api/v1/cars                                    | Добавление авто       |
| POST  | /api/v1/rentals                                 | Создание аренды       |
| GET   | /api/v1/rentals/active                          | Активные аренды       |
| GET   | /api/v1/rentals/history                         | История               |
| PUT   | /api/v1/rentals/{id}/complete                   | Завершение            |

---

## Остановка

```
docker-compose down
```
