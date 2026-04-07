#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QtSql/QSqlDatabase>
#include "Shape.h"

class Database {
public:
    Database(const QString& path = "paint_data.db");
    ~Database();

    bool open();
    void close();

    bool saveDrawing(const QString& name, const QList<Shape*>& shapes);
    QList<Shape*> loadDrawing(const QString& name);
    QStringList getSavedDrawingNames();
    bool deleteDrawing(const QString& name);

private:
    QSqlDatabase m_db;
    QString m_path;

    QByteArray serializeShapes(const QList<Shape*>& shapes);
    QList<Shape*> deserializeShapes(const QByteArray& data);
};

