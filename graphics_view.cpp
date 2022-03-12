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
    qreal MARGIN = 10.0;
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
    auto meta_y = 3.0*MARGIN+lane_height;
    std::vector<qreal> x_positions;
    while(!queue.empty()) {
        x_positions.push_back(xpos);
        std::shared_ptr<Node> node = queue.top();
        event_indexes[node->view] = node->event_index;
        queue.pop();
        auto pid_opt = get_pid(node->event);
        if(node->event_index != last_index + 1) {
            qDebug()<<"Index assumption has broken! "<<node->event_index<<":"<<last_index+1;
            break;
        }

        auto rect = node->view->boundingRect();
        auto brush = QBrush(node->colour);
        if(auto trace = std::get_if<TraceEvent>(node->event.get())) {
            if(!pid_opt) {
                node->view->setX(xpos);
                x_gridlines.push_back(xpos + rect.width()/2.0);
                node->view->setY(meta_y);
            } else {
                auto pid = pid_opt.value();
                if(pid_heights.find(pid) == pid_heights.end()) {
                    // Work out height and insert
                    pid_heights[pid] = pid_heights.size() * -lane_height;
                }
                auto ypos = pid_heights[pid];
                node->view->setY(ypos);
                node->view->setX(xpos);
                auto rect = s->addRect(node->view->sceneBoundingRect(), QPen(), brush);
                if(trace->is_bad()) {
                    rect->setBrush(QColor(255, 0, 0, 90));
                    bad_nodes.push_back(node);
                }
                // EDGES
                if(auto parent = node->parent.lock()) {

                    auto left_connector = node->view->sceneBoundingRect().topLeft();
                    auto parent_rect = parent->view->sceneBoundingRect();
                    auto right_connector = parent_rect.topRight();
                    s->addLine({left_connector, right_connector});
                }
            }
        } else {
            node->view->setX(xpos);
            x_gridlines.push_back(xpos + rect.width()/2.0);
            node->view->setY(meta_y);
        }
        xpos += rect.width() + MARGIN;
        last_index = node->event_index;
        for(const auto& child: node->children) {
            if(auto node = child.lock()) {
                queue.push(node);
            }
        }
    }

    for(size_t index: markers) {
        auto x = x_positions[index] - MARGIN/2.0;
        auto parent_rect = s->sceneRect();
        auto top_connector = parent_rect.topRight();
        top_connector.setX(x);
        auto bottom_connector = parent_rect.bottomRight();
        bottom_connector.setX(top_connector.x());

        QPen marker_pen;
        marker_pen.setStyle(Qt::DashLine);
        marker_pen.setColor(QColor(0, 0, 0, 200));

        s->addLine({top_connector, bottom_connector}, marker_pen);
    }

    update();
}

void graphics_view::create_scene(const std::vector<std::shared_ptr<Event>>& events) {
    QGraphicsScene* s = scene();
    s->clear();
    markers.clear();
    nodes.clear();
    event_indexes.clear();
    QFontMetrics fm(render_font);
    std::set<uint64_t> pid_set;
    pid_set.insert(0);
    size_t index = 0;
    for(const auto& event: events) {
        if(!event) {
            continue;
        }
        auto colour = get_node_colour(event);
        if (is_marker(event)) {
            markers.insert(index);
            continue;
        } else if(auto conf = std::get_if<Config>(event.get())) {
            auto text_box = s->addText(conf->name, render_font);
            text_box->setZValue(1);
            auto node = std::make_shared<Node>(index, text_box, event);
            node->colour = colour;
            if(!nodes.empty()) {
                node->parent = nodes.back();
                nodes.back()->children.push_back(node);
            }
            nodes.push_back(node);
        } else if(auto bin = std::get_if<TestBinary>(event.get())) {
            auto text_box = s->addText(bin->path, render_font);
            text_box->setZValue(1);
            auto node = std::make_shared<Node>(index, text_box, event);
            node->colour = colour;
            if(!nodes.empty()) {
                node->parent = nodes.back();
                nodes.back()->children.push_back(node);
            }
            nodes.push_back(node);
        } else if(auto trace = std::get_if<TraceEvent>(event.get())) {
            auto contents = trace->to_string();
            auto text_box = s->addText(contents, render_font);
            text_box->setZValue(1);
            auto node = std::make_shared<Node>(index, text_box, event);
            node->colour = colour;
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

void graphics_view::deselect() {
    if(selected_node && !nodes.empty()) {
        auto value = selected_node.value();
        auto centre = nodes[value]->view->sceneBoundingRect().center().toPoint();
        auto old_items = items(mapFromScene(centre));
        for(auto& item: old_items) {
            // This is the outline... Change the color ideallly
            QGraphicsRectItem* rect = qgraphicsitem_cast<QGraphicsRectItem*>(item);
            if(rect) {
                rect->setPen(QPen(Qt::black));
            }
        }
    }
    selected_node = std::nullopt;
}

void graphics_view::highlight_selected() {
    if(selected_node && !nodes.empty()) {
        auto value = selected_node.value();
        auto centre = nodes[value]->view->sceneBoundingRect().center().toPoint();
        auto old_items = items(mapFromScene(centre));
        for(auto& item: old_items) {
            // This is the outline... Change the color ideallly
            QGraphicsRectItem* rect = qgraphicsitem_cast<QGraphicsRectItem*>(item);
            if(rect) {
                rect->setPen(QPen(Qt::red));
            }
        }
    }
}

void graphics_view::mousePressEvent(QMouseEvent *event) {
    auto graphics_items = items(event->pos());
    deselect();
    for(auto& item: graphics_items) {
        if(event_indexes.find(item) != event_indexes.end()) {
            selected_node = event_indexes[item];
        } else {
            // This is the outline... Change the color ideallly
            QGraphicsRectItem* rect = qgraphicsitem_cast<QGraphicsRectItem*>(item);
            if(rect) {
                rect->setPen(QPen(Qt::red));
            }
        }
    }
}

// Todo be less lazy with move_left move_right

void graphics_view::move_left() {
    if(auto index = selected_node) {
        if(*index > 0 && !nodes.empty()) {
            deselect();
            selected_node = *index - 1;
            centerOn(nodes[*index-1]->view);
            highlight_selected();
        }
    } else {
        pan(-5.0, 0.0);
    }
    update();
}

void graphics_view::move_right() {
    if(auto index = selected_node) {
        if(*index + 1 < nodes.size()) {
            deselect();
            selected_node = *index + 1;
            centerOn(nodes[*index+1]->view);
            highlight_selected();
        }
    } else {
        pan(5.0, 0.0);
    }
    update();
}

void graphics_view::move_pid_left() {
    if(auto index = selected_node) {
        auto node = nodes[*index];
        deselect();
        if(auto parent = node->parent.lock()) {
            selected_node = parent->event_index;
        }
        if(auto index = selected_node) {
            centerOn(nodes[*index]->view);
        }
        highlight_selected();
        update();
    }
}

void graphics_view::move_pid_right() {
    if(auto index = selected_node) {
        auto node = nodes[*index];
        deselect();
        auto pid = get_pid(node->event);
        std::optional<size_t> new_index = std::nullopt;
        for(const auto& child_ref: node->children) {
            if(auto child = child_ref.lock()) {
                auto child_pid = get_pid(node->event);
                if(child_pid == pid) {
                    new_index = child->event_index;
                    break;
                } else if(auto current = new_index) {
                    if(child->event_index < *current) {
                        new_index = child->event_index;
                    }
                } else {
                    new_index = child->event_index;
                }
            }
        }
        selected_node = new_index;
        if(auto index = selected_node) {
            centerOn(nodes[*index]->view);
        }
        highlight_selected();
        update();
    }
}


void graphics_view::next_failure() {
    std::shared_ptr<Node> first;
    qDebug()<<"Bad nodes "<<bad_nodes.size();
    for(int i=0; i<bad_nodes.size(); i++) {
        if(auto node = bad_nodes[i].lock()) {
            if(!first) {
                first = node;
            }
            // Check if x positon is beyond the current view if so jump to it
            auto current_centre = mapToScene(rect().center());
            auto node_pos = node->view->scenePos();
            if(node_pos.x() > current_centre.x()) {

                deselect();
                selected_node = node->event_index;
                centerOn(node->view);
                highlight_selected();
            }
        } else {
            bad_nodes.erase(bad_nodes.begin() + i);
            i--;
        }
    }
}
