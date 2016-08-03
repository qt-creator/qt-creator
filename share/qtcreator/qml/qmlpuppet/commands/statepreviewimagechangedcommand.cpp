/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "statepreviewimagechangedcommand.h"

#include <QDebug>

#include <algorithm>

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
