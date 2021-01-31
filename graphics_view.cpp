#include "graphics_view.h"
#include <QScrollBar>
#include <QTouchEvent>
#include <QDebug>
#include <QGraphicsTextItem>
#include <QFontMetrics>
#include <QFont>
#include <set>
#include <queue>
#include <map>

bool node_compare(const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b) {
    return a->event_index > b->event_index;
}

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

void graphics_view::layout_scene() {
    using node_queue = std::priority_queue<std::shared_ptr<Node>, std::vector<std::shared_ptr<Node>>, decltype(&node_compare)>;
    if(nodes.empty()) {
        return;
    }
    QGraphicsScene* s = scene();
    std::map<int, qreal> pid_heights;
    qreal MARGIN = 30.0;
    qreal xpos = MARGIN;
    std::vector<qreal> x_gridlines;
    node_queue queue(node_compare);
    queue.push(nodes.front());
    int last_index = -1;
    qreal lane_height = MARGIN*2.0 + 50.0;
    for(const auto& node: nodes) {
        qreal candidate = node->view->boundingRect().height() + MARGIN*2.0;
        if(candidate > lane_height) {
            lane_height = candidate;
        }
    }
    qDebug()<<"Lane height: "<<lane_height;
    while(!queue.empty()) {
        std::shared_ptr<Node> node = queue.top();
        queue.pop();

        if(node->event_index != last_index + 1) {
            qDebug()<<"Index assumption has broken! "<<node->event_index<<":"<<last_index+1;
            break;
        }
        auto rect = node->view->boundingRect();
        if(auto trace = std::get_if<TraceEvent>(node->event.get())) {
            auto pid = get_pid(node->event).value_or(0);
            if(pid_heights.find(pid) == pid_heights.end()) {
                // Work out height and insert
                pid_heights[pid] = pid_heights.size() * -lane_height;
            }
            auto ypos = pid_heights[pid];
            node->view->setY(ypos + rect.height()/2.0);
            node->view->setX(xpos);
            // EDGES
        } else {
            node->view->setX(xpos);
            x_gridlines.push_back(xpos + rect.width()/2.0);
            node->view->setY(MARGIN+lane_height);
        }
        xpos += rect.width() + MARGIN;
        last_index = node->event_index;
        for(const auto& child: node->children) {
            if(auto node = child.lock()) {
                queue.push(node);
            }
        }
    }
    update();
}

void graphics_view::create_scene(const std::vector<std::shared_ptr<Event>>& events) {
    QGraphicsScene* s = scene();
    s->clear();
    nodes.clear();
    QFontMetrics fm(render_font);
    std::set<uint64_t> pid_set;
    pid_set.insert(0);
    size_t index = 0;
    for(const auto& event: events) {
        if(!event) {
            continue;
        }
        if(auto conf = std::get_if<Config>(event.get())) {
            auto text_box = s->addText(conf->name, render_font);
            auto node = std::make_shared<Node>(index, text_box, event);
            if(!nodes.empty()) {
                node->parent = nodes.back();
                nodes.back()->children.push_back(node);
            }
            nodes.push_back(node);
        } else if(auto bin = std::get_if<TestBinary>(event.get())) {
            auto text_box = s->addText(bin->path, render_font);
            auto node = std::make_shared<Node>(index, text_box, event);
            if(!nodes.empty()) {
                node->parent = nodes.back();
                nodes.back()->children.push_back(node);
            }
            nodes.push_back(node);
        } else if(auto trace = std::get_if<TraceEvent>(event.get())) {
            auto contents = trace->to_string();
            auto text_box = s->addText(contents, render_font);
            auto node = std::make_shared<Node>(index, text_box, event);
            pid_set.insert(trace->pid.value_or(0));
            for(auto it=nodes.rbegin(); it!=nodes.rend(); ++it) {
                auto pid = get_pid((*it)->event);
                auto child = get_child((*it)->event);
                // So this trace is either a child of another trace or a continuation of a running thread
                // Assuming each trace can only have one parent but can have multiple children - might be incorrect for tests that use wait syscall
                if(!is_end_node((*it)->event) && ((pid && trace->pid == pid) || (child && trace->pid == child) )) {
                    (*it)->children.push_back(node);
                    node->parent = *it;
                    break;
                }
            }
            if(!nodes.empty() && !node->parent.lock()) {
                node->parent = nodes.back();
                nodes.back()->children.push_back(node);
            }
            nodes.push_back(node);
        } else {
            qDebug()<<"Unexpected event type";
        }
        pid_count = pid_set.size();
        index += 1;
    }
    layout_scene();
}
