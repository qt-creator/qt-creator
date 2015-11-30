/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "pixmapchangedcommand.h"

#include <QDebug>

#include <QVarLengthArray>

namespace QmlDesigner {

PixmapChangedCommand::PixmapChangedCommand()
{
}

PixmapChangedCommand::PixmapChangedCommand(const QVector<ImageContainer> &imageVector)
    : m_imageVector(imageVector)
{
}

QVector<ImageContainer> PixmapChangedCommand::images() const
{
    return m_imageVector;
}

void PixmapChangedCommand::sort()
{
    qSort(m_imageVector);
}

QDataStream &operator<<(QDataStream &out, const PixmapChangedCommand &command)
{
    out << command.images();

    return out;
}

QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command)
{
    in >> command.m_imageVector;

    return in;
}

bool operator ==(const PixmapChangedCommand &first, const PixmapChangedCommand &second)
{
    return first.m_imageVector == second.m_imageVector;
}

QDebug operator <<(QDebug debug, const PixmapChangedCommand &command)
{
    return debug.nospace() << "PixmapChangedCommand(" << command.images() << ")";
}

} // namespace QmlDesigner
