#include "Database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDataStream>
#include <QBuffer>
#include <QDebug>
#include <QColor>

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

    //рисунки
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS drawings (id SERIAL PRIMARY KEY, name TEXT);");

    //мастерфигура
    query.exec("CREATE TABLE IF NOT EXISTS shapes ("
        "id SERIAL PRIMARY KEY, "
        "drawing_id INTEGER REFERENCES drawings(id) ON DELETE CASCADE, "
        "shape_type INTEGER, "
        "z_order INTEGER, "
        "color_index INTEGER DEFAULT 0);");

    //фигуры
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

    //цветы
    query.exec("CREATE TABLE IF NOT EXISTS palette ("
        "drawing_id INTEGER REFERENCES drawings(id) ON DELETE CASCADE, "
        "color_index INTEGER, "
        "color_hex TEXT, "
        "PRIMARY KEY (drawing_id, color_index));");

    query.exec("SELECT COUNT(*) FROM palette");
    if (query.next() && query.value(0).toInt() == 0) {
        QStringList defaultColors = { "#000000", "#FF0000", "#00FF00", "#0000FF",
                                     "#FFFF00", "#FF00FF", "#00FFFF", "#FFA500" };
        for (int i = 0; i < 8; ++i) {
            query.prepare("INSERT INTO palette (id, color_hex) VALUES (:id, :color)");
            query.bindValue(":id", i);
            query.bindValue(":color", defaultColors[i]);
            query.exec();
        }
    }

    return true;
}

void Database::close() {
    if (db.isOpen()) {
        db.close();
    }
}

bool Database::saveDrawing(int& id, const QString& name, const QList<Shape*>& shapes, const QVector<QColor>& palette) {
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
        query.prepare("INSERT INTO shapes (drawing_id, shape_type, z_order, color_index) "
            "VALUES (:d_id, :type, :z, :c_idx) RETURNING id");
        query.bindValue(":d_id", id);
        query.bindValue(":type", (int)shapes[i]->getType());
        query.bindValue(":z", i);
        query.bindValue(":c_idx", shapes[i]->getColorIndex());

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

    QSqlQuery pQuery;
    for (int i = 0; i < palette.size(); ++i) {
        pQuery.prepare("INSERT INTO palette (drawing_id, color_index, color_hex) "
            "VALUES (:d_id, :idx, :color) "
            "ON CONFLICT (drawing_id, color_index) DO UPDATE SET color_hex = EXCLUDED.color_hex");
        pQuery.bindValue(":d_id", id);
        pQuery.bindValue(":idx", i);
        pQuery.bindValue(":color", palette[i].name());
        pQuery.exec();
    }

    return db.commit();
}

void Database::savePalette(const QVector<QColor>& palette) {
    QSqlQuery query;
    for (int i = 0; i < palette.size(); ++i) {
        query.prepare("UPDATE palette SET color_hex = :color WHERE id = :id");
        query.bindValue(":id", i);
        query.bindValue(":color", palette[i].name());
        query.exec();
    }
}

QVector<QColor> Database::loadPalette(int drawingId) {
    QVector<QColor> palette(8, Qt::black);
    QSqlQuery query;
    query.prepare("SELECT color_index, color_hex FROM palette WHERE drawing_id = :id ORDER BY color_index ASC");
    query.bindValue(":id", drawingId);

    if (query.exec()) {
        while (query.next()) {
            int idx = query.value(0).toInt();
            if (idx >= 0 && idx < 8) palette[idx] = QColor(query.value(1).toString());
        }
    }
    return palette;
}

QList<Shape*> Database::loadDrawing(int id) {
    QList<Shape*> shapes;
    QSqlQuery query;
    query.prepare("SELECT id, shape_type, color_index FROM shapes WHERE drawing_id = :id ORDER BY z_order ASC");
    query.bindValue(":id", id);

    if (query.exec()) {
        while (query.next()) {
            int shapeId = query.value(0).toInt();
            Shape::Type type = static_cast<Shape::Type>(query.value(1).toInt());
            int colorIdx = query.value(2).toInt();
                
            Shape* s = Shape::createFromType(type);
            if (s) {
                s->setColorIndex(colorIdx);
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