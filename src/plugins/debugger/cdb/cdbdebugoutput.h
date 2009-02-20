#ifndef DEBUGGER_CDBOUTPUT_H
#define DEBUGGER_CDBOUTPUT_H

namespace Debugger {
namespace Internal {

class CdbDebugEngine;

class CdbDebugOutput : public IDebugOutputCallbacks
{
public:
    CdbDebugOutput(CdbDebugEngine* engine)
        : m_pEngine(engine)
    {}

    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    // IDebugOutputCallbacks.
    STDMETHOD(Output)(
        THIS_
        IN ULONG mask,
        IN PCSTR text
        );

private:
    CdbDebugEngine* m_pEngine;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_CDBOUTPUT_H
