#pragma once

#include <QRect>
#include <QPolygon>
#include <QPoint>
#include <QDataStream>

class QPainter;

class Shape {
public:
    enum Type {
        Rectangle,
        Circle,
        Line,
        Freehand
    };

    Shape(Type type);
    virtual ~Shape() = default;

    virtual void draw(QPainter& painter) = 0;
    virtual bool isPointInShape(QPoint p) = 0;
    virtual void move(QPoint delta) = 0;
    virtual void serialize(QDataStream& out) const = 0;
    virtual void deserialize(QDataStream& in) = 0;

    virtual void setFromPoints(const QPoint& start, const QPoint& end, const QPolygon& poly) = 0;

    Type getType() const { return type_; }

    static Shape* createFromType(Type type);

protected:
    Type type_;
};

