/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BLACKBERRYCHECKDEVICESTATUSSTEP_H
#define QNX_INTERNAL_BLACKBERRYCHECKDEVICESTATUSSTEP_H

#include "blackberrydeviceconfiguration.h"

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
class QEventLoop;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

class BlackBerryDeviceInformation;
class BlackBerryCheckDeviceStatusStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class BlackBerryCheckDeviceStatusStepFactory;

public:
    explicit BlackBerryCheckDeviceStatusStep(ProjectExplorer::BuildStepList *bsl);

    bool init();
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    void raiseError(const QString &error);
    void raiseWarning(const QString &warning);

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    bool debugTokenCheckEnabled () const;
    bool runtimeCheckEnabled() const;

protected:
    BlackBerryCheckDeviceStatusStep(ProjectExplorer::BuildStepList *bsl,
                                    BlackBerryCheckDeviceStatusStep *bs);

protected slots:
    void checkDeviceInfo(int status);
    void emitOutputInfo();

    void enableDebugTokenCheck(bool enable);
    void enableRuntimeCheck(bool enable);

    bool handleVersionMismatch(const QString &runtimeVersion, const QString &apiLevelVersion);

private:
    BlackBerryDeviceInformation *m_deviceInfo;
    BlackBerryDeviceConfiguration::ConstPtr m_device;
    QEventLoop *m_eventLoop;

    bool m_runtimeCheckEnabled;
    bool m_debugTokenCheckEnabled;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYCHECKDEVICESTATUSSTEP_H
