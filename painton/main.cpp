#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QFileDialog>
#include <QVector>
#include <QDataStream>
#include <QFile>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <Shape.h>

class PaintWidget : public QWidget {
public:
    enum Mode {
        FreeHand,
        Line,
        Circle,
        Rect,
        Move
    };

    struct Shape {
        Mode mode;
        QRect rect;
        QPolygon points;
        QPoint lineStart;
        QPoint lineEnd;
    };

    PaintWidget(QWidget* parent = nullptr) : QWidget(parent) {
        drawing = false;
        currentMode = FreeHand;
        selectedShapeIndex = -1;
    }

    void setMode(Mode mode) {
        currentMode = mode;
        selectedShapeIndex = -1;
        update();
    }

    void clearAll() {
        shapes.clear();
        selectedShapeIndex = -1;
        update();
    }

    void saveToFile() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save", "", "(*.pnt)");
        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QDataStream out(&file);
            out << (int)shapes.size();
            for (const auto& s : shapes) {
                out << (int)s.mode << s.rect << s.points << s.lineStart << s.lineEnd;
            }
            file.close();
        }
    }

    void loadFromFile() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open", "", "(*.pnt)");
        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            shapes.clear();
            QDataStream in(&file);
            int size;
            in >> size;
            for (int i = 0; i < size; ++i) {
                Shape s;
                int m;
                in >> m >> s.rect >> s.points >> s.lineStart >> s.lineEnd;
                s.mode = (Mode)m;
                shapes.append(s);
            }
            file.close();
            selectedShapeIndex = -1;
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        //painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), Qt::white);

        for (int i = 0; i < shapes.size(); ++i) {
            if (i == selectedShapeIndex && currentMode == Move)
                painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
            else
                painter.setPen(QPen(Qt::black, 2, Qt::SolidLine));

            drawSingleShape(painter, shapes[i]);
        }

        if (drawing && currentMode != Move) {
            painter.setPen(QPen(Qt::black, 2, Qt::DashLine));
            Shape tempShape;
            tempShape.mode = currentMode;
            tempShape.rect = QRect(startPoint, currentPoint).normalized();
            tempShape.lineStart = startPoint;
            tempShape.lineEnd = currentPoint;
            tempShape.points = currentPoly;
            drawSingleShape(painter, tempShape);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            startPoint = event->pos();
            currentPoint = event->pos();

            if (currentMode == Move) {
                selectedShapeIndex = -1;
                for (int i = shapes.size() - 1; i >= 0; --i) {
                    if (isPointInShape(event->pos(), shapes[i])) {
                        selectedShapeIndex = i;
                        lastMousePos = event->pos();
                        break;
                    }
                }
            }
            else {
                drawing = true;
                if (currentMode == FreeHand) {
                    currentPoly.clear();
                    currentPoly << event->pos();
                }
            }
            update();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (event->buttons() & Qt::LeftButton) {
            if (currentMode == Move && selectedShapeIndex != -1) {
                QPoint delta = event->pos() - lastMousePos;
                moveShape(shapes[selectedShapeIndex], delta);
                lastMousePos = event->pos();
            }
            else if (drawing) {
                currentPoint = event->pos();
                if (currentMode == FreeHand) {
                    currentPoly << event->pos();
                }
            }
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            selectedShapeIndex = -1;
            if (drawing) {
                Shape newShape;
                newShape.mode = currentMode;
                newShape.rect = QRect(startPoint, event->pos()).normalized();
                newShape.lineStart = startPoint;
                newShape.lineEnd = event->pos();
                newShape.points = currentPoly;
                shapes.append(newShape);
                drawing = false;
            }
            update();
        }
    }

private:
    void drawSingleShape(QPainter& painter, const Shape& s) {
        switch (s.mode) {
            case Line:     painter.drawLine(s.lineStart, s.lineEnd); break;
            case Rect:     painter.drawRect(s.rect); break;
            case Circle:   painter.drawEllipse(s.rect); break;
            case FreeHand: painter.drawPolyline(s.points); break;
            default: break;
        }
    }

    bool isPointInShape(QPoint p, const Shape& s) {
        if (s.mode == Rect) {
            return s.rect.contains(p);
        }
        else if (s.mode == Circle) {
            if (s.rect.width() <= 0 || s.rect.height() <= 0) return false;

            QPointF center = s.rect.center();

            double rx = s.rect.width() / 2.0;
            double ry = s.rect.height() / 2.0;

            double dx = p.x() - center.x();
            double dy = p.y() - center.y();

            return (dx * dx) / (rx * rx) + (dy * dy) / (ry * ry) <= 1.0;
        }
        else if (s.mode == Line) {
            QPainterPath path;
            path.moveTo(s.lineStart);
            path.lineTo(s.lineEnd);

            QPainterPathStroker stroker;
            stroker.setWidth(10);
            QPainterPath outline = stroker.createStroke(path);

            return outline.contains(p);
        }
        else if (s.mode == FreeHand) {
            if (s.points.isEmpty()) return false;

            QPainterPath path;
            path.addPolygon(s.points);

            QPainterPathStroker stroker;
            stroker.setWidth(10);
            QPainterPath outline = stroker.createStroke(path);

            return outline.contains(p);
        }
        return false;
    }

    void moveShape(Shape& s, QPoint delta) {
        s.rect.translate(delta);
        s.lineStart += delta;
        s.lineEnd += delta;
        s.points.translate(delta);
    }

    QList<Shape> shapes;
    bool drawing;
    QPoint startPoint;
    QPoint currentPoint;
    QPoint lastMousePos;
    QPolygon currentPoly;
    Mode currentMode;
    int selectedShapeIndex;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Painton");
    window.resize(1100, 700);

    QVBoxLayout* mainLayout = new QVBoxLayout(&window);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    PaintWidget* paintArea = new PaintWidget();

    QPushButton* btnFree = new QPushButton("Кисть");
    QPushButton* btnLine = new QPushButton("Линия");
    QPushButton* btnRect = new QPushButton("Квадрат");
    QPushButton* btnCirc = new QPushButton("Круг");
    QPushButton* btnMove = new QPushButton("Переместить");
    QPushButton* btnClear = new QPushButton("Очистить");
    QPushButton* btnSave = new QPushButton("Сохранить");
    QPushButton* btnLoad = new QPushButton("Открыть");

    buttonLayout->addWidget(btnFree);
    buttonLayout->addWidget(btnLine);
    buttonLayout->addWidget(btnRect);
    buttonLayout->addWidget(btnCirc);
    buttonLayout->addWidget(btnMove);
    buttonLayout->addWidget(btnClear);
    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnLoad);

    QObject::connect(btnFree, &QPushButton::clicked, [=]() { paintArea->setMode(PaintWidget::FreeHand); });
    QObject::connect(btnLine, &QPushButton::clicked, [=]() { paintArea->setMode(PaintWidget::Line); });
    QObject::connect(btnRect, &QPushButton::clicked, [=]() { paintArea->setMode(PaintWidget::Rect); });
    QObject::connect(btnCirc, &QPushButton::clicked, [=]() { paintArea->setMode(PaintWidget::Circle); });
    QObject::connect(btnMove, &QPushButton::clicked, [=]() { paintArea->setMode(PaintWidget::Move); });
    QObject::connect(btnClear, &QPushButton::clicked, [=]() { paintArea->clearAll(); });
    QObject::connect(btnSave, &QPushButton::clicked, [=]() { paintArea->saveToFile(); });
    QObject::connect(btnLoad, &QPushButton::clicked, [=]() { paintArea->loadFromFile(); });

    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(paintArea);

    window.show();
    return app.exec();
}