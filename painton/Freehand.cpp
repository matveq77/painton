#include "Freehand.h"
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

void Freehand::setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) {
    points_ = poly;
}