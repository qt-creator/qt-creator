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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNXCONFIGURATION_H
#define QNXCONFIGURATION_H

#include "qnxbaseconfiguration.h"
#include "qnxversionnumber.h"

namespace ProjectExplorer { class Kit; }

namespace Qnx {
namespace Internal {
class QnxQtVersion;
class QnxConfiguration : public QnxBaseConfiguration
{
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::QnxConfiguration)

public:
    QnxConfiguration(const Utils::FileName &sdpEnvFile);
    QnxConfiguration(const QVariantMap &data);
    QString displayName() const;
    bool activate();
    void deactivate();
    bool isActive() const;
    bool canCreateKits() const;
    Utils::FileName sdpPath() const;
    QnxQtVersion* qnxQtVersion(QnxArchitecture arch) const;

private:
    QString m_configName;

    ProjectExplorer::Kit *createKit(QnxArchitecture arch,
                                    QnxToolChain *toolChain,
                                    const QVariant &debuggerItemId,
                                    const QString &displayName);

    void readInformation();

};

} // Internal
} // Qnx

#endif // QNXCONFIGURATION_H
