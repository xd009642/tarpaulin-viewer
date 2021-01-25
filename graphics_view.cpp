#include "graphics_view.h"
#include <QScrollBar>
#include <QTouchEvent>
#include <QDebug>
#include <QGraphicsTextItem>
#include <QFontMetrics>
#include <QFont>
#include <map>

graphics_view::graphics_view(QWidget *parent):
    QGraphicsView(parent)
{

}

void graphics_view::apply_zoom(qreal amount) {
    scale(amount, amount);
    update();
}

void graphics_view::pan(qreal dx, qreal dy) {
    translate(dx, dy);
    update();
}


void graphics_view::reset() {
    resetTransform();
}


void graphics_view::create_scene(const QVector<Event>& events) {
    qreal MARGIN = 30.0;
    QGraphicsScene* s = scene();
    s->clear();
    QVector<std::pair<qreal, QString>> configs;
    QVector<std::pair<qreal, TestBinary>> binaries;
    qreal x_pos = MARGIN;
    QFont render_font;
    QFontMetrics fm(render_font);
    std::map<int, qreal> pid_heights;
    auto height = 4.1 * fm.height();
    qreal y_pos = - (MARGIN + height);
    for(const auto& event: events) {
        if(auto conf = std::get_if<Config>(&event)) {
            configs.push_back(std::make_pair(x_pos, conf->name));
            x_pos += MARGIN + fm.width(conf->name);
        } else if(auto bin = std::get_if<TestBinary>(&event)) {
            binaries.push_back(std::make_pair(x_pos, TestBinary(*bin)));
            x_pos += MARGIN + fm.width(bin->path);
        } else if(auto trace = std::get_if<TraceEvent>(&event)) {
            if(trace->pid) {
                auto y = 0.0;
                auto y_it = pid_heights.find(trace->pid.value());
                if(y_it != pid_heights.end()) {
                    y = y_it->second;
                } else {
                    y = y_pos;
                    y_pos -= height;
                    pid_heights[trace->pid.value()] = y;
                }
                auto contents = trace->to_string();
                auto text_box = s->addText(contents, render_font);
                text_box->setPos(x_pos, y);
                auto rect = text_box->boundingRect();
                rect.moveTo(x_pos, y);
                s->addRect(rect);
                x_pos += 2.0*MARGIN + text_box->boundingRect().width();
            }
        } else {
            qDebug()<<"Unexpected event type";
        }
    }
    auto scene_height = s->height();
    for(const auto& conf: configs) {
        s->addLine(conf.first, scene_height, conf.first, -scene_height);
        auto text_box = s->addText(conf.second, render_font);
        text_box->setPos(conf.first - fm.height(), scene_height);
    }
    for(const auto& bin: binaries) {

        s->addLine(bin.first, scene_height, bin.first, -scene_height);
        auto text_box = s->addText(bin.second.path, render_font);
        text_box->setPos(bin.first - fm.height(), scene_height);
    }
}
