/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "ne7sshobject.h"

#include <QtCore/QMutexLocker>

#include <ne7ssh.h>

namespace Core {
namespace Internal {

Ne7SshObject *Ne7SshObject::instance()
{
    if (!m_instance)
        m_instance = new Ne7SshObject;
    return m_instance;
}

void Ne7SshObject::removeInstance()
{
    delete m_instance;
}

Ne7SshObject::Ptr Ne7SshObject::get()
{
    QMutexLocker locker(&m_mutex);
    QSharedPointer<ne7ssh> shared = m_weakRef.toStrongRef();
    if (!shared) {
        shared = QSharedPointer<ne7ssh>(new ne7ssh);
        m_weakRef = shared;
    }
    return shared;
}

Ne7SshObject::Ne7SshObject()
{
}

Ne7SshObject *Ne7SshObject::m_instance = 0;

} // namespace Internal
} // namespace Core
