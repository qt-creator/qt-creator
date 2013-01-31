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

#ifndef MAEMOGLOBAL_H
#define MAEMOGLOBAL_H

#include <coreplugin/id.h>
#include <coreplugin/idocument.h>
#include <utils/fileutils.h>
#include <utils/portlist.h>
#include <utils/environment.h>

#include <QCoreApplication>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class Target;
} // namespace ProjectExplorer

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
    static bool hasMaemoDevice(const ProjectExplorer::Kit *k);
    static bool supportsMaemoDevice(const ProjectExplorer::Kit *k);
    static bool isValidMaemo5QtVersion(const QString &qmakePath);
    static bool isValidHarmattanQtVersion(const QString &qmakePath);

    static QString homeDirOnDevice(const QString &uname);
    static QString devrootshPath();
    static int applicationIconSize(const ProjectExplorer::Target *target);
    static QString remoteSudo(Core::Id deviceType, const QString &uname);
    static QString remoteSourceProfilesCommand();
    static Utils::PortList freePorts(const ProjectExplorer::Kit *k);

    static void addMaddeEnvironment(Utils::Environment &env, const QString &qmakePath);
    static void transformMaddeCall(QString &command, QStringList &args, const QString &qmakePath);
    static QString maddeRoot(const QString &qmakePath);
    static Utils::FileName maddeRoot(const ProjectExplorer::Kit *k);
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
