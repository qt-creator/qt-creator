/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#ifndef MAEMOQEMURUNTIME_H
#define MAEMOQEMURUNTIME_H

#include "maemoqemusettings.h"

#include <remotelinux/portlist.h>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QString>

namespace RemoteLinux {
namespace Internal {

enum QemuStatus {
    QemuStarting,
    QemuFailedToStart,
    QemuFinished,
    QemuCrashed,
    QemuUserReason
};

struct MaemoQemuRuntime
{
    typedef QPair<QString, QString> Variable;

    MaemoQemuRuntime() {}
    MaemoQemuRuntime(const QString &root) : m_root(root) {}
    bool isValid() const {
        return !m_bin.isEmpty();
    }
    QProcessEnvironment environment() const {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        foreach (const Variable &var, m_normalVars)
            env.insert(var.first, var.second);
        QHash<MaemoQemuSettings::OpenGlMode, QString>::ConstIterator it
            = m_openGlBackendVarValues.find(MaemoQemuSettings::openGlMode());
        if (it != m_openGlBackendVarValues.constEnd())
            env.insert(m_openGlBackendVarName, it.value());
        return env;
    }

    QString m_name;
    QString m_bin;
    QString m_root;
    QString m_args;
    QString m_sshPort;
    QString m_watchPath;
    PortList m_freePorts;
    QList<Variable> m_normalVars;
    QString m_openGlBackendVarName;
    QHash<MaemoQemuSettings::OpenGlMode, QString> m_openGlBackendVarValues;
};

}   // namespace Internal
}   // namespace RemoteLinux

#endif // MAEMOQEMURUNTIME_H
