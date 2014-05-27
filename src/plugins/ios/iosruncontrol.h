/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#ifndef IOSRUNCONTROL_H
#define IOSRUNCONTROL_H

#include <projectexplorer/runconfiguration.h>
#include <utils/qtcoverride.h>

namespace Ios {
namespace Internal {

class IosRunConfiguration;
class IosRunner;

class IosRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    explicit IosRunControl(IosRunConfiguration *runConfig);
    ~IosRunControl();


    void start() QTC_OVERRIDE;
    StopResult stop() QTC_OVERRIDE;
    bool isRunning() const QTC_OVERRIDE;
    QString displayName() const QTC_OVERRIDE;

private slots:
    void handleRemoteProcessFinished(bool cleanEnd);
    void handleRemoteOutput(const QString &output);
    void handleRemoteErrorOutput(const QString &output);

private:

    IosRunner * const m_runner;
    bool m_running;
};

} // namespace Internal
} // namespace Ios

#endif // IOSRUNCONTROL_H
