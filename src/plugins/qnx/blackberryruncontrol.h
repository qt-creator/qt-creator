/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
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

#ifndef QNX_INTERNAL_BLACKBERRYRUNCONTROL_H
#define QNX_INTERNAL_BLACKBERRYRUNCONTROL_H

#include <projectexplorer/runconfiguration.h>

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;
}

namespace Qnx {
namespace Internal {

class BlackBerryConnect;
class BlackBerryRunConfiguration;
class BlackBerryApplicationRunner;

class BlackBerryRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    explicit BlackBerryRunControl(BlackBerryRunConfiguration *runConfiguration);
    ~BlackBerryRunControl();

    void start();
    ProjectExplorer::RunControl::StopResult stop();
    bool isRunning() const;
    QIcon icon() const;

private slots:
    void handleStartFailed(const QString &message);

private:
    BlackBerryApplicationRunner *m_runner;
    BlackBerryConnect *m_connector;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYRUNCONTROL_H
