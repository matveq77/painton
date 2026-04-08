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

bool Rectangle::saveToSql(QSqlQuery& query, int shapeId) const {
    query.prepare("INSERT INTO rect_data (shape_id, x, y, width, height) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(shapeId);
    query.addBindValue(rect_.x());
    query.addBindValue(rect_.y());
    query.addBindValue(rect_.width());
    query.addBindValue(rect_.height());
    return query.exec();
}

bool Rectangle::loadFromSql(QSqlQuery& query, int shapeId) {
    query.prepare("SELECT x, y, width, height FROM rect_data WHERE shape_id = ?");
    query.addBindValue(shapeId);
    if (query.exec() && query.next()) {
        rect_ = QRect(query.value(0).toInt(), query.value(1).toInt(),
            query.value(2).toInt(), query.value(3).toInt());
        return true;
    }
    return false;
}

void Rectangle::setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) {
    rect_ = QRect(start, end).normalized();
}