#ifndef GRAPHICS_VIEW_H
#define GRAPHICS_VIEW_H

#include <QGraphicsView>
#include <memory>
#include <vector>
#include "types.h"


struct Node {
    int event_index;
    int pid;
    QGraphicsItem* view;
    Event event;
    std::vector<std::weak_ptr<Node>> children;
    std::vector<std::weak_ptr<Node>> parents;// Do I?
};

class graphics_view: public QGraphicsView
{
    Q_OBJECT
public:
    graphics_view(QWidget *parent=0);

    void apply_zoom(qreal amount);

    void pan(qreal dx, qreal dy);

    void create_scene(const std::vector<std::shared_ptr<Event>>& events);
public slots:
    void reset();

protected:

    std::vector<std::shared_ptr<Node>> nodes;
};

#endif // GRAPHICS_VIEW_H
