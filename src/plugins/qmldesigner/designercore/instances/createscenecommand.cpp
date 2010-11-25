#include "createscenecommand.h"

namespace QmlDesigner {

CreateSceneCommand::CreateSceneCommand()
{
}

QDataStream &operator<<(QDataStream &out, const CreateSceneCommand &/*command*/)
{
    return out;
}

QDataStream &operator>>(QDataStream &in, CreateSceneCommand &/*command*/)
{
    return in;
}

}
