#ifndef CREATESCENECOMMAND_H
#define CREATESCENECOMMAND_H

#include <qmetatype.h>

namespace QmlDesigner {

class CreateSceneCommand
{
    friend QDataStream &operator>>(QDataStream &in, CreateSceneCommand &command);

public:
    CreateSceneCommand();
};

QDataStream &operator<<(QDataStream &out, const CreateSceneCommand &command);
QDataStream &operator>>(QDataStream &in, CreateSceneCommand &command);

}

Q_DECLARE_METATYPE(QmlDesigner::CreateSceneCommand)

#endif // CREATESCENECOMMAND_H
