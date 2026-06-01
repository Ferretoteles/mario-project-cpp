#pragma once

#include "Core.hpp"

class EventSystem {
public:
    using Listener = std::function<void(const GameEvent&)>;

    void subscribe(EventType type, Listener listener);
    void publish(const GameEvent& event);

private:
    std::unordered_map<int, std::vector<Listener>> m_listeners;
};
