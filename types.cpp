#include <types.h>
#include <variant>
#include <QDebug>


Signal str_to_sig(const QString& s) {
    if(s=="SIGHUP") {
        return Signal::sighup;
    } else if(s=="SIGINT") {
        return Signal::sigint;
    } else if(s=="SIGILL") {
        return Signal::sigill;
    } else if(s=="SIGQUIT") {
        return Signal::sigquit;
    } else if(s=="SIGABRT") {
        return Signal::sigabrt;
    } else if(s=="SIGFPE") {
        return Signal::sigfpe;
    } else if(s=="SIGKILL") {
        return Signal::sigkill;
    } else if(s=="SIGSEGV") {
        return Signal::sigsegv;
    } else if(s=="SIGPIPE") {
        return Signal::sigpipe;
    } else if(s=="SIGALRM") {
        return Signal::sigalrm;
    } else if(s=="SIGTERM") {
        return Signal::sigterm;
    } else if(s=="SIGUSR1") {
        return Signal::sigusr1;
    } else if(s=="SIGUSR2") {
        return Signal::sigusr2;
    } else if(s=="SIGCHLD") {
        return Signal::sigchld;
    } else if(s=="SIGCONT") {
        return Signal::sigcont;
    } else if(s=="SIGSTOP") {
        return Signal::sigstop;
    } else if(s=="SIGTSTP") {
        return Signal::sigtstp;
    } else if(s=="SIGTTIN") {
        return Signal::sigttin;
    } else if(s=="SIGTTOU") {
        return Signal::sigttou;
    }
    return Signal::unknown;
}

QString sig_to_str(Signal s) {
    if(s==Signal::sighup) {
        return "SIGHUP";
    } else if(s==Signal::sigint) {
        return "SIGINT";
    } else if(s==Signal::sigill) {
        return "SIGILL";
    } else if(s==Signal::sigquit) {
        return "SIGQUIT";
    } else if(s==Signal::sigabrt) {
        return "SIGABRT";
    } else if(s==Signal::sigfpe) {
        return "SIGFPE";
    } else if(s==Signal::sigkill) {
        return "SIGKILL";
    } else if(s==Signal::sigsegv) {
        return "SIGSEGV";
    } else if(s==Signal::sigpipe) {
        return "SIGPIPE";
    } else if(s==Signal::sigalrm) {
        return "SIGALRM";
    } else if(s==Signal::sigterm) {
        return "SIGTERM";
    } else if(s==Signal::sigusr1) {
        return "SIGUSR1";
    } else if(s==Signal::sigusr2) {
        return "SIGUSR2";
    } else if(s==Signal::sigchld) {
        return "SIGCHLD";
    } else if(s==Signal::sigcont) {
        return "SIGCONT";
    } else if(s==Signal::sigstop) {
        return "SIGSTOP";
    } else if(s==Signal::sigtstp) {
        return "SIGTSTP";
    } else if(s==Signal::sigttin) {
        return "SIGTTIN";
    } else if(s==Signal::sigttou) {
        return "SIGTTOU";
    }
    return "UNKNOWN";
}

std::optional<uint64_t> get_pid(std::shared_ptr<Event> event) {
    if(event) {
        if(auto trace = std::get_if<TraceEvent>(event.get())) {
            return trace->pid;
        }
    }
    return std::nullopt;
}

std::optional<uint64_t> get_child(std::shared_ptr<Event> event) {
    if(event) {
        if(auto trace = std::get_if<TraceEvent>(event.get())) {
            return trace->child;
        }
    }
    return std::nullopt;
}

bool is_end_node(std::shared_ptr<Event> event) {
    if(event) {
        if(auto trace = std::get_if<TraceEvent>(event.get())) {
            return trace->ret.has_value();
        }
    }
    return true;

}

float generate_hue(int ind, int length, float min_colour, float max_colour) {
    auto index = static_cast<float>(ind);
    auto total =  static_cast<float>(length);

    return static_cast<int>((index/total) * (max_colour - min_colour) + min_colour);
}


QColor get_node_colour(std::shared_ptr<Event> event) {
    auto result = QColor();
    if(event) {
        if(auto trace = std::get_if<TraceEvent>(event.get())) {
            auto index = 0;
            auto total_traces = static_cast<int>(Signal::_length) + 2;
            if (auto signal = trace->signal) {
                index = static_cast<int>(signal.value()) + 1;
            } else if(trace->ret) {
                index = total_traces;
            }
            int hue = generate_hue(index, total_traces, 55.0, 355.0);
            result = QColor::fromHsv(hue, 127, 255);
        } else if (auto bin = std::get_if<TestBinary>(event.get())) {
            auto index = static_cast<int>(RunType::_length);
            auto total_traces =  index + 1;
            if (auto signal = bin->ty) {
                index = static_cast<int>(signal.value());
            }
            int hue = generate_hue(index, total_traces, 0, 55.0);
            result = QColor::fromHsv(hue, 127, 255);
        }
    }
    return result;
}


bool is_marker(std::shared_ptr<Event> event) {
    if(event) {
        return event->index() == 3;
    } else {
        return false;
    }
}
