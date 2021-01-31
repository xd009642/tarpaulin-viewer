#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <memory>
#include <optional>
#include <variant>
#include <QDebug>

enum class RunType {
    Tests,
    Doctests,
    Benchmarks,
    Examples,
    Lib,
    Bins,
    AllTargets
};

struct Config {
    QString name;
};

struct TestBinary {
    QString path;
    std::optional<RunType> ty;
    std::optional<QString> cargo_dir;
    std::optional<QString> pkg_name;
    bool should_panic;
};

struct TraceEvent {
    std::optional<uint64_t> pid;
    std::optional<uint64_t> child;
    std::optional<QString> signal;
    std::optional<uint64_t> addr;
    std::optional<uint64_t> ret;
    QString description;

    QString to_string() const {
        QString contents;
        if(auto pd = pid) {
            contents.append(QString("pid: %1\n").arg((int)*pd));
        }
        if(auto ch = child) {
            contents.append(QString("child: %1\n").arg((int)*ch));
        }
        if(auto sig = signal) {
            contents.append(*sig);
            contents.append('\n');
        }
        if(auto a = addr) {
            contents.append(QString("addr: %1\n").arg((uint64_t)*a));
        }
        if(auto r = ret) {
            contents.append(QString("return: %1\n").arg((int)*r));
        }
        contents.append(description);
        return contents;
    }
};

using Event = std::variant<Config, TestBinary, TraceEvent>;


std::optional<uint64_t> get_child(std::shared_ptr<Event> event);

std::optional<uint64_t> get_pid(std::shared_ptr<Event> event);

bool is_end_node(std::shared_ptr<Event> event);

#endif // TYPES_H
