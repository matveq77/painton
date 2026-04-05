#pragma once

#include "Shape.h"

class Circle : public Shape {
public:
    Circle();
    Circle(const QRect& rect);

    void draw(QPainter& painter) override;
    bool isPointInShape(QPoint p) override;
    void move(QPoint delta) override;
    void serialize(QDataStream& out) const override;
    void deserialize(QDataStream& in) override;

    void setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) override;

private:
    QRect rect_;
};