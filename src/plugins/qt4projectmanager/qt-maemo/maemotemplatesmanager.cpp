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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "maemotemplatesmanager.h"

#include "maemopackagecreationstep.h"
#include "maemotoolchain.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4projectmanagerconstants.h>
#include <qt4projectmanager/qt4target.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QProcess>
#include <QtGui/QMessageBox>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

const QLatin1String MaemoTemplatesManager::PackagingDirName("packaging");

MaemoTemplatesManager *MaemoTemplatesManager::m_instance = 0;

MaemoTemplatesManager *MaemoTemplatesManager::instance(QObject *parent)
{
    Q_ASSERT(!m_instance != !parent);
    if (!m_instance)
        m_instance = new MaemoTemplatesManager(parent);
    return m_instance;
}

MaemoTemplatesManager::MaemoTemplatesManager(QObject *parent) :
    QObject(parent), m_activeProject(0)
{
    SessionManager * const session
        = ProjectExplorerPlugin::instance()->session();
    connect(session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        this, SLOT(handleActiveProjectChanged(ProjectExplorer::Project*)));
    handleActiveProjectChanged(session->startupProject());
}

void MaemoTemplatesManager::handleActiveProjectChanged(ProjectExplorer::Project *project)
{
    if (m_activeProject)
        disconnect(m_activeProject, 0, this, 0);
    m_activeProject= project;
    if (m_activeProject) {
        connect(m_activeProject, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(createTemplatesIfNecessary(ProjectExplorer::Target*)));
        connect(m_activeProject,
            SIGNAL(activeTargetChanged(ProjectExplorer::Target*)), this,
            SLOT(createTemplatesIfNecessary(ProjectExplorer::Target*)));
        const QList<Target *> &targets = m_activeProject->targets();
        foreach (Target * const target, targets)
            createTemplatesIfNecessary(target);
    }
}

void MaemoTemplatesManager::createTemplatesIfNecessary(ProjectExplorer::Target *target)
{
    Q_ASSERT_X(m_activeProject, Q_FUNC_INFO,
        "Impossible: Received signal from unknown project");

    if (!target
        || target->id() != QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        return;

    QDir projectDir(m_activeProject->projectDirectory());
    if (projectDir.exists(PackagingDirName))
        return;
    const QString packagingTemplatesDir
        = projectDir.path() + QLatin1Char('/') + PackagingDirName;
    if (!projectDir.mkdir(PackagingDirName)) {
        raiseError(tr("Could not create directory '%1'.")
            .arg(QDir::toNativeSeparators(packagingTemplatesDir)));
        return;
    }

    QProcess dh_makeProc;
    QString error;
    const Qt4Target * const qt4Target = qobject_cast<Qt4Target *>(target);
    Q_ASSERT_X(qt4Target, Q_FUNC_INFO, "Target ID does not match actual type.");
    const MaemoToolChain * const tc
        = dynamic_cast<MaemoToolChain *>(qt4Target->activeBuildConfiguration()->toolChain());
    Q_ASSERT_X(tc, Q_FUNC_INFO, "Maemo target has no maemo toolchain.");
    if (!MaemoPackageCreationStep::preparePackagingProcess(&dh_makeProc, tc,
        packagingTemplatesDir, &error)) {
        raiseError(error);
        return;
    }

    const QString command = QLatin1String("dh_make -s -n -p ")
        + m_activeProject->displayName() + QLatin1Char('_')
        + MaemoPackageCreationStep::DefaultVersionNumber;
    dh_makeProc.start(MaemoPackageCreationStep::packagingCommand(tc, command));
    if (!dh_makeProc.waitForStarted()) {
        raiseError(tr("Unable to create debian templates: dh_make failed (%1)")
            .arg(dh_makeProc.errorString()));
        return;
    }
    dh_makeProc.write("\n"); // Needs user input.
    dh_makeProc.waitForFinished(-1);
    if (dh_makeProc.error() != QProcess::UnknownError
        || dh_makeProc.exitCode() != 0) {
        raiseError(tr("Unable to create debian templates: dh_make failed (%1)")
            .arg(dh_makeProc.errorString()));
        return;
    }

    QDir debianDir(packagingTemplatesDir + QLatin1String("/debian"));
    const QStringList &files = debianDir.entryList(QDir::Files);
    QStringList filesToAddToProject;
    foreach (const QString &fileName, files) {
        if (fileName.endsWith(QLatin1String(".ex"), Qt::CaseInsensitive)) {
            debianDir.remove(fileName);
        } else
            filesToAddToProject << debianDir.absolutePath()
                + QLatin1Char('/') + fileName;
    }
    qobject_cast<Qt4Project *>(m_activeProject)->rootProjectNode()
        ->addFiles(UnknownFileType, filesToAddToProject);

    const QString rulesFilePath
        = packagingTemplatesDir + QLatin1String("/debian/rules");
    QFile rulesFile(rulesFilePath);
    if (!rulesFile.open(QIODevice::ReadWrite)) {
        raiseError(tr("Packaging Error: Cannot open file '%1'.")
                   .arg(QDir::toNativeSeparators(rulesFilePath)));
        return;
    }

    QByteArray rulesContents = rulesFile.readAll();
    rulesContents.replace("DESTDIR", "INSTALL_ROOT");
    rulesContents.replace("dh_shlibdeps", "# dh_shlibdeps");

    // Would be the right solution, but does not work (on Windows),
    // because dpkg-genchanges doesn't know about it (and can't be told).
    // rulesContents.replace("dh_builddeb", "dh_builddeb --destdir=.");

    rulesFile.resize(0);
    rulesFile.write(rulesContents);
    if (rulesFile.error() != QFile::NoError) {
        raiseError(tr("Packaging Error: Cannot write file '%1'.")
                   .arg(QDir::toNativeSeparators(rulesFilePath)));
        return;
    }
}

QString MaemoTemplatesManager::version(const Project *project,
    QString *error) const
{
    const QString changeLogFilePath
        = project->projectDirectory() + QLatin1Char('/') + PackagingDirName
            + QLatin1String("/debian/changelog");
    const QString nativePath = QDir::toNativeSeparators(changeLogFilePath);
    QFile changeLog(changeLogFilePath);
    if (!changeLog.exists()) {
        *error = tr("File '%1' does not exist").arg(nativePath);
        return QString();
    }
    if (!changeLog.open(QIODevice::ReadOnly)) {
        *error = tr("Cannot open Debian changelog file '%1': %2")
            .arg(nativePath, changeLog.errorString());
        return QString();
    }
    const QByteArray &firstLine = changeLog.readLine();
    const int openParenPos = firstLine.indexOf('(');
    if (openParenPos == -1) {
        *error = tr("Debian changelog file '%1' has unexpected format.")
            .arg(nativePath);
        return QString();
    }
    const int closeParenPos = firstLine.indexOf(')', openParenPos);
    if (closeParenPos == -1) {
        *error = tr("Debian changelog file '%1' has unexpected format.")
            .arg(nativePath);
        return QString();
    }
    return QString::fromUtf8(firstLine.mid(openParenPos + 1,
        closeParenPos - openParenPos - 1).data());
}

bool MaemoTemplatesManager::setVersion(const Project *project,
    const QString &version, QString *error) const
{
    const QString debianDir = project->projectDirectory() + QLatin1Char('/')
        + PackagingDirName + QLatin1String("/debian/");
    const QString changeLogFilePath = debianDir + QLatin1String("changelog");
    const QString nativePath = QDir::toNativeSeparators(changeLogFilePath);
    QFile changeLog(changeLogFilePath);
    if (!changeLog.exists()) {
        *error = tr("File '%1' does not exist").arg(nativePath);
        return false;
    }
    if (!changeLog.open(QIODevice::ReadWrite)) {
        *error = tr("Cannot open Debian changelog file '%1': %2")
            .arg(nativePath , changeLog.errorString());
        return false;
    }

    QString content = QString::fromUtf8(changeLog.readAll());
    content.replace(QRegExp(QLatin1String("\\([a-zA-Z0-9_\\.]+\\)")),
        QLatin1Char('(') % version % QLatin1Char(')'));
    changeLog.resize(0);
    changeLog.write(content.toUtf8());
    changeLog.close();
    if (changeLog.error() != QFile::NoError) {
        *error = tr("Error writing Debian changelog file '%1': %2")
            .arg(nativePath , changeLog.errorString());
        return false;
    }
    return true;
}

void MaemoTemplatesManager::raiseError(const QString &reason)
{
    QMessageBox::critical(0, tr("Error creating Maemo templates"), reason);
}

} // namespace Internal
} // namespace Qt4ProjectManager
