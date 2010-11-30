#include "statepreviewimagechangedcommand.h"

namespace QmlDesigner {

StatePreviewImageChangedCommand::StatePreviewImageChangedCommand()
{
}

StatePreviewImageChangedCommand::StatePreviewImageChangedCommand(const QVector<ImageContainer> &imageVector)
    : m_previewVector(imageVector)
{
}

QVector<ImageContainer> StatePreviewImageChangedCommand::previews()const
{
    return m_previewVector;
}

QDataStream &operator<<(QDataStream &out, const StatePreviewImageChangedCommand &command)
{
    out << command.previews();

    return out;
}

QDataStream &operator>>(QDataStream &in, StatePreviewImageChangedCommand &command)
{
    in >> command.m_previewVector;

    return in;
}
} // namespace QmlDesigner
