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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/
#ifndef MAEMOQEMURUNTIME_H
#define MAEMOQEMURUNTIME_H

#include "maemodeviceconfigurations.h"
#include "maemoqemusettings.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QString>

namespace Qt4ProjectManager {
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
    MaemoPortList m_freePorts;
    QList<Variable> m_normalVars;
    QString m_openGlBackendVarName;
    QHash<MaemoQemuSettings::OpenGlMode, QString> m_openGlBackendVarValues;
};

}   // namespace Internal
}   // namespace Qt4ProjectManager

#endif // MAEMOQEMURUNTIME_H
