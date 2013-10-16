/**************************************************************************
**
** Copyright (C) 2011,2012,2013 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry Limited (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef BLACKBERRYCONFIGURATIONS_H
#define BLACKBERRYCONFIGURATIONS_H

#include "qnxconstants.h"

#include <utils/environment.h>
#include <utils/fileutils.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/gcctoolchain.h>

#include <QObject>
#include <QCoreApplication>

namespace QtSupport {
class BaseQtVersion;
}

namespace Qnx {
namespace Internal {

class QnxAbstractQtVersion;

class BlackBerryConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::BlackBerryConfiguration)
public:
    BlackBerryConfiguration(const Utils::FileName &ndkEnvFile, bool isAutoDetected, const QString &displayName = QString());
    bool activate();
    void deactivate();
    QString ndkPath() const;
    QString displayName() const;
    QString targetName() const;
    bool isAutoDetected() const;
    bool isActive() const;
    bool isValid() const;
    Utils::FileName ndkEnvFile() const;
    Utils::FileName qmake4BinaryFile() const;
    Utils::FileName qmake5BinaryFile() const;
    Utils::FileName gccCompiler() const;
    Utils::FileName deviceDebuger() const;
    Utils::FileName simulatorDebuger() const;
    Utils::FileName sysRoot() const;
    QMultiMap<QString, QString> qnxEnv() const;

private:
    QString m_displayName;
    QString m_targetName;
    bool m_isAutoDetected;
    Utils::FileName m_ndkEnvFile;
    Utils::FileName m_qmake4BinaryFile;
    Utils::FileName m_qmake5BinaryFile;
    Utils::FileName m_gccCompiler;
    Utils::FileName m_deviceDebugger;
    Utils::FileName m_simulatorDebugger;
    Utils::FileName m_sysRoot;
    QMultiMap<QString, QString> m_qnxEnv;

    void createConfigurationPerQtVersion(
            const Utils::FileName &qmakePath, Qnx::QnxArchitecture arch);
    QnxAbstractQtVersion* createQtVersion(
            const Utils::FileName &qmakePath, Qnx::QnxArchitecture arch);
    ProjectExplorer::GccToolChain* createGccToolChain(QnxAbstractQtVersion *version);
    ProjectExplorer::Kit* createKit(
            QnxAbstractQtVersion* version, ProjectExplorer::ToolChain* toolChain);
    QList<QtSupport::BaseQtVersion *> findRegisteredQtVersions() const;
};

} // namespace Internal
} // namespace Qnx

#endif // BLACKBERRYCONFIGURATIONS_H
