#include "Shape.h"
#include "Rectangle.h"
#include "Circle.h"
#include "Line.h"
#include "Freehand.h"

Shape::Shape(Type type) : type_(type) {}

Shape* Shape::createFromType(Type type) {
    switch (type) {
    case Type::Rectangle: return new ::Rectangle();
    case Type::Circle: return new ::Circle();
    case Type::Line: return new ::Line();
    case Type::Freehand: return new ::Freehand();
    default: return nullptr;
    }
}
