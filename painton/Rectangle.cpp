#include "Rectangle.h"
#include <QPainter>

Rectangle::Rectangle() : Shape(Type::Rectangle) {}

Rectangle::Rectangle(const QRect& rect) : Shape(Type::Rectangle), rect_(rect) {}

void Rectangle::draw(QPainter& painter) {
    painter.drawRect(rect_);
}

bool Rectangle::isPointInShape(QPoint p) {
    return rect_.contains(p);
}

void Rectangle::move(QPoint delta) {
    rect_.translate(delta);
}

void Rectangle::serialize(QDataStream& out) const {
    out << rect_;
}

void Rectangle::deserialize(QDataStream& in) {
    in >> rect_;
}

void Rectangle::setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) {
    rect_ = QRect(start, end).normalized();
}