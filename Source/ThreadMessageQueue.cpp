#include "ThreadMessageQueue.h"

ThreadMessageQueue::ThreadMessageQueue(juce::CriticalSection& lock_) :
    lock(lock_)
{
}

void ThreadMessageQueue::postMessage(ThreadMessage::Callback callback_)
{
    auto message = std::make_unique<ThreadMessage>(callback_);

    juce::ScopedLock scopedLock(lock);
    messages.add(message.release());
}

void ThreadMessageQueue::dispatchNextMessage()
{
    juce::ScopedTryLock tryLocker{ lock };

    if (tryLocker.isLocked())
    {
        if (auto message = messages.getFirst())
        {
            if (message->callback)
            {
                message->callback();
            }

            messages.remove(0);
        }
    }
}
