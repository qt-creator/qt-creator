/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include <QtCore/QBuffer>
#include <QtCore/QDataStream>

#include "copyhelper.h"
#include "internalnode_p.h"

namespace QmlDesigner {
namespace Internal {

CopyHelper::CopyHelper(const QString &source):
        m_source(source)
{
}

QMimeData *CopyHelper::createMimeData(const QList<InternalNodeState::Pointer> &nodeStates)
{
    if (nodeStates.isEmpty())
        return 0;

    m_locations.clear();

    foreach (const InternalNodeState::Pointer &nodeState, nodeStates) {
        collectLocations(nodeState);
    }

    if (m_locations.isEmpty())
        return 0;

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/x-qt-bauhaus", contentsAsByteArray());

    return mimeData;
}

void CopyHelper::collectLocations(const InternalNodeState::Pointer &nodeState)
{
    // TODO: copy way more here.
    m_locations.insert(nodeState->modelNode()->baseNodeState()->location());
}

QByteArray CopyHelper::contentsAsByteArray() const
{
    QBuffer result;
    result.open(QIODevice::WriteOnly);
    QDataStream os(&result);

    os << m_locations.size();
    foreach (const TextLocation &location, m_locations) {
        os << location.position();
        os << location.length();
    }

    os << m_source;

    result.close();
    return result.buffer();
}

} // namespace Internal
} // namespace QmlDesigner
