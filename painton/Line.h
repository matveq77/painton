#pragma once

#include "Shape.h"

class Line : public Shape {
public:
    Line();
    Line(const QPoint& start, const QPoint& end);

    void draw(QPainter& painter) override;
    bool isPointInShape(QPoint p) override;
    void move(QPoint delta) override;
    void serialize(QDataStream& out) const override;
    void deserialize(QDataStream& in) override;

    void setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) override;

private:
    QPoint start_;
    QPoint end_;
};