#ifndef __MSVCDEBUGOUTPUT_H__
#define __MSVCDEBUGOUTPUT_H__

namespace Debugger {
namespace Internal {

class MSVCDebugEngine;

class MSVCDebugOutput : public IDebugOutputCallbacks
{
public:
    MSVCDebugOutput(MSVCDebugEngine* engine)
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
    MSVCDebugEngine* m_pEngine;
};

} // namespace Internal
} // namespace Debugger

#endif // #ifndef __MSVCDEBUGOUTPUT_H__
