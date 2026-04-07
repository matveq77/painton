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
#include <QInputDialog>
#include <QLineEdit>
#include <QDebug>

#include "Shape.h"
#include "Rectangle.h"
#include "Circle.h"
#include "Line.h"
#include "Freehand.h"

#include "Database.h"

class PaintWidget : public QWidget {
public:
    enum class Action {
        Draw,
        Move,
        Delete
    };

    PaintWidget(QWidget* parent = nullptr) : QWidget(parent) {
        drawing = false;
        currentAction = Action::Draw;
        currentShapeType = Shape::Freehand;
        selectedShapeIndex = -1;

        if (!db.open()) {
            qDebug() << "Database Error: Could not open connection";
        }
    }

    ~PaintWidget() {
        qDeleteAll(shapes);
    }

    void setDrawMode(Shape::Type type) {
        currentAction = Action::Draw;
        currentShapeType = type;
        selectedShapeIndex = -1;
        update();
    }

    void setMoveMode() {
        currentAction = Action::Move;
        selectedShapeIndex = -1;
        update();
    }

    void setDeleteMode() {
        currentAction = Action::Delete;
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
        QString fileName = QFileDialog::getSaveFileName(this, "Save File", "", "Paint Files (*.pnt)");
        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QDataStream out(&file);
            out << (int)shapes.size();
            for (const auto& s : shapes) {
                out << (int)s->getType();
                s->serialize(out);
            }
        }
    }

    void loadFromFile() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open File", "", "Paint Files (*.pnt)");
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
                Shape* s = Shape::createFromType(static_cast<Shape::Type>(typeInt));
                if (s) {
                    s->deserialize(in);
                    shapes.append(s);
                }
            }
            selectedShapeIndex = -1;
            update();
        }
    }

    void saveToDatabase() {
        bool ok;
        QString name = QInputDialog::getText(this, "DB Save", "Name:", QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty()) {
            db.saveDrawing(name, shapes);
        }
    }

    void loadFromDatabase() {
        QStringList items = db.getSavedDrawingNames();
        if (items.isEmpty()) return;

        bool ok;
        QString item = QInputDialog::getItem(this, "DB Load", "Choose:", items, 0, false, &ok);
        if (ok && !item.isEmpty()) {
            qDeleteAll(shapes);
            shapes = db.loadDrawing(item);
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
            if (i == selectedShapeIndex && currentAction == Action::Move)
                painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
            else
                painter.setPen(QPen(Qt::black, 2, Qt::SolidLine));

            shapes[i]->draw(painter);
        }

        if (drawing && currentAction == Action::Draw) {
            painter.setPen(QPen(Qt::black, 1, Qt::DashLine));
            Shape* tempShape = Shape::createFromType(currentShapeType);
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

            switch (currentAction) {
            case Action::Delete:
                for (int i = shapes.size() - 1; i >= 0; --i) {
                    if (shapes[i]->isPointInShape(event->pos())) {
                        delete shapes.takeAt(i);
                        break;
                    }
                }
                break;

            case Action::Move:
                selectedShapeIndex = -1;
                for (int i = shapes.size() - 1; i >= 0; --i) {
                    if (shapes[i]->isPointInShape(event->pos())) {
                        selectedShapeIndex = i;
                        lastMousePos = event->pos();
                        break;
                    }
                }
                break;

            case Action::Draw:
                drawing = true;
                if (currentShapeType == Shape::Freehand) {
                    currentPoly.clear();
                    currentPoly << event->pos();
                }
                break;
            }
            update();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (event->buttons() & Qt::LeftButton) {
            if (currentAction == Action::Move && selectedShapeIndex != -1) {
                QPoint delta = event->pos() - lastMousePos;
                shapes[selectedShapeIndex]->move(delta);
                lastMousePos = event->pos();
            }
            else if (drawing && currentAction == Action::Draw) {
                currentPoint = event->pos();
                if (currentShapeType == Shape::Freehand) {
                    currentPoly << event->pos();
                }
            }
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        selectedShapeIndex = -1;
        if (event->button() == Qt::LeftButton) {
            if (drawing && currentAction == Action::Draw) {
                Shape* newShape = Shape::createFromType(currentShapeType);
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

    Action currentAction;
    Shape::Type currentShapeType;

    int selectedShapeIndex;
    Database db;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Painton");
    window.resize(1200, 800);

    QVBoxLayout* mainLayout = new QVBoxLayout(&window);
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    PaintWidget* paintArea = new PaintWidget();

    QPushButton* btnFree = new QPushButton("Free");
    QPushButton* btnLine = new QPushButton("Line");
    QPushButton* btnRect = new QPushButton("Reactangle");
    QPushButton* btnCirc = new QPushButton("Circle");
    QPushButton* btnMove = new QPushButton("Move");
    QPushButton* btnDel = new QPushButton("Delete");
    QPushButton* btnClear = new QPushButton("Clear");

    QPushButton* btnSave = new QPushButton("Save(File)");
    QPushButton* btnLoad = new QPushButton("Open(File)");
    QPushButton* btnSaveDb = new QPushButton("Save(Db)");
    QPushButton* btnLoadDb = new QPushButton("Open(Db)");

    buttonLayout->addWidget(btnFree);
    buttonLayout->addWidget(btnLine);
    buttonLayout->addWidget(btnRect);
    buttonLayout->addWidget(btnCirc);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(btnMove);
    buttonLayout->addWidget(btnDel);
    buttonLayout->addWidget(btnClear);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnLoad);
    buttonLayout->addWidget(btnSaveDb);
    buttonLayout->addWidget(btnLoadDb);

    QObject::connect(btnFree, &QPushButton::clicked, [=]() { paintArea->setDrawMode(Shape::Freehand); });
    QObject::connect(btnLine, &QPushButton::clicked, [=]() { paintArea->setDrawMode(Shape::Line); });
    QObject::connect(btnRect, &QPushButton::clicked, [=]() { paintArea->setDrawMode(Shape::Rectangle); });
    QObject::connect(btnCirc, &QPushButton::clicked, [=]() { paintArea->setDrawMode(Shape::Circle); });

    QObject::connect(btnMove, &QPushButton::clicked, [=]() { paintArea->setMoveMode(); });
    QObject::connect(btnDel, &QPushButton::clicked, [=]() { paintArea->setDeleteMode(); });
    QObject::connect(btnClear, &QPushButton::clicked, [=]() { paintArea->clearAll(); });

    QObject::connect(btnSave, &QPushButton::clicked, [=]() { paintArea->saveToFile(); });
    QObject::connect(btnLoad, &QPushButton::clicked, [=]() { paintArea->loadFromFile(); });
    QObject::connect(btnSaveDb, &QPushButton::clicked, [=]() { paintArea->saveToDatabase(); });
    QObject::connect(btnLoadDb, &QPushButton::clicked, [=]() { paintArea->loadFromDatabase(); });

    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(paintArea);

    window.show();
    return app.exec();
}