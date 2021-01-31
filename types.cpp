#include <types.h>
#include <variant>


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

