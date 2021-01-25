#ifndef TARPAULINVIEWER_H
#define TARPAULINVIEWER_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QKeyEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class TarpaulinViewer; }
QT_END_NAMESPACE

class TarpaulinViewer : public QMainWindow
{
    Q_OBJECT

public:
    TarpaulinViewer(QWidget *parent = nullptr);
    ~TarpaulinViewer();
public slots:
    void load_traces();
protected:
    void keyReleaseEvent(QKeyEvent* event) override;
private:
    Ui::TarpaulinViewer *ui;

    QGraphicsScene *scene;
};
#endif // TARPAULINVIEWER_H
