#include "clearscenecommand.h"

namespace QmlDesigner {

ClearSceneCommand::ClearSceneCommand()
{
}

QDataStream &operator<<(QDataStream &out, const ClearSceneCommand &/*command*/)
{
    return out;
}

QDataStream &operator>>(QDataStream &in, ClearSceneCommand &/*command*/)
{
    return in;
}

} // namespace QmlDesigner
