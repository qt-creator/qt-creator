/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef DEBUGGINGHELPERBUILDTASK_H
#define DEBUGGINGHELPERBUILDTASK_H

#include "qtsupport_global.h"
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <coreplugin/messagemanager.h>

#include <QObject>
#include <QFutureInterface>
#include <QMetaType>

namespace ProjectExplorer { class ToolChain; }

namespace QtSupport {

class BaseQtVersion;

class QTSUPPORT_EXPORT DebuggingHelperBuildTask : public QObject
{
    Q_OBJECT

public:
    enum DebuggingHelper {
        QmlDump = 0x02,
        AllTools = QmlDump
    };
    Q_DECLARE_FLAGS(Tools, DebuggingHelper)

    explicit DebuggingHelperBuildTask(const BaseQtVersion *version,
                                      const ProjectExplorer::ToolChain *toolChain,
                                      Tools tools = AllTools);

    void showOutputOnError(bool show);
    void run(QFutureInterface<void> &future);

    static Tools availableTools(const BaseQtVersion *version);

signals:
    void finished(int qtVersionId, const QString &output, DebuggingHelperBuildTask::Tools tools);

    // used internally
    void logOutput(const QString &output, Core::MessageManager::PrintToOutputPaneFlags flags);
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
