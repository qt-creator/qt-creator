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

#ifndef DEBUGGINGHELPERBUILDTASK_H
#define DEBUGGINGHELPERBUILDTASK_H

#include "qtsupport_global.h"
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <projectexplorer/toolchain.h>

#include <QObject>
#include <QFutureInterface>
#include <QMetaType>

namespace QtSupport {

class BaseQtVersion;

class QTSUPPORT_EXPORT DebuggingHelperBuildTask : public QObject
{
    Q_OBJECT

public:
    enum DebuggingHelper {
        GdbDebugging = 0x01,
        QmlDebugging = 0x02,
        QmlObserver = 0x04,
        QmlDump = 0x08,
        AllTools = GdbDebugging | QmlDebugging | QmlObserver | QmlDump
    };
    Q_DECLARE_FLAGS(Tools, DebuggingHelper)

    explicit DebuggingHelperBuildTask(const BaseQtVersion *version,
                                      ProjectExplorer::ToolChain *toolChain,
                                      Tools tools = AllTools);

    void showOutputOnError(bool show);
    void run(QFutureInterface<void> &future);

    static Tools availableTools(const BaseQtVersion *version);

signals:
    void finished(int qtVersionId, const QString &output, DebuggingHelperBuildTask::Tools tools);

    // used internally
    void logOutput(const QString &output, bool bringToForeground);
    void updateQtVersions(const Utils::FileName &qmakeCommand);

private:
    bool buildDebuggingHelper(QFutureInterface<void> &future);
    void log(const QString &output, const QString &error);

    const Tools m_tools;

    int m_qtId;
    QString m_qtInstallData;
    QString m_target;
    Utils::FileName m_qmakeCommand;
    QStringList m_qmakeArguments;
    QString m_makeCommand;
    QStringList m_makeArguments;
    Utils::FileName m_mkspec;
    Utils::Environment m_environment;
    QString m_log;
    bool m_invalidQt;
    bool m_showErrors;
};

} // namespace QtSupport

Q_DECLARE_METATYPE(QtSupport::DebuggingHelperBuildTask::Tools)

#endif // DEBUGGINGHELPERBUILDTASK_H
