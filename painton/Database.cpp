#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDataStream>
#include <QBuffer>
#include <QDebug>

Database::Database(const ConnectionDetails& details) : details(details) {
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName(details.host);
    db.setPort(details.port);
    db.setDatabaseName(details.dbName);
    db.setUserName(details.user);
    db.setPassword(details.password);
}

Database::~Database() {
    close();
}

bool Database::open() {
    if (!db.open()) return false;

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS drawings (id SERIAL PRIMARY KEY, name TEXT);");

    query.exec("CREATE TABLE IF NOT EXISTS shapes ("
        "id SERIAL PRIMARY KEY, "
        "drawing_id INTEGER REFERENCES drawings(id) ON DELETE CASCADE, "
        "shape_type INTEGER, "
        "z_order INTEGER);");

    query.exec("CREATE TABLE IF NOT EXISTS rect_data ("
        "shape_id INTEGER PRIMARY KEY REFERENCES shapes(id) ON DELETE CASCADE, "
        "x INTEGER, y INTEGER, width INTEGER, height INTEGER);");

    query.exec("CREATE TABLE IF NOT EXISTS circle_data ("
        "shape_id INTEGER PRIMARY KEY REFERENCES shapes(id) ON DELETE CASCADE, "
        "x INTEGER, y INTEGER, width INTEGER, height INTEGER);");

    query.exec("CREATE TABLE IF NOT EXISTS line_data ("
        "shape_id INTEGER PRIMARY KEY REFERENCES shapes(id) ON DELETE CASCADE, "
        "x1 INTEGER, y1 INTEGER, x2 INTEGER, y2 INTEGER);");

    query.exec("CREATE TABLE IF NOT EXISTS freehand_data ("
        "shape_id INTEGER PRIMARY KEY REFERENCES shapes(id) ON DELETE CASCADE, "
        "points_blob BYTEA);");

    return true;
}

void Database::close() {
    if (db.isOpen()) {
        db.close();
    }
}

bool Database::saveDrawing(int& id, const QString& name, const QList<Shape*>& shapes) {
    if (!db.transaction()) return false;

    QSqlQuery query;

    if (id == -1) {
        query.prepare("INSERT INTO drawings (name) VALUES (:name) RETURNING id");
        query.bindValue(":name", name);
        if (!query.exec() || !query.next()) {
            db.rollback();
            return false;
        }
        id = query.value(0).toInt();
    }
    else {
        query.prepare("UPDATE drawings SET name = :name WHERE id = :id");
        query.bindValue(":name", name);
        query.bindValue(":id", id);
        if (!query.exec()) {
            db.rollback();
            return false;
        }
        query.prepare("DELETE FROM shapes WHERE drawing_id = :id");
        query.bindValue(":id", id);
        query.exec();
    }

    for (int i = 0; i < shapes.size(); ++i) {
        query.prepare("INSERT INTO shapes (drawing_id, shape_type, z_order) "
            "VALUES (:d_id, :type, :z) RETURNING id");
        query.bindValue(":d_id", id);
        query.bindValue(":type", (int)shapes[i]->getType());
        query.bindValue(":z", i);

        if (!query.exec() || !query.next()) {
            qDebug() << "Error inserting master shape:" << query.lastError();
            db.rollback();
            return false;
        }

        int shapeId = query.value(0).toInt();

        QSqlQuery detailQuery;
        if (!shapes[i]->saveToSql(detailQuery, shapeId)) {
            qDebug() << "Error inserting shape details:" << detailQuery.lastError();
            db.rollback();
            return false;
        }
    }

    return db.commit();
}

QList<Shape*> Database::loadDrawing(int id) {
    QList<Shape*> shapes;
    QSqlQuery query;
    query.prepare("SELECT id, shape_type FROM shapes WHERE drawing_id = :id ORDER BY z_order ASC");
    query.bindValue(":id", id);

    if (query.exec()) {
        while (query.next()) {
            int shapeId = query.value(0).toInt();
            Shape::Type type = static_cast<Shape::Type>(query.value(1).toInt());

            Shape* s = Shape::createFromType(type);
            if (s) {
                QSqlQuery detailQuery;
                if (s->loadFromSql(detailQuery, shapeId)) {
                    shapes.append(s);
                }
                else {
                    qDebug() << "Failed to load details for shape ID:" << shapeId;
                    delete s;
                }
            }
        }
    }
    return shapes;
}

QList<Database::DrawingInfo> Database::getSavedDrawings() {
    QList<Database::DrawingInfo> list;
    QSqlQuery query("SELECT id, name FROM drawings");
    while (query.next()) {
        list.append({ query.value(0).toInt(), query.value(1).toString() });
    }
    return list;
}

bool Database::deleteDrawing(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM drawings WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

bool Database::clearDatabase() {
    QSqlQuery query;
    if (!query.exec("TRUNCATE drawings, shapes RESTART IDENTITY CASCADE")) {
        qDebug() << "Error clearing database:" << query.lastError();
        return false;
    }
    return true;
}