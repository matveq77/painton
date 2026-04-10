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

    struct ConnectionDetails {
        QString host;
        int port;
        QString dbName;
        QString user;
        QString password;
    };

    Database(const ConnectionDetails& details);
    ~Database();

    bool open();
    void close();

    void savePalette(const QVector<QColor>& palette);
    QVector<QColor> loadPalette(int drawingId);

    bool saveDrawing(int& id, const QString& name, const QList<Shape*>& shapes, const QVector<QColor>& palette);
    QList<Shape*> loadDrawing(int id);
    QList<DrawingInfo> getSavedDrawings();
    bool deleteDrawing(int id);
    bool clearDatabase();

private:
    QSqlDatabase db;
    ConnectionDetails details;
    QByteArray serializeSingleShape(Shape* shape);
};