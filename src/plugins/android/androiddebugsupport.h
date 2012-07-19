/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef ANDROIDDEBUGSUPPORT_H
#define ANDROIDDEBUGSUPPORT_H

#include "androidrunconfiguration.h"

#include <QObject>
#include <QPointer>

namespace Debugger {
class DebuggerRunControl;
}
namespace QtSupport {class BaseQtVersion; }
namespace ProjectExplorer { class RunControl; }

namespace Android {

namespace Internal {

class AndroidRunConfiguration;
class AndroidRunner;

class AndroidDebugSupport : public QObject
{
    Q_OBJECT
public:
    static ProjectExplorer::RunControl *createDebugRunControl(AndroidRunConfiguration *runConfig);

    AndroidDebugSupport(AndroidRunConfiguration *runConfig,
        Debugger::DebuggerRunControl *runControl);
    ~AndroidDebugSupport();

private slots:
    void handleRemoteProcessStarted(int gdbServerPort = -1, int qmlPort = -1);
    void handleRemoteProcessFinished(const QString &errorMsg);

    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);

private:
    static QStringList qtSoPaths(QtSupport::BaseQtVersion *qtVersion);

private:
    Debugger::DebuggerRunControl* m_runControl;
    AndroidRunner * const m_runner;
    const AndroidRunConfiguration::DebuggingType m_debuggingType;
    const QString m_dumperLib;

    int m_gdbServerPort;
    int m_qmlPort;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDDEBUGSUPPORT_H
