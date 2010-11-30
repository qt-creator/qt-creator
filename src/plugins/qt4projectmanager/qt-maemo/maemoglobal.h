/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MAEMOGLOBAL_H
#define MAEMOGLOBAL_H

#include <utils/environment.h>

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QtCore/QList>
#include <QtCore/QSharedPointer>

#define ASSERT_STATE_GENERIC(State, expected, actual)                         \
    MaemoGlobal::assertState<State>(expected, actual, Q_FUNC_INFO)

QT_BEGIN_NAMESPACE
class QProcess;
class QString;
QT_END_NAMESPACE

namespace Core { class SshConnection; }

namespace Qt4ProjectManager {
namespace Internal {
class MaemoDeviceConfig;

class MaemoGlobal
{
public:
    static QString homeDirOnDevice(const QString &uname);
    static QString remoteSudo();
    static QString remoteCommandPrefix(const QString &commandFilePath);
    static QString remoteEnvironment(const QList<Utils::EnvironmentItem> &list);
    static QString remoteSourceProfilesCommand();
    static QString failedToConnectToServerMessage(const QSharedPointer<Core::SshConnection> &connection,
        const MaemoDeviceConfig &deviceConfig);
    static QString maddeRoot(const QString &qmakePath);
    static QString targetName(const QString &qmakePath);

    static bool removeRecursively(const QString &filePath, QString &error);
    static void callMaddeShellScript(QProcess &proc, const QString &maddeRoot,
        const QString &command, const QStringList &args);

    template<class T> static T *buildStep(const ProjectExplorer::DeployConfiguration *dc)
    {
        ProjectExplorer::BuildStepList *bsl = dc->stepList();
        if (!bsl)
            return 0;
        const QList<ProjectExplorer::BuildStep *> &buildSteps = bsl->steps();
        for (int i = buildSteps.count() - 1; i >= 0; --i) {
            if (T * const step = qobject_cast<T *>(buildSteps.at(i)))
                return step;
        }
        return 0;
    }

    template<typename State> static void assertState(State expected,
        State actual, const char *func)
    {
        assertState(QList<State>() << expected, actual, func);
    }

    template<typename State> static void assertState(const QList<State> &expected,
        State actual, const char *func)
    {
        if (!expected.contains(actual)) {
            qWarning("Warning: Unexpected state %d in function %s.",
                actual, func);
        }
    }
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOGLOBAL_H
