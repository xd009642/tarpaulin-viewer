#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <memory>
#include <optional>
#include <variant>
#include <QDebug>
#include <QColor>

enum class Signal {
    sigtstp,
    sighup,
    sigint,
    sigquit,
    sigabrt,
    sigfpe,
    sigsegv,
    sigkill,
    sigill,
    sigpipe,
    sigcont,
    sigalrm,
    sigusr1,
    unknown,
    sigusr2,
    sigchld,
    sigttin,
    sigstop,
    sigttou,
    sigterm,
    _length
};

Signal str_to_sig(const QString& s);

QString sig_to_str(Signal s);

enum class RunType {
    Tests,
    Doctests,
    Benchmarks,
    Examples,
    Lib,
    Bins,
    AllTargets,
    _length
};

constexpr size_t num_colours() {
    // 2 extras are for nullopt_t
    return static_cast<size_t>(RunType::_length) + static_cast<size_t>(Signal::_length) + 2;
}

struct Config {
    QString name;
};

struct Marker {

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
    std::optional<Signal> signal;
    std::optional<uint64_t> addr;
    std::optional<uint64_t> ret;
    std::optional<QString> file;
    std::optional<int> line;
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
            if(*sig != Signal::unknown) {
                contents.append(sig_to_str(*sig));
                contents.append('\n');
            }
        }
        if(auto a = addr) {
            contents.append(QString("addr: %1\n").arg((uint64_t)*a));
        }
        if(auto f = file) {
            auto l = line.value_or(0);
            contents.append(QString("%1:%2\n").arg(*f).arg(l));
        }
        if(auto r = ret) {
            contents.append(QString("return: %1\n").arg((int)*r));
        }
        contents.append(description);
        return contents;
    }

    bool is_bad() const {
        bool bad = false;
        if(auto r = ret) {
            bad |= (*r != 0);
        }
        if(bad) {
            return bad;
        }
        if(auto s = signal) {
            bad |= (*s == Signal::sigsegv) | (*s == Signal::sigill);
        }
        return bad;
    }
};

using Event = std::variant<Config, TestBinary, TraceEvent, Marker>;


std::optional<uint64_t> get_child(std::shared_ptr<Event> event);

std::optional<uint64_t> get_pid(std::shared_ptr<Event> event);

QColor get_node_colour(std::shared_ptr<Event> event);

bool is_end_node(std::shared_ptr<Event> event);

bool is_marker(std::shared_ptr<Event> event);

#endif // TYPES_H
