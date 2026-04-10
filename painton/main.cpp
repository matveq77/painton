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
#include <QColorDialog>

#include "Shape.h"
#include "Rectangle.h"
#include "Circle.h"
#include "Line.h"
#include "Freehand.h"
#include "Database.h"

class ColorButton : public QPushButton {
    Q_OBJECT
public:
    int index;
    ColorButton(int idx, QWidget* parent = nullptr) : QPushButton(parent), index(idx) {
        setFixedSize(40, 40);
    }
signals:
    void colorRightClicked(int idx);
    void colorLeftClicked(int idx);
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::RightButton) {
            emit colorRightClicked(index);
        }
        else if (event->button() == Qt::LeftButton) {
            emit colorLeftClicked(index);
            QPushButton::mousePressEvent(event);
        }
    }
};

class PaintWidget : public QWidget {
    Q_OBJECT
public:
    enum class Action { Draw, Move, Delete };

    PaintWidget(QWidget* parent = nullptr) : QWidget(parent), db({ "localhost", 5432, "painton", "postgres", "postgres" }) {
        drawing = false;
        currentAction = Action::Draw;
        currentShapeType = Shape::Freehand;
        selectedShapeIndex = -1;
        palette = { "#000000", "#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#00FFFF", "#FFA500" };
        if (!db.open()) qDebug() << "Database Error";
        activeColorIndex = 0;
    }
    ~PaintWidget() { qDeleteAll(shapes); }

    void setDrawMode(Shape::Type type) { currentAction = Action::Draw; currentShapeType = type; update(); }
    void setMoveMode() { currentAction = Action::Move; update(); }
    void setDeleteMode() { currentAction = Action::Delete; update(); }
    void clearAll() { qDeleteAll(shapes); shapes.clear(); update(); }
    void createNewDrawing() { clearAll(); currentId = -1; currentName = ""; palette = { "#000000", "#FF0000", "#00FF00", "#0000FF", "#FFFF00", "#FF00FF", "#00FFFF", "#FFA500" }; activeColorIndex = 0; emit paletteLoaded(); }

    void setPaletteColor(int idx, QColor color) {
        if (idx >= 0 && idx < palette.size()) {
            palette[idx] = color;
            update();
        }
    }

    QColor getPaletteColor(int idx) const { return palette[idx]; }
    int getActiveColorIndex() const { return activeColorIndex; }
    void setActiveColorIndex(int idx) { activeColorIndex = idx; }

    void saveToDatabase() {
        if (currentId == -1) {
            bool ok;
            QString name = QInputDialog::getText(this, "Save", "Name:", QLineEdit::Normal, "Untitled", &ok);
            if (!ok) return;
            currentName = name.isEmpty() ? "Untitled" : name;
        }
        db.saveDrawing(currentId, currentName, shapes, palette);
    }

    void loadFromDatabase() {
        QList<Database::DrawingInfo> items = db.getSavedDrawings();
        if (items.isEmpty()) return;
        QStringList displayList;
        for (const auto& info : items) displayList << QString("%1(id: %2)").arg(info.name).arg(info.id);
        bool ok;
        QString res = QInputDialog::getItem(this, "Load", "Select:", displayList, 0, false, &ok);
        if (ok && !res.isEmpty()) {
            int index = displayList.indexOf(res);
            qDeleteAll(shapes);
            shapes = db.loadDrawing(items[index].id);
            palette = db.loadPalette(items[index].id);
            currentId = items[index].id;
            currentName = items[index].name;
            emit paletteLoaded();
            update();
        }
    }

    void clearDatabase() { db.clearDatabase(); }

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
        }
    }

signals:
    void paletteLoaded();

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);
        for (int i = 0; i < shapes.size(); ++i) {
            QColor color = palette[shapes[i]->getColorIndex()];
            if (i == selectedShapeIndex && currentAction == Action::Move)
                painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
            else
                painter.setPen(QPen(color, 2, Qt::SolidLine));
            shapes[i]->draw(painter);
        }
        if (drawing && currentAction == Action::Draw) {
            painter.setPen(QPen(palette[activeColorIndex], 1, Qt::DashLine));
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
                if (currentShapeType == Shape::Freehand) currentPoly << event->pos();
            }
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        selectedShapeIndex = -1;
        if (event->button() == Qt::LeftButton && drawing) {
            Shape* newShape = Shape::createFromType(currentShapeType);
            if (newShape) {
                newShape->setFromPoints(startPoint, event->pos(), currentPoly);
                newShape->setColorIndex(activeColorIndex);
                shapes.append(newShape);
            }
            drawing = false;
        }
        update();
    }

private:
    QList<Shape*> shapes;
    bool drawing;
    QPoint startPoint, currentPoint, lastMousePos;
    QPolygon currentPoly;
    Action currentAction;
    Shape::Type currentShapeType;
    int selectedShapeIndex;
    Database db;
    int currentId = -1;
    QString currentName = "";
    QVector<QColor> palette;
    int activeColorIndex;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QWidget window;
    window.setWindowTitle("Painton");
    window.resize(1200, 800);
    QVBoxLayout* mainLayout = new QVBoxLayout(&window);
    QHBoxLayout* topControlLayout = new QHBoxLayout();
    QHBoxLayout* colorPaletteLayout = new QHBoxLayout();
    PaintWidget* paintArea = new PaintWidget();

    QPushButton* btnFree = new QPushButton("Free");
    QPushButton* btnLine = new QPushButton("Line");
    QPushButton* btnRect = new QPushButton("Reactangle");
    QPushButton* btnCirc = new QPushButton("Circle");
    topControlLayout->addWidget(btnFree);
    topControlLayout->addWidget(btnLine);
    topControlLayout->addWidget(btnRect);
    topControlLayout->addWidget(btnCirc);
    topControlLayout->addSpacing(20);

    QPushButton* btnMove = new QPushButton("Move");
    QPushButton* btnDel = new QPushButton("Delete");
    QPushButton* btnCreateNew = new QPushButton("Create new");
    QPushButton* btnClear = new QPushButton("Clear");
    topControlLayout->addWidget(btnMove);
    topControlLayout->addWidget(btnDel);
    topControlLayout->addWidget(btnCreateNew);
    topControlLayout->addWidget(btnClear);
    topControlLayout->addStretch();

    QPushButton* btnSave = new QPushButton("Save(File)");
    QPushButton* btnLoad = new QPushButton("Open(File)");
    QPushButton* btnSaveDb = new QPushButton("Save(Db)");
    QPushButton* btnLoadDb = new QPushButton("Open(Db)");
    QPushButton* btnClearDb = new QPushButton("Clear(Db)");
    //topControlLayout->addWidget(btnSave);
    //topControlLayout->addWidget(btnLoad);
    topControlLayout->addWidget(btnSaveDb);
    topControlLayout->addWidget(btnLoadDb);
    topControlLayout->addWidget(btnClearDb);

    QVector<ColorButton*> paletteButtons;
    auto updatePaletteUI = [&]() {
        for (int i = 0; i < 8; ++i) {
            QColor col = paintArea->getPaletteColor(i);
            paletteButtons[i]->setStyleSheet(QString("background-color: %1; border: 2px solid #555;").arg(col.name()));
            paletteButtons[i]->setText(i == paintArea->getActiveColorIndex() ? "●" : "");
        }
        };

    for (int i = 0; i < 8; ++i) {
        ColorButton* cBtn = new ColorButton(i);
        paletteButtons.append(cBtn);
        colorPaletteLayout->addWidget(cBtn);
        QObject::connect(cBtn, &ColorButton::colorLeftClicked, [=, &paletteButtons](int clickedIdx) {
            paintArea->setActiveColorIndex(clickedIdx);
            updatePaletteUI();
            });
        QObject::connect(cBtn, &ColorButton::colorRightClicked, [=, &window](int clickedIdx) {
            QColor newCol = QColorDialog::getColor(paintArea->getPaletteColor(clickedIdx), &window, "Pick Color");
            if (newCol.isValid()) {
                paintArea->setPaletteColor(clickedIdx, newCol);
                updatePaletteUI();      
            }
            });
    }
    updatePaletteUI();

    QObject::connect(paintArea, &PaintWidget::paletteLoaded, updatePaletteUI);
    QObject::connect(btnCreateNew, &QPushButton::clicked, [=]() { paintArea->createNewDrawing(); });
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
    QObject::connect(btnClearDb, &QPushButton::clicked, [=]() { paintArea->clearDatabase(); });

    mainLayout->addLayout(topControlLayout);
    mainLayout->addLayout(colorPaletteLayout);
    mainLayout->addWidget(paintArea);

    window.show();
    return app.exec();
}

#include "main.moc"