#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDataStream>
#include <QBuffer>
#include <QDebug>

Database::Database(const QString& path) : m_path(path) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(m_path);
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

    QString str = "CREATE TABLE IF NOT EXISTS drawings ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE, "
        "drawing_data BLOB"
        ");";

    if (!query.exec(str)) {
        qDebug() << "DataBase: error of create table" << query.lastError();
        return false;
    }

    return true;
}

void Database::close() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool Database::saveDrawing(const QString& name, const QList<Shape*>& shapes) {
    QByteArray data = serializeShapes(shapes);

    QSqlQuery query;

    query.prepare("INSERT OR REPLACE INTO drawings (name, drawing_data) VALUES (:name, :data)");
    query.bindValue(":name", name);
    query.bindValue(":data", data);

    if (!query.exec()) {
        qDebug() << "Error saving drawing:" << query.lastError();
        return false;
    }
    return true;
}

QList<Shape*> Database::loadDrawing(const QString& name) {
    QSqlQuery query;
    query.prepare("SELECT drawing_data FROM drawings WHERE name = :name");
    query.bindValue(":name", name);

    if (query.exec() && query.next()) {
        QByteArray data = query.value(0).toByteArray();
        return deserializeShapes(data);
    }
    return QList<Shape*>();
}

QStringList Database::getSavedDrawingNames() {
    QStringList names;
    QSqlQuery query("SELECT name FROM drawings");
    while (query.next()) {
        names << query.value(0).toString();
    }
    return names;
}

bool Database::deleteDrawing(const QString& name) {
    QSqlQuery query;
    query.prepare("DELETE FROM drawings WHERE name = :name");
    query.bindValue(":name", name);
    return query.exec();
}

QByteArray Database::serializeShapes(const QList<Shape*>& shapes) {
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    QDataStream out(&buffer);

    out << (int)shapes.size();
    for (const auto& s : shapes) {
        out << (int)s->getType();
        s->serialize(out);
    }
    return byteArray;
}

QList<Shape*> Database::deserializeShapes(const QByteArray& data) {
    QList<Shape*> shapes;
    QDataStream in(data);

    int size;
    in >> size;
    for (int i = 0; i < size; ++i) {
        int typeInt;
        in >> typeInt;
        Shape::Type type = static_cast<Shape::Type>(typeInt);
        Shape* s = Shape::createFromType(type);
        if (s) {
            s->deserialize(in);
            shapes.append(s);
        }
    }
    return shapes;
}