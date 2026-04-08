#include "Line.h"
#include <QSqlQuery>
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

bool Line::saveToSql(QSqlQuery& query, int shapeId) const {
    query.prepare("INSERT INTO line_data (shape_id, x1, y1, x2, y2) "
        "VALUES (:id, :x1, :y1, :x2, :y2)");
    query.bindValue(":id", shapeId);
    query.bindValue(":x1", start_.x());
    query.bindValue(":y1", start_.y());
    query.bindValue(":x2", end_.x());
    query.bindValue(":y2", end_.y());

    return query.exec();
}

bool Line::loadFromSql(QSqlQuery& query, int shapeId) {
    query.prepare("SELECT x1, y1, x2, y2 FROM line_data WHERE shape_id = :id");
    query.bindValue(":id", shapeId);

    if (query.exec() && query.next()) {
        int x1 = query.value("x1").toInt();
        int y1 = query.value("y1").toInt();
        int x2 = query.value("x2").toInt();
        int y2 = query.value("y2").toInt();
        start_ = QPoint(x1, y1);
        end_ = QPoint(x2, y2);
        return true;
    }
    return false;
}

void Line::setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) {
    start_ = start;
    end_ = end;
}