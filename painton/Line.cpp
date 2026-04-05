#include "Line.h"
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>

Line::Line() : Shape(Type::Line) {}

Line::Line(const QPoint& start, const QPoint& end) : Shape(Type::Line), start_(start), end_(end) {}

void Line::draw(QPainter& painter) {
    painter.drawLine(start_, end_);
}

bool Line::isPointInShape(QPoint p) {
    QPainterPath path;
    path.moveTo(start_);
    path.lineTo(end_);

    QPainterPathStroker stroker;
    stroker.setWidth(10);
    QPainterPath outline = stroker.createStroke(path);

    return outline.contains(p);
}

void Line::move(QPoint delta) {
    start_ += delta;
    end_ += delta;
}

void Line::serialize(QDataStream& out) const {
    out << start_ << end_;
}

void Line::deserialize(QDataStream& in) {
    in >> start_ >> end_;
}

void Line::setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) {
    start_ = start;
    end_ = end;
}