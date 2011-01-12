/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGINGHELPERBUILDTASK_H
#define DEBUGGINGHELPERBUILDTASK_H

#include "qtversionmanager.h"
#include <utils/environment.h>
#include <QtCore/QObject>

namespace Qt4ProjectManager {
namespace Internal {

class DebuggingHelperBuildTask : public QObject {
    Q_DISABLE_COPY(DebuggingHelperBuildTask)
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

    explicit DebuggingHelperBuildTask(QtVersion *version, Tools tools = AllTools);
    virtual ~DebuggingHelperBuildTask();

    void run(QFutureInterface<void> &future);

signals:
    void finished(int qtVersionId, DebuggingHelperBuildTask::Tools tools, const QString &output);

private:
    bool buildDebuggingHelper(QFutureInterface<void> &future, QString *output);

    Tools m_tools;

    int m_qtId;
    QString m_qtInstallData;
    QString m_target;
    QString m_qmakeCommand;
    QString m_makeCommand;
    QString m_mkspec;
    Utils::Environment m_environment;
    QString m_errorMessage;
};

} //namespace Internal
} //namespace Qt4ProjectManager

Q_DECLARE_METATYPE(Qt4ProjectManager::Internal::DebuggingHelperBuildTask::Tools)

#endif // DEBUGGINGHELPERBUILDTASK_H
