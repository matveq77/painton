#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDataStream>
#include <QBuffer>
#include <QDebug>

Database::Database(const ConnectionDetails& details) : m_details(details) {
    m_db = QSqlDatabase::addDatabase("QPSQL");
    m_db.setHostName(m_details.host);
    m_db.setPort(m_details.port);
    m_db.setDatabaseName(m_details.dbName);
    m_db.setUserName(m_details.user);
    m_db.setPassword(m_details.password);
}

Database::~Database() {
    close();
}

bool Database::open() {
    if (!m_db.open()) {
        qDebug() << "Error: connection with database fail" << m_db.lastError();
        return false;
    }

    QSqlQuery query;

    if (!query.exec("CREATE TABLE IF NOT EXISTS drawings ("
        "id SERIAL PRIMARY KEY, "
        "name TEXT);")) {
        qDebug() << "DataBase: error creating drawings table" << query.lastError();
        return false;
    }

    if (!query.exec("CREATE TABLE IF NOT EXISTS shapes ("
        "id SERIAL PRIMARY KEY, "
        "drawing_id INTEGER, "
        "shape_type INTEGER, "
        "shape_data BYTEA, "
        "z_order INTEGER, "
        "FOREIGN KEY(drawing_id) REFERENCES drawings(id) ON DELETE CASCADE);")) {
        qDebug() << "DataBase: error creating shapes table" << query.lastError();
        return false;
    }

    return true;
}

void Database::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool Database::saveDrawing(int& id, const QString& name, const QList<Shape*>& shapes) {
    if (!m_db.transaction()) return false;

    QSqlQuery query;

    if (id == -1) {
        query.prepare("INSERT INTO drawings (name) VALUES (:name) RETURNING id");
        query.bindValue(":name", name);

        if (!query.exec() || !query.next()) {
            m_db.rollback();
            return false;
        }
        id = query.value(0).toInt();
    }
    else {
        query.prepare("UPDATE drawings SET name = :name WHERE id = :id");
        query.bindValue(":name", name);
        query.bindValue(":id", id);

        if (!query.exec()) {
            m_db.rollback();
            return false;
        }

        query.prepare("DELETE FROM shapes WHERE drawing_id = :id");
        query.bindValue(":id", id);

        if (!query.exec()) {
            m_db.rollback();
            return false;
        }
    }

    query.prepare("INSERT INTO shapes (drawing_id, shape_type, shape_data, z_order) "
        "VALUES (:d_id, :type, :data, :z)");

    for (int i = 0; i < shapes.size(); ++i) {
        query.bindValue(":d_id", id);
        query.bindValue(":type", (int)shapes[i]->getType());
        query.bindValue(":data", serializeSingleShape(shapes[i]));
        query.bindValue(":z", i);

        if (!query.exec()) {
            qDebug() << "Error inserting shape:" << query.lastError();
            m_db.rollback(); // Откатываем всё, если хоть одна фигура не вставилась
            return false;
        }
    }

    return m_db.commit();
}

QList<Shape*> Database::loadDrawing(int id) {
    QList<Shape*> shapes;
    QSqlQuery query;
    query.prepare("SELECT shape_type, shape_data FROM shapes WHERE drawing_id = :id ORDER BY z_order ASC");
    query.bindValue(":id", id);

    if (query.exec()) {
        while (query.next()) {
            Shape* s = Shape::createFromType(static_cast<Shape::Type>(query.value(0).toInt()));
            if (s) {
                QDataStream in(query.value(1).toByteArray());
                s->deserialize(in);
                shapes.append(s);
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

QByteArray Database::serializeSingleShape(Shape* shape) {
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    QDataStream out(&buffer);

    shape->serialize(out);
    return byteArray;
}

bool Database::clearDatabase() {
    QSqlQuery query;

    if (!query.exec("TRUNCATE drawings, shapes RESTART IDENTITY CASCADE")) {
        qDebug() << "Error clearing database:" << query.lastError();
        return false;
    }

    return true;
}