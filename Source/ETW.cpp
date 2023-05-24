/*
  ==============================================================================

    ETW.cpp
    Created: 23 May 2023 2:43:02pm
    Author:  Matt

  ==============================================================================
*/

#include <windows.h>
#include <TraceLoggingProvider.h>
#include <evntrace.h>
#include "ETW.h"

TRACELOGGING_DEFINE_PROVIDER(globalTraceLoggingProvider,
    "Direct2DDemoPlugin",
    // {6A612E78-284D-4DDB-877A-5F521EB33132}
    (0x6a612e78, 0x284d, 0x4ddb, 0x87, 0x7a, 0x5f, 0x52, 0x1e, 0xb3, 0x31, 0x32));

struct ETW::Pimpl
{
    Pimpl()
    {
        auto hr = TraceLoggingRegister(globalTraceLoggingProvider);
        jassert(SUCCEEDED(hr));
    }

    ~Pimpl()
    {
        TraceLoggingUnregister(globalTraceLoggingProvider);
    }
};

ETW::ETW() :
    pimpl(std::make_unique<Pimpl>())
{
}

ETW::~ETW()
{
}

void ETW::log()
{
   TraceLoggingWrite(
        globalTraceLoggingProvider,
        "Direct2DDemoPlugin",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingKeyword(5),
       TraceLoggingValue(count));

   ++count;
}
