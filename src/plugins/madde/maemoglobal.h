/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MAEMOGLOBAL_H
#define MAEMOGLOBAL_H

#include <coreplugin/id.h>
#include <coreplugin/idocument.h>
#include <utils/portlist.h>
#include <utils/environment.h>

#include <QCoreApplication>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QProcess;
class QString;
QT_END_NAMESPACE

namespace QtSupport { class BaseQtVersion; }
namespace RemoteLinux { class LinuxDeviceConfiguration; }
namespace ProjectExplorer { class Target; }

namespace Madde {
namespace Internal {

class WatchableFile : public Core::IDocument
{
    Q_OBJECT
public:
    WatchableFile(const QString &fileName, QObject *parent = 0)
        : Core::IDocument(parent), m_fileName(fileName) {}

    bool save(QString *, const QString &, bool) { return false; }
    QString fileName() const { return m_fileName; }
    QString defaultPath() const { return QString(); }
    QString suggestedFileName() const { return QString(); }
    QString mimeType() const { return QLatin1String("text/plain"); }
    bool isModified() const { return false; }
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

    static bool isMaemoTargetId(const Core::Id id);
    static bool isFremantleTargetId(const Core::Id id);
    static bool isHarmattanTargetId(const Core::Id id);
    static bool isMeegoTargetId(const Core::Id id);
    static bool isValidMaemo5QtVersion(const QString &qmakePath);
    static bool isValidHarmattanQtVersion(const QString &qmakePath);
    static bool isValidMeegoQtVersion(const QString &qmakePath);

    static QString homeDirOnDevice(const QString &uname);
    static QString devrootshPath();
    static int applicationIconSize(const ProjectExplorer::Target *target);
    static QString remoteSudo(Core::Id deviceType, const QString &uname);
    static QString remoteSourceProfilesCommand();
    static Utils::PortList freePorts(const QSharedPointer<const RemoteLinux::LinuxDeviceConfiguration> &devConf,
        const QtSupport::BaseQtVersion *qtVersion);

    static void addMaddeEnvironment(Utils::Environment &env, const QString &qmakePath);
    static void transformMaddeCall(QString &command, QStringList &args, const QString &qmakePath);
    static QString maddeRoot(const QString &qmakePath);
    static QString targetRoot(const QString &qmakePath);
    static QString targetName(const QString &qmakePath);
    static QString madCommand(const QString &qmakePath);
    static QString madDeveloperUiName(Core::Id deviceType);
    static Core::Id deviceType(const QString &qmakePath);

    // TODO: IS this still needed with Qt Version having an Abi?
    static QString architecture(const QString &qmakePath);

    static bool callMad(QProcess &proc, const QStringList &args,
        const QString &qmakePath, bool useTarget);
    static bool callMadAdmin(QProcess &proc, const QStringList &args,
        const QString &qmakePath, bool useTarget);

    static bool isValidMaemoQtVersion(const QString &qmakePath, Core::Id deviceType);
private:
    static QString madAdminCommand(const QString &qmakePath);
    static bool callMaddeShellScript(QProcess &proc, const QString &qmakePath,
        const QString &command, const QStringList &args, bool useTarget);
    static QStringList targetArgs(const QString &qmakePath, bool useTarget);
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOGLOBAL_H
