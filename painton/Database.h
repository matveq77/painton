#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QtSql/QSqlDatabase>
#include "Shape.h"

class Database {
public:
    struct DrawingInfo {
        int id;
        QString name;
    };

    Database(const QString& path = "paint_data.db");
    ~Database();

    bool open();
    void close();

    bool saveDrawing(int& id, const QString& name, const QList<Shape*>& shapes);
    QList<Shape*> loadDrawing(int id);
    QList<DrawingInfo> getSavedDrawings();
    bool deleteDrawing(int id);
    bool clearDatabase();

private:
    QSqlDatabase m_db;
    QString m_path;
    QByteArray serializeSingleShape(Shape* shape);
};