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
#include <Rectangle.h>
#include <Circle.h>
#include <Line.h>
#include <Freehand.h>

class PaintWidget : public QWidget {
public:
    enum Mode {
        Rectangle = Shape::Rectangle,
        Circle = Shape::Circle,
        Line = Shape::Line,
        Freehand = Shape::Freehand,
        Move
    };

    PaintWidget(QWidget* parent = nullptr) : QWidget(parent) {
        drawing = false;
        currentMode = Freehand;
        selectedShapeIndex = -1;
    }

    ~PaintWidget() {
        qDeleteAll(shapes);
    }

    void setMode(Mode mode) {
        currentMode = mode;
        selectedShapeIndex = -1;
        update();
    }

    void clearAll() {
        qDeleteAll(shapes);
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
                out << (int)s->getType();
                s->serialize(out);
            }
            file.close();
        }
    }

    void loadFromFile() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open", "", "(*.pnt)");
        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            qDeleteAll(shapes);
            shapes.clear();
            QDataStream in(&file);
            int size;
            in >> size;
            for (int i = 0; i < size; ++i) {
                int typeInt;
                in >> typeInt;
                Shape::Type type = (Shape::Type)typeInt;
                Shape* s = Shape::createFromType(type);
                if (s) {
                    s->deserialize(in);
                    shapes.append(s);
                }
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

            shapes[i]->draw(painter);
        }

        if (drawing && currentMode != Move) {
            painter.setPen(QPen(Qt::black, 2, Qt::DashLine));
            Shape* tempShape = Shape::createFromType(static_cast<Shape::Type>(currentMode));
            if (tempShape) {
                tempShape->setFromPoints(startPoint, currentPoint, currentPoly);
                tempShape->draw(painter);
                delete tempShape;
            }
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            startPoint = event->pos();
            currentPoint = event->pos();

            if (currentMode == Move) {
                selectedShapeIndex = -1;
                for (int i = shapes.size() - 1; i >= 0; --i) {
                    if (shapes[i]->isPointInShape(event->pos())) {
                        selectedShapeIndex = i;
                        lastMousePos = event->pos();
                        break;
                    }
                }
            }
            else {
                drawing = true;
                if (currentMode == Freehand) {
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
                shapes[selectedShapeIndex]->move(delta);
                lastMousePos = event->pos();
            }
            else if (drawing) {
                currentPoint = event->pos();
                if (currentMode == Freehand) {
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
                Shape* newShape = Shape::createFromType(static_cast<Shape::Type>(currentMode));
                if (newShape) {
                    newShape->setFromPoints(startPoint, event->pos(), currentPoly);
                    shapes.append(newShape);
                }
                drawing = false;
            }
            update();
        }
    }

private:
    QList<Shape*> shapes;
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

    QObject::connect(btnFree, &QPushButton::clicked, [=]() { paintArea->setMode(PaintWidget::Freehand); });
    QObject::connect(btnLine, &QPushButton::clicked, [=]() { paintArea->setMode(PaintWidget::Line); });
    QObject::connect(btnRect, &QPushButton::clicked, [=]() { paintArea->setMode(PaintWidget::Rectangle); });
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