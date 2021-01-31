#ifndef GRAPHICS_VIEW_H
#define GRAPHICS_VIEW_H

#include <QGraphicsView>
#include <QFont>
#include <memory>
#include <vector>
#include "types.h"
#include <optional>


struct Node {
    Node(size_t index, QGraphicsItem* item, std::shared_ptr<Event> event):
        event_index(index),
        view(item),
        event(event)
    {}
    size_t event_index;
    QGraphicsItem* view;
    std::shared_ptr<Event> event;
    std::vector<std::weak_ptr<Node>> children;
    std::weak_ptr<Node> parent;
};

class graphics_view: public QGraphicsView
{
    Q_OBJECT
public:
    graphics_view(QWidget *parent=0);

    void apply_zoom(qreal amount);

    void pan(qreal dx, qreal dy);

    void create_scene(const std::vector<std::shared_ptr<Event>>& events);

    void layout_scene();
public slots:
    void reset();

    void move_left();

    void move_right();

protected:
    void mousePressEvent(QMouseEvent *event) override;


    std::optional<size_t> selected_node;
    size_t pid_count = 0;
    std::vector<std::shared_ptr<Node>> nodes;
    std::map<QGraphicsItem*, size_t> event_indexes;
    QFont render_font;
};

#endif // GRAPHICS_VIEW_H
