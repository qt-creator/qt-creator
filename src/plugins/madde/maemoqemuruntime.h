/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef MAEMOQEMURUNTIME_H
#define MAEMOQEMURUNTIME_H

#include "maemoqemusettings.h"

#include <utils/portlist.h>

#include <QHash>
#include <QList>
#include <QPair>
#include <QProcessEnvironment>
#include <QString>

namespace Madde {
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
    Utils::PortList m_freePorts;
    QList<Variable> m_normalVars;
    QString m_openGlBackendVarName;
    QHash<MaemoQemuSettings::OpenGlMode, QString> m_openGlBackendVarValues;
};

}   // namespace Internal
}   // namespace Madde

#endif // MAEMOQEMURUNTIME_H
