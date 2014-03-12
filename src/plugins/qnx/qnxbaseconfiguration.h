/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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

#ifndef QNXBASECONFIGURATION_H
#define QNXBASECONFIGURATION_H

#include <qnxconstants.h>
#include <qnxutils.h>

#include <utils/fileutils.h>
#include <utils/environment.h>

#include <projectexplorer/abi.h>

#include <debugger/debuggeritemmanager.h>

#include <qvariant.h>

namespace Qnx {
namespace Internal {

class QnxToolChain;

class QnxBaseConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::QnxBaseConfiguration)

public:
    QnxBaseConfiguration();
    QnxBaseConfiguration(const Utils::FileName &envFile);
    QnxBaseConfiguration(const QVariantMap &data);
    virtual ~QnxBaseConfiguration();
    Utils::FileName envFile() const;
    Utils::FileName qnxTarget() const;
    Utils::FileName qnxHost() const;
    Utils::FileName qccCompilerPath() const;
    Utils::FileName armDebuggerPath() const;
    Utils::FileName x86DebuggerPath() const;
    QList<Utils::EnvironmentItem> qnxEnv() const;
    QVariantMap toMap() const;

    virtual bool isValid() const;
    virtual bool isActive() const = 0;

    virtual bool activate() = 0;
    virtual void deactivate() = 0;

protected:
     QVariant createDebuggerItem(Qnx::QnxArchitecture arch,
                                              const QString &displayName);
     QnxToolChain* createToolChain(Qnx::QnxArchitecture arch,
                                  const QString &displayName,
                                  const QString &ndkPath);

     virtual QStringList validationErrors() const;
private:
    Utils::FileName m_envFile;
    Utils::FileName m_qnxTarget;
    Utils::FileName m_qnxHost;
    Utils::FileName m_qccCompiler;
    Utils::FileName m_armlev7Debugger;
    Utils::FileName m_x86Debugger;
    QList<Utils::EnvironmentItem> m_qnxEnv;

    void ctor(const Utils::FileName &envFile);
};

}
}

#endif // QNXBASECONFIGURATION_H
