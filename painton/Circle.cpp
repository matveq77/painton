#include "Circle.h"
#include <QPainter>

Circle::Circle() : Shape(Type::Circle) {}

Circle::Circle(const QRect& rect) : Shape(Type::Circle), rect_(rect) {}

void Circle::draw(QPainter& painter) {
    painter.drawEllipse(rect_);
}

bool Circle::isPointInShape(QPoint p) {
    if (rect_.width() <= 0 || rect_.height() <= 0) return false;

    QPointF center = rect_.center();
    double rx = rect_.width() / 2.0;
    double ry = rect_.height() / 2.0;
    double dx = p.x() - center.x();
    double dy = p.y() - center.y();

    return (dx * dx) / (rx * rx) + (dy * dy) / (ry * ry) <= 1.0;
}

void Circle::move(QPoint delta) {
    rect_.translate(delta);
}

void Circle::serialize(QDataStream& out) const {
    out << rect_;
}

void Circle::deserialize(QDataStream& in) {
    in >> rect_;
}

void Circle::setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) {
    rect_ = QRect(start, end).normalized();
}