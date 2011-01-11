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
class QtVersion;
namespace Internal {
class MaemoDeviceConfig;

class MaemoGlobal
{
public:
    enum MaemoVersion { Maemo5, Maemo6 };

    class FileUpdate {
    public:
        FileUpdate(const QString &fileName);
        ~FileUpdate();
    private:
        const QString m_fileName;
    };

    static QString homeDirOnDevice(const QString &uname);
    static QString remoteSudo();
    static QString remoteCommandPrefix(const QString &commandFilePath);
    static QString remoteEnvironment(const QList<Utils::EnvironmentItem> &list);
    static QString remoteSourceProfilesCommand();
    static QString failedToConnectToServerMessage(const QSharedPointer<Core::SshConnection> &connection,
        const MaemoDeviceConfig &deviceConfig);

    static QString maddeRoot(const QtVersion *qtVersion);
    static QString targetRoot(const QtVersion *qtVersion);
    static QString targetName(const QtVersion *qtVersion);
    static QString madCommand(const QtVersion *qtVersion);
    static MaemoVersion version(const QtVersion *qtVersion);
    static bool allowsRemoteMounts(const QtVersion *qtVersion) { return version(qtVersion) == Maemo5; }
    static bool allowsPackagingDisabling(const QtVersion *qtVersion) { return version(qtVersion) == Maemo5; }
    static bool allowsQmlDebugging(const QtVersion *qtVersion) { return version(qtVersion) == Maemo6; }

    static bool callMad(QProcess &proc, const QStringList &args,
        const QtVersion *qtVersion);
    static bool callMadAdmin(QProcess &proc, const QStringList &args,
        const QtVersion *qtVersion);

    static bool removeRecursively(const QString &filePath, QString &error);

    template<class T> static T *buildStep(const ProjectExplorer::DeployConfiguration *dc)
    {
        if (!dc)
            return 0;
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

private:
    static QString madAdminCommand(const QtVersion *qtVersion);
    static bool callMaddeShellScript(QProcess &proc, const QString &maddeRoot,
        const QString &command, const QStringList &args);
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOGLOBAL_H
