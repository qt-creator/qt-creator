#ifndef QMLJSDEBUGCLIENTCONSTANTS_H
#define QMLJSDEBUGCLIENTCONSTANTS_H

namespace QmlJsDebugClient {
namespace Constants {

const char STR_WAITING_FOR_CONNECTION[] = "Waiting for connection ";
const char STR_UNABLE_TO_LISTEN[] = "Unable to listen ";
const char STR_IGNORING_DEBUGGER[] = "Ignoring \"-qmljsdebugger=";
const char STR_IGNORING_DEBUGGER2[] = "Ignoring\"-qmljsdebugger="; // There is (was?) a bug in one of the error strings - safest to handle both
const char STR_CONNECTION_ESTABLISHED[] = "Connection established";

} // namespace Constants
} // namespace QmlJsDebugClient

#endif // QMLJSDEBUGCLIENTCONSTANTS_H
