/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "drop3dlibraryitemcommand.h"

#include <QDataStream>

namespace QmlDesigner {

Drop3DLibraryItemCommand::Drop3DLibraryItemCommand(const QByteArray &itemData)
    : m_itemData(itemData)
{
}

QDataStream &operator<<(QDataStream &out, const Drop3DLibraryItemCommand &command)
{
    out << command.itemData();

    return out;
}

QDataStream &operator>>(QDataStream &in, Drop3DLibraryItemCommand &command)
{
    in >> command.m_itemData;

    return in;
}

bool operator==(const Drop3DLibraryItemCommand &first, const Drop3DLibraryItemCommand &second)
{
    return first.m_itemData == second.m_itemData;
}

} // namespace QmlDesigner
