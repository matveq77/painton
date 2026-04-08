#include "Circle.h"
#include <QSqlQuery>
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


bool Circle::saveToSql(QSqlQuery& query, int shapeId) const {
    query.prepare("INSERT INTO circle_data (shape_id, x, y, width, height) "
        "VALUES (:id, :x, :y, :w, :h)");
    query.bindValue(":id", shapeId);
    query.bindValue(":x", rect_.x());
    query.bindValue(":y", rect_.y());
    query.bindValue(":w", rect_.width());
    query.bindValue(":h", rect_.height());

    return query.exec();
}

bool Circle::loadFromSql(QSqlQuery& query, int shapeId) {
    query.prepare("SELECT x, y, width, height FROM circle_data WHERE shape_id = :id");
    query.bindValue(":id", shapeId);

    if (query.exec() && query.next()) {
        int x = query.value("x").toInt();
        int y = query.value("y").toInt();
        int w = query.value("width").toInt();
        int h = query.value("height").toInt();
        rect_ = QRect(x, y, w, h);
        return true;
    }
    return false;
}

void Circle::setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) {
    rect_ = QRect(start, end).normalized();
}