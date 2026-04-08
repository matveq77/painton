#pragma once

#include "Shape.h"

class Rectangle : public Shape {
public:
    Rectangle();
    Rectangle(const QRect& rect);

    void draw(QPainter& painter) override;
    bool isPointInShape(QPoint p) override;
    void move(QPoint delta) override;

    void serialize(QDataStream& out) const override;
    void deserialize(QDataStream& in) override;

    bool saveToSql(QSqlQuery& query, int shapeId) const override;
    bool loadFromSql(QSqlQuery& query, int shapeId) override;

    void setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) override;

private:
    QRect rect_;
};