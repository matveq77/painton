#include "Freehand.h"
#include <QSqlQuery>
#include <QDataStream>
#include <QBuffer>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>

Freehand::Freehand() : Shape(Type::Freehand) {}

Freehand::Freehand(const QPolygon& points) : Shape(Type::Freehand), points_(points) {}

void Freehand::draw(QPainter& painter) {
    painter.drawPolyline(points_);
}

bool Freehand::isPointInShape(QPoint p) {
    if (points_.isEmpty()) return false;

    QPainterPath path;
    path.addPolygon(points_);

    QPainterPathStroker stroker;
    stroker.setWidth(10);
    QPainterPath outline = stroker.createStroke(path);

    return outline.contains(p);
}

void Freehand::move(QPoint delta) {
    points_.translate(delta);
}

void Freehand::serialize(QDataStream& out) const {
    out << points_;
}

void Freehand::deserialize(QDataStream& in) {
    in >> points_;
}

bool Freehand::saveToSql(QSqlQuery& query, int shapeId) const {
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    QDataStream out(&buffer);
    out << points_;
    buffer.close();

    query.prepare("INSERT INTO freehand_data (shape_id, points_blob) VALUES (:id, :data)");
    query.bindValue(":id", shapeId);
    query.bindValue(":data", byteArray);

    return query.exec();
}

bool Freehand::loadFromSql(QSqlQuery& query, int shapeId) {
    query.prepare("SELECT points_blob FROM freehand_data WHERE shape_id = :id");
    query.bindValue(":id", shapeId);

    if (query.exec() && query.next()) {
        QByteArray byteArray = query.value("points_blob").toByteArray();

        QDataStream in(byteArray);
        in >> points_;
        return true;
    }
    return false;
}

void Freehand::setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) {
    points_ = poly;
}