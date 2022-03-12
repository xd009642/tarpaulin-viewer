#include "tarpaulinviewer.h"
#include "./ui_tarpaulinviewer.h"
#include <QPushButton>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValueRef>
#include <QDebug>
#include "types.h"

TraceEvent json_to_trace(const QJsonObject obj, const QDir& root) {
    TraceEvent event;
    auto pid = obj.find("pid");
    if(pid != obj.end() && !pid->isNull()) {
        event.pid = pid->toInt();
    }
    auto child = obj.find("child");
    if(child != obj.end() && !child->isNull()) {
        event.child = child->toInt();
    }
    auto signal = obj.find("signal");
    if(signal != obj.end() && !signal->isNull()) {
        event.signal = str_to_sig(signal->toString());
    }
    auto addr = obj.find("addr");
    if(addr != obj.end() && !addr->isNull()) {
        event.addr = (uint64_t)addr->toDouble();
        auto location = obj.find("location");
        if(location != obj.end() && !location->isNull()) {
            auto loc = location->toObject();
            // if we have a location we have a file and a line
            auto file = root.relativeFilePath(loc.find("file")->toString());
            auto line = (int)loc.find("line")->toDouble();
            event.file = file;
            event.line = line;
        }
    }
    auto ret = obj.find("return_val");
    if(ret != obj.end() && !ret->isNull()) {
        event.ret = ret->toInt();
    }
    event.description = obj.find("description")->toString();
    return event;
}

TestBinary json_to_bin(const QJsonObject obj, const QDir& root) {
    TestBinary bin;
    QString file = root.relativeFilePath(obj.find("path")->toString());
    if(!file.isEmpty()) {
        file = file.split("/").last();
    }
    bin.path = file;
    bin.should_panic = obj.find("should_panic")->toBool();
    auto ty = obj.find("ty");
    if(ty != obj.end()) {
        QString tyname = ty->toString();
        RunType type = RunType::Tests;
        if(tyname == "Doctests") {
            type = RunType::Doctests;
        } else if(tyname == "Benchmarks") {
            type = RunType::Benchmarks;
        } else if(tyname == "Examples") {
            type = RunType::Examples;
        } else if(tyname == "Lib") {
            type = RunType::Lib;
        } else if(tyname == "Bins") {
            type = RunType::Bins;
        } else if(tyname == "AllTargets") {
            type = RunType::AllTargets;
        }
        bin.ty = type;
    }
    auto dir = obj.find("cargo_dir");
    if(dir != obj.end()) {
        bin.cargo_dir = dir->toString();
    }
    auto name = obj.find("pkg_name");
    if(name != obj.end()) {
        bin.pkg_name = name->toString();
    }
    return bin;
}

TarpaulinViewer::TarpaulinViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TarpaulinViewer)
{
    ui->setupUi(this);
    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);

    connect(ui->reset, &QPushButton::pressed, ui->graphicsView, &graphics_view::reset);
    connect(ui->load, &QPushButton::pressed, this, &TarpaulinViewer::load_traces);
}

TarpaulinViewer::~TarpaulinViewer()
{
    delete ui;
}

void TarpaulinViewer::load_traces() {
    auto trace_file = QFileDialog::getOpenFileName(this, "Load traces", QString(), "Traces (*.json)");
    QFile input(trace_file);
    input.open(QIODevice::ReadOnly | QIODevice::Text);
    QByteArray data = input.readAll();
    input.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if(doc.isNull()) {
        qDebug() << "Parsing failed";
    }
    QJsonObject root = doc.object();
    QDir root_path;
    QJsonArray paths = root.value("manifest_paths").toArray();
    for(const QJsonValue& root: paths) {
        auto tmp = QDir(root.toString());
        if(root_path == QDir::currentPath() || tmp.count() < root_path.count()) {
            root_path = tmp;
        }
    }
    QJsonArray events = root.value("events").toArray();
    std::vector<std::shared_ptr<Event>> parsed_events;
    for(const QJsonValue& event: events) {
        // Event is either: ConfigLaunch, BinaryLaunch, Trace
        auto obj = event.toObject();
        for(auto entry = obj.begin(); entry!=obj.end(); ++entry) {
            if(entry.key() == "ConfigLaunch") {
                auto conf = Config { entry.value().toString()};
                parsed_events.push_back(std::make_shared<Event>(conf));
            } else if(entry.key() == "BinaryLaunch") {
                TestBinary bin = json_to_bin(entry.value().toObject(), root_path);
                parsed_events.push_back(std::make_shared<Event>(bin));
            } else if(entry.key() == "Trace") {
                TraceEvent event = json_to_trace(entry.value().toObject(), root_path);
                parsed_events.push_back(std::make_shared<Event>(event));
            } else if(entry.key() == "Marker") {
                Marker m{};
                parsed_events.push_back(std::make_shared<Event>(m));
            }
        }
    }
    qDebug()<<parsed_events.size()<<" events found";
    ui->graphicsView->create_scene(parsed_events);
}

void TarpaulinViewer::keyReleaseEvent(QKeyEvent* event)
{
    switch(event->key()) {
    case Qt::Key_F: {
        ui->graphicsView->next_failure();
        break;
    }
    case Qt::Key_Left: {
        if(event->modifiers()==Qt::ControlModifier) {
            ui->graphicsView->move_pid_left();
        } else {
            ui->graphicsView->move_left();
        }
        break;
    }
    case Qt::Key_Right: {
        if(event->modifiers()==Qt::ControlModifier) {
            ui->graphicsView->move_pid_right();
        } else {
            ui->graphicsView->move_right();
        }
        break;
    }
    case Qt::Key_Up: {
        ui->graphicsView->pan(0.0, -5.0);
        break;
    }
    case Qt::Key_Down: {
        ui->graphicsView->pan(0.0, 5.0);
        break;
    }
    case Qt::Key_Plus: {
        ui->graphicsView->apply_zoom(1.25);
        break;
    }
    case Qt::Key_Minus: {
        ui->graphicsView->apply_zoom(0.75);
        break;
    }
    }
}
