/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PIXMAPCHANGEDCOMMAND_H
#define PIXMAPCHANGEDCOMMAND_H

#include <QMetaType>
#include "imagecontainer.h"

namespace QmlDesigner {

class PixmapChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command);
public:
    PixmapChangedCommand();
    PixmapChangedCommand(const QVector<ImageContainer> &imageVector);

    QVector<ImageContainer> images() const;

private:
    QVector<ImageContainer> m_imageVector;
};

QDataStream &operator<<(QDataStream &out, const PixmapChangedCommand &command);
QDataStream &operator>>(QDataStream &in, PixmapChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PixmapChangedCommand)

#endif // PIXMAPCHANGEDCOMMAND_H
