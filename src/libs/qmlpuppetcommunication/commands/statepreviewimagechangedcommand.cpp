// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "statepreviewimagechangedcommand.h"

#include <QDebug>

#include <algorithm>

namespace QmlDesigner {

StatePreviewImageChangedCommand::StatePreviewImageChangedCommand() = default;

StatePreviewImageChangedCommand::StatePreviewImageChangedCommand(const QVector<ImageContainer> &imageVector)
    : m_previewVector(imageVector)
{
}

QVector<ImageContainer> StatePreviewImageChangedCommand::previews()const
{
    return m_previewVector;
}

void StatePreviewImageChangedCommand::sort()
{
    std::sort(m_previewVector.begin(), m_previewVector.end());
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

bool operator ==(const StatePreviewImageChangedCommand &first, const StatePreviewImageChangedCommand &second)
{
    return first.m_previewVector == second.m_previewVector;
}

QDebug operator <<(QDebug debug, const StatePreviewImageChangedCommand &command)
{
    return debug.nospace() << "StatePreviewImageChangedCommand(" << command.previews() << ")";

}

} // namespace QmlDesigner
