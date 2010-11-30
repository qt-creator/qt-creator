#ifndef STATEPREVIEWIMAGECHANGEDCOMMAND_H
#define STATEPREVIEWIMAGECHANGEDCOMMAND_H

#include <QMetaType>

#include "imagecontainer.h"

namespace QmlDesigner {

class StatePreviewImageChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, StatePreviewImageChangedCommand &command);
public:
    StatePreviewImageChangedCommand();
    StatePreviewImageChangedCommand(const QVector<ImageContainer> &imageVector);

    QVector<ImageContainer> previews() const;

private:
    QVector<ImageContainer> m_previewVector;
};

QDataStream &operator<<(QDataStream &out, const StatePreviewImageChangedCommand &command);
QDataStream &operator>>(QDataStream &in, StatePreviewImageChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::StatePreviewImageChangedCommand);

#endif // STATEPREVIEWIMAGECHANGEDCOMMAND_H
