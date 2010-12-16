#ifndef CLEARSCENECOMMAND_H
#define CLEARSCENECOMMAND_H

#include <qmetatype.h>

namespace QmlDesigner {

class ClearSceneCommand
{
public:
    ClearSceneCommand();
};

QDataStream &operator<<(QDataStream &out, const ClearSceneCommand &command);
QDataStream &operator>>(QDataStream &in, ClearSceneCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ClearSceneCommand)

#endif // CLEARSCENECOMMAND_H
