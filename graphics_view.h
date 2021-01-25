#ifndef GRAPHICS_VIEW_H
#define GRAPHICS_VIEW_H

#include <QGraphicsView>
#include "types.h"

class graphics_view: public QGraphicsView
{
    Q_OBJECT
public:
    graphics_view(QWidget *parent=0);

    void apply_zoom(qreal amount);

    void pan(qreal dx, qreal dy);

    void create_scene(const QVector<Event>& events);
public slots:
    void reset();
};

#endif // GRAPHICS_VIEW_H
