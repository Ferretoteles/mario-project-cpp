#include "EventSystem.hpp"

void EventSystem::subscribe(EventType type, Listener listener)
{
    m_listeners[static_cast<int>(type)].push_back(std::move(listener));
}

void EventSystem::publish(const GameEvent& event)
{
    const auto found = m_listeners.find(static_cast<int>(event.type));
    if (found == m_listeners.end())
        return;

    for (const auto& listener : found->second)
        listener(event);
}
