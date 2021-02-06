#ifndef TYPES_H
#define TYPES_H

#include <QString>
#include <memory>
#include <optional>
#include <variant>
#include <QDebug>

enum class Signal {
    sighup,
    sigint,
    sigquit,
    sigabrt,
    sigfpe,
    sigkill,
    sigill,
    sigsegv,
    sigpipe,
    sigalrm,
    sigterm,
    sigusr1,
    sigusr2,
    sigchld,
    sigcont,
    sigstop,
    sigtstp,
    sigttin,
    sigttou,
    unknown
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

using Event = std::variant<Config, TestBinary, TraceEvent>;


std::optional<uint64_t> get_child(std::shared_ptr<Event> event);

std::optional<uint64_t> get_pid(std::shared_ptr<Event> event);

bool is_end_node(std::shared_ptr<Event> event);

#endif // TYPES_H
