#pragma once

#include <JuceHeader.h>

class ThreadMessage
{
public:
    using Callback = std::function<void()>;

    ThreadMessage(Callback callback_) :
        callback(callback_)
    {
    }
    ~ThreadMessage() = default;
    
    Callback callback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreadMessage);
};

class ThreadMessageQueue
{
public:
    ThreadMessageQueue(juce::CriticalSection& lock_);
    ~ThreadMessageQueue() = default;

    void postMessage(ThreadMessage::Callback callback_);
    void dispatchNextMessage();

private:
    juce::CriticalSection& lock;
    juce::OwnedArray<ThreadMessage> messages;
};
