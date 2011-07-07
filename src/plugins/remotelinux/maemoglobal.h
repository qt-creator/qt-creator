/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMOGLOBAL_H
#define MAEMOGLOBAL_H

#include "linuxdeviceconfiguration.h"

#include <coreplugin/ifile.h>
#include <utils/environment.h>

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QCoreApplication>

#define ASSERT_STATE_GENERIC(State, expected, actual)                         \
    MaemoGlobal::assertState<State>(expected, actual, Q_FUNC_INFO)

QT_BEGIN_NAMESPACE
class QDateTime;
class QProcess;
class QString;
QT_END_NAMESPACE

namespace QtSupport { class BaseQtVersion; }
namespace Utils { class SshConnection; }

namespace RemoteLinux {
namespace Internal {

class WatchableFile : public Core::IFile
{
    Q_OBJECT
public:
    WatchableFile(const QString &fileName, QObject *parent = 0)
        : Core::IFile(parent), m_fileName(fileName) {}

    bool save(QString *, const QString &, bool) { return false; }
    QString fileName() const { return m_fileName; }
    QString defaultPath() const { return QString(); }
    QString suggestedFileName() const { return QString(); }
    QString mimeType() const { return QLatin1String("text/plain"); }
    bool isModified() const { return false; }
    bool isReadOnly() const { return false; }
    bool isSaveAsAllowed() const { return false; }
    ReloadBehavior reloadBehavior(ChangeTrigger, ChangeType) const { return BehaviorSilent; }
    bool reload(QString *, ReloadFlag, ChangeType) { emit modified(); return true; }
    void rename(const QString &) {}

signals:
    void modified();

private:
    QString m_fileName;
};

class MaemoGlobal
{
    Q_DECLARE_TR_FUNCTIONS(RemoteLinux::Internal::MaemoGlobal)
public:
    enum PackagingSystem { Dpkg, Rpm, Tar };

    static bool isMaemoTargetId(const QString &id);
    static bool isFremantleTargetId(const QString &id);
    static bool isHarmattanTargetId(const QString &id);
    static bool isMeegoTargetId(const QString &id);
    static bool isValidMaemo5QtVersion(const QString &qmakePath);
    static bool isValidHarmattanQtVersion(const QString &qmakePath);
    static bool isValidMeegoQtVersion(const QString &qmakePath);

    static bool isLinuxQt(const QtSupport::BaseQtVersion *qtVersion);
    static bool hasLinuxQt(const ProjectExplorer::Target *target);

    static QString homeDirOnDevice(const QString &uname);
    static QString devrootshPath();
    static int applicationIconSize(const QString &osType);
    static QString remoteSudo(const QString &osType, const QString &uname);
    static QString remoteCommandPrefix(const QString &osType);
    static QString remoteSourceProfilesCommand();
    static QString failedToConnectToServerMessage(const QSharedPointer<Utils::SshConnection> &connection,
        const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfig);
    static QString deviceConfigurationName(const QSharedPointer<const LinuxDeviceConfiguration> &devConf);
    static PortList freePorts(const QSharedPointer<const LinuxDeviceConfiguration> &devConf,
        const QtSupport::BaseQtVersion *qtVersion);

    static void addMaddeEnvironment(Utils::Environment &env, const QString &qmakePath);
    static void transformMaddeCall(QString &command, QStringList &args, const QString &qmakePath);
    static QString maddeRoot(const QString &qmakePath);
    static QString targetRoot(const QString &qmakePath);
    static QString targetName(const QString &qmakePath);
    static QString madCommand(const QString &qmakePath);
    static QString madDeveloperUiName(const QString &osType);
    static QString osType(const QString &qmakePath);

    // TODO: IS this still needed with Qt Version having an Abi?
    static QString architecture(const QString &qmakePath);

    static bool callMad(QProcess &proc, const QStringList &args,
        const QString &qmakePath, bool useTarget);
    static bool callMadAdmin(QProcess &proc, const QStringList &args,
        const QString &qmakePath, bool useTarget);

    static QString osTypeToString(const QString &osType);

    static PackagingSystem packagingSystem(const QString &osType);

    template<class T> static T *earlierBuildStep(const ProjectExplorer::DeployConfiguration *dc,
        const ProjectExplorer::BuildStep *laterBuildStep)
    {
        if (!dc)
            return 0;
        const ProjectExplorer::BuildStepList * const bsl = dc->stepList();
        const QList<ProjectExplorer::BuildStep *> &buildSteps = bsl->steps();
        for (int i = 0; i < buildSteps.count(); ++i) {
            if (buildSteps.at(i) == laterBuildStep)
                return 0;
            if (T * const step = dynamic_cast<T *>(buildSteps.at(i)))
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

    static bool isValidMaemoQtVersion(const QString &qmakePath, const QString &osType);
private:
    static QString madAdminCommand(const QString &qmakePath);
    static bool callMaddeShellScript(QProcess &proc, const QString &qmakePath,
        const QString &command, const QStringList &args, bool useTarget);
    static QStringList targetArgs(const QString &qmakePath, bool useTarget);
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOGLOBAL_H
