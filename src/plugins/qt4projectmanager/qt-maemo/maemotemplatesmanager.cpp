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

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QList>
#include <QtGui/QPixmap>
#include <QtCore/QProcess>
#include <QtGui/QMessageBox>

#include <cctype>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {

namespace {
const QByteArray IconFieldName("XB-Maemo-Icon-26:");
} // anonymous namespace

const QLatin1String MaemoTemplatesManager::PackagingDirName("packaging");

MaemoTemplatesManager *MaemoTemplatesManager::m_instance = 0;

MaemoTemplatesManager *MaemoTemplatesManager::instance(QObject *parent)
{
    Q_ASSERT(!m_instance != !parent);
    if (!m_instance)
        m_instance = new MaemoTemplatesManager(parent);
    return m_instance;
}

MaemoTemplatesManager::MaemoTemplatesManager(QObject *parent) : QObject(parent)
{
    SessionManager * const session
        = ProjectExplorerPlugin::instance()->session();
    connect(session, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
        this, SLOT(handleActiveProjectChanged(ProjectExplorer::Project*)));
    connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
        this, SLOT(handleProjectToBeRemoved(ProjectExplorer::Project*)));
    handleActiveProjectChanged(session->startupProject());
}

void MaemoTemplatesManager::handleActiveProjectChanged(ProjectExplorer::Project *project)
{
    if (project && !m_maemoProjects.contains(project)) {
        connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(handleTarget(ProjectExplorer::Target*)));
        connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            this, SLOT(handleTarget(ProjectExplorer::Target*)));
        const QList<Target *> &targets = project->targets();
        foreach (Target * const target, targets)
            handleTarget(target);
    }
}

void MaemoTemplatesManager::handleTarget(ProjectExplorer::Target *target)
{
    if (!target
        || target->id() != QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        return;
    if (!createTemplatesIfNecessary(target))
        return;

    Project * const project = target->project();
    QFileSystemWatcher * const fsWatcher = new QFileSystemWatcher(this);
    fsWatcher->addPath(debianDirPath(project));
    fsWatcher->addPath(changeLogFilePath(project));
    fsWatcher->addPath(controlFilePath(project));
    connect(fsWatcher, SIGNAL(directoryChanged(QString)), this,
        SLOT(handleDebianDirContentsChanged()));
    connect(fsWatcher, SIGNAL(fileChanged(QString)), this,
        SLOT(handleDebianFileChanged(QString)));
    handleDebianDirContentsChanged();
    handleDebianFileChanged(changeLogFilePath(project));
    handleDebianFileChanged(controlFilePath(project));
    m_maemoProjects.insert(project, fsWatcher);
}

bool MaemoTemplatesManager::createTemplatesIfNecessary(const ProjectExplorer::Target *target)
{
    Project * const project = target->project();
    QDir projectDir(project->projectDirectory());
    if (projectDir.exists(PackagingDirName))
        return true;
    const QString packagingTemplatesDir
        = projectDir.path() + QLatin1Char('/') + PackagingDirName;
    if (!projectDir.mkdir(PackagingDirName)) {
        raiseError(tr("Could not create directory '%1'.")
            .arg(QDir::toNativeSeparators(packagingTemplatesDir)));
        return false;
    }

    QProcess dh_makeProc;
    QString error;
    const Qt4Target * const qt4Target = qobject_cast<const Qt4Target *>(target);
    Q_ASSERT_X(qt4Target, Q_FUNC_INFO, "Target ID does not match actual type.");
    const MaemoToolChain * const tc
        = dynamic_cast<MaemoToolChain *>(qt4Target->activeBuildConfiguration()->toolChain());
    Q_ASSERT_X(tc, Q_FUNC_INFO, "Maemo target has no maemo toolchain.");
    if (!MaemoPackageCreationStep::preparePackagingProcess(&dh_makeProc, tc,
        packagingTemplatesDir, &error)) {
        raiseError(error);
        return false;
    }

    const QString command = QLatin1String("dh_make -s -n -p ")
        + project->displayName() + QLatin1Char('_')
        + MaemoPackageCreationStep::DefaultVersionNumber;
    dh_makeProc.start(MaemoPackageCreationStep::packagingCommand(tc, command));
    if (!dh_makeProc.waitForStarted()) {
        raiseError(tr("Unable to create debian templates: dh_make failed (%1)")
            .arg(dh_makeProc.errorString()));
        return false;
    }
    dh_makeProc.write("\n"); // Needs user input.
    dh_makeProc.waitForFinished(-1);
    if (dh_makeProc.error() != QProcess::UnknownError
        || dh_makeProc.exitCode() != 0) {
        raiseError(tr("Unable to create debian templates: dh_make failed (%1)")
            .arg(dh_makeProc.errorString()));
        return false;
    }

    QDir debianDir(packagingTemplatesDir + QLatin1String("/debian"));
    const QStringList &files = debianDir.entryList(QDir::Files);
    QStringList filesToAddToProject;
    foreach (const QString &fileName, files) {
        if (fileName.endsWith(QLatin1String(".ex"), Qt::CaseInsensitive)
            || fileName.compare(QLatin1String("README.debian"), Qt::CaseInsensitive) == 0
            || fileName.compare(QLatin1String("dirs"), Qt::CaseInsensitive) == 0
            || fileName.compare(QLatin1String("docs"), Qt::CaseInsensitive) == 0) {
            debianDir.remove(fileName);
        } else
            filesToAddToProject << debianDir.absolutePath()
                + QLatin1Char('/') + fileName;
    }
    qobject_cast<Qt4Project *>(project)->rootProjectNode()
        ->addFiles(UnknownFileType, filesToAddToProject);

    const QString rulesFilePath
        = packagingTemplatesDir + QLatin1String("/debian/rules");
    QFile rulesFile(rulesFilePath);
    if (!rulesFile.open(QIODevice::ReadWrite)) {
        raiseError(tr("Packaging Error: Cannot open file '%1'.")
                   .arg(QDir::toNativeSeparators(rulesFilePath)));
        return false;
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
        return false;
    }
    return true;
}

void MaemoTemplatesManager::handleProjectToBeRemoved(ProjectExplorer::Project *project)
{
    MaemoProjectMap::Iterator it = m_maemoProjects.find(project);
    if (it != m_maemoProjects.end()) {
        delete it.value();
        m_maemoProjects.erase(it);
    }
}

QString MaemoTemplatesManager::version(const Project *project,
    QString *error) const
{
    QSharedPointer<QFile> changeLog
        = openFile(changeLogFilePath(project), QIODevice::ReadOnly, error);
    if (!changeLog)
        return QString();
    const QByteArray &firstLine = changeLog->readLine();
    const int openParenPos = firstLine.indexOf('(');
    if (openParenPos == -1) {
        *error = tr("Debian changelog file '%1' has unexpected format.")
                .arg(QDir::toNativeSeparators(changeLog->fileName()));
        return QString();
    }
    const int closeParenPos = firstLine.indexOf(')', openParenPos);
    if (closeParenPos == -1) {
        *error = tr("Debian changelog file '%1' has unexpected format.")
                .arg(QDir::toNativeSeparators(changeLog->fileName()));
        return QString();
    }
    return QString::fromUtf8(firstLine.mid(openParenPos + 1,
        closeParenPos - openParenPos - 1).data());
}

bool MaemoTemplatesManager::setVersion(const Project *project,
    const QString &version, QString *error) const
{
    QSharedPointer<QFile> changeLog
        = openFile(changeLogFilePath(project), QIODevice::ReadWrite, error);
    if (!changeLog)
        return false;

    QString content = QString::fromUtf8(changeLog->readAll());
    content.replace(QRegExp(QLatin1String("\\([a-zA-Z0-9_\\.]+\\)")),
        QLatin1Char('(') + version + QLatin1Char(')'));
    changeLog->resize(0);
    changeLog->write(content.toUtf8());
    changeLog->close();
    if (changeLog->error() != QFile::NoError) {
        *error = tr("Error writing Debian changelog file '%1': %2")
            .arg(QDir::toNativeSeparators(changeLog->fileName()),
                 changeLog->errorString());
        return false;
    }
    return true;
}

QIcon MaemoTemplatesManager::packageManagerIcon(const Project *project,
    QString *error) const
{
    QSharedPointer<QFile> controlFile
        = openFile(controlFilePath(project), QIODevice::ReadOnly, error);
    if (!controlFile)
        return QIcon();

    bool iconFieldFound = false;
    QByteArray currentLine;
    while (!iconFieldFound && !controlFile->atEnd()) {
        currentLine = controlFile->readLine();
        iconFieldFound = currentLine.startsWith(IconFieldName);
    }
    if (!iconFieldFound)
        return QIcon();

    int pos = IconFieldName.length();
    currentLine = currentLine.trimmed();
    QByteArray base64Icon;
    do {
        while (pos < currentLine.length())
            base64Icon += currentLine.at(pos++);
        do
            currentLine = controlFile->readLine();
        while (currentLine.startsWith('#'));
        if (currentLine.isEmpty() || !isspace(currentLine.at(0)))
            break;
        currentLine = currentLine.trimmed();
        if (currentLine.isEmpty())
            break;
        pos = 0;
    } while (true);
    QPixmap pixmap;
    if (!pixmap.loadFromData(QByteArray::fromBase64(base64Icon))) {
        *error = tr("Invalid icon data in Debian control file.");
        return QIcon();
    }
    return QIcon(pixmap);
}

bool MaemoTemplatesManager::setPackageManagerIcon(const Project *project,
    const QString &iconFilePath, QString *error) const
{
    const QSharedPointer<QFile> controlFile
        = openFile(controlFilePath(project), QIODevice::ReadWrite, error);
    if (!controlFile)
        return false;
    const QPixmap pixmap(iconFilePath);
    if (pixmap.isNull()) {
        *error = tr("Could not read image file '%1'.").arg(iconFilePath);
        return false;
    }

    QByteArray iconAsBase64;
    QBuffer buffer(&iconAsBase64);
    buffer.open(QIODevice::WriteOnly);
    if (!pixmap.scaled(48, 48).save(&buffer,
        QFileInfo(iconFilePath).suffix().toAscii())) {
        *error = tr("Could not export image file '%1'.").arg(iconFilePath);
        return false;
    }
    buffer.close();
    iconAsBase64 = iconAsBase64.toBase64();
    QByteArray contents = controlFile->readAll();
    const int iconFieldPos = contents.startsWith(IconFieldName)
        ? 0 : contents.indexOf('\n' + IconFieldName);
    if (iconFieldPos == -1) {
        if (!contents.endsWith('\n'))
            contents += '\n';
        contents.append(IconFieldName).append(' ').append(iconAsBase64)
            .append('\n');
    } else {
        const int oldIconStartPos
            = (iconFieldPos != 0) + iconFieldPos + IconFieldName.length();
        int nextEolPos = contents.indexOf('\n', oldIconStartPos);
        while (nextEolPos != -1 && nextEolPos != contents.length() - 1
            && contents.at(nextEolPos + 1) != '\n'
            && (contents.at(nextEolPos + 1) == '#'
                || std::isspace(contents.at(nextEolPos + 1))))
            nextEolPos = contents.indexOf('\n', nextEolPos + 1);
        if (nextEolPos == -1)
            nextEolPos = contents.length();
        contents.replace(oldIconStartPos, nextEolPos - oldIconStartPos,
            ' ' + iconAsBase64);
    }
    controlFile->resize(0);
    controlFile->write(contents);
    if (controlFile->error() != QFile::NoError) {
        *error = tr("Error writing file '%1': %2")
            .arg(QDir::toNativeSeparators(controlFile->fileName()),
                controlFile->errorString());
        return false;
    }
    return true;
}

QStringList MaemoTemplatesManager::debianFiles(const Project *project) const
{
    return QDir(debianDirPath(project))
        .entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
}

QString MaemoTemplatesManager::debianDirPath(const Project *project) const
{
    return project->projectDirectory() + QLatin1Char('/')
        + PackagingDirName + QLatin1String("/debian");
}

QString MaemoTemplatesManager::changeLogFilePath(const Project *project) const
{
    return debianDirPath(project) + QLatin1String("/changelog");
}

QString MaemoTemplatesManager::controlFilePath(const Project *project) const
{
    return debianDirPath(project) + QLatin1String("/control");
}

void MaemoTemplatesManager::raiseError(const QString &reason)
{
    QMessageBox::critical(0, tr("Error creating Maemo templates"), reason);
}

void MaemoTemplatesManager::handleDebianFileChanged(const QString &filePath)
{
    const Project * const project
        = findProject(qobject_cast<QFileSystemWatcher *>(sender()));
    if (project) {
        if (filePath == changeLogFilePath(project))
            emit changeLogChanged(project);
        else if (filePath == controlFilePath(project))
            emit controlChanged(project);
    }
}

void MaemoTemplatesManager::handleDebianDirContentsChanged()
{
    const Project * const project
        = findProject(qobject_cast<QFileSystemWatcher *>(sender()));
    if (project)
        emit debianDirContentsChanged(project);
}

QSharedPointer<QFile> MaemoTemplatesManager::openFile(const QString &filePath,
    QIODevice::OpenMode mode, QString *error) const
{
    const QString nativePath = QDir::toNativeSeparators(filePath);
    QSharedPointer<QFile> file(new QFile(filePath));
    if (!file->exists()) {
        *error = tr("File '%1' does not exist").arg(nativePath);
    } else if (!file->open(mode)) {
        *error = tr("Cannot open file '%1': %2")
            .arg(nativePath, file->errorString());
    }
    return file;
}

Project *MaemoTemplatesManager::findProject(const QFileSystemWatcher *fsWatcher) const
{
    for (MaemoProjectMap::ConstIterator it = m_maemoProjects.constBegin();
        it != m_maemoProjects.constEnd(); ++it) {
        if (it.value() == fsWatcher)
            return it.key();
    }
    return 0;
}

} // namespace Internal
} // namespace Qt4ProjectManager
