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

#include "maemodeployablelistmodel.h"
#include "maemodeployables.h"
#include "maemodeploystep.h"
#include "maemoglobal.h"
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
const QByteArray NameFieldName("XB-Maemo-Display-Name");
const QByteArray ShortDescriptionFieldName("Description");
const QLatin1String PackagingDirName("qtc_packaging");
const QLatin1String DebianDirNameFremantle("debian_fremantle");
} // anonymous namespace


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
    connect(session, SIGNAL(projectAdded(ProjectExplorer::Project*)), this,
        SLOT(handleActiveProjectChanged(ProjectExplorer::Project*)));
    connect(session, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
        this, SLOT(handleProjectToBeRemoved(ProjectExplorer::Project*)));
    handleActiveProjectChanged(session->startupProject());
}

void MaemoTemplatesManager::handleActiveProjectChanged(ProjectExplorer::Project *project)
{
    if (!project || m_maemoProjects.contains(project))
        return;

    connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTarget(ProjectExplorer::Target*)));
    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
        this, SLOT(handleTarget(ProjectExplorer::Target*)));
    const QList<Target *> &targets = project->targets();
    foreach (Target * const target, targets)
        handleTarget(target);
}

bool MaemoTemplatesManager::handleTarget(ProjectExplorer::Target *target)
{
    if (!target
        || target->id() != QLatin1String(Constants::MAEMO_DEVICE_TARGET_ID))
        return false;
    if (!createDebianTemplatesIfNecessary(target))
        return false;

    const Qt4Target * const qt4Target = qobject_cast<Qt4Target *>(target);
    const MaemoDeployStep * const deployStep
        = MaemoGlobal::buildStep<MaemoDeployStep>(qt4Target->activeDeployConfiguration());
    connect(deployStep->deployables().data(), SIGNAL(modelReset()), this,
        SLOT(handleProFileUpdated()), Qt::QueuedConnection);

    Project * const project = target->project();
    if (m_maemoProjects.contains(project))
        return true;

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

    return true;
}

bool MaemoTemplatesManager::createDebianTemplatesIfNecessary(const ProjectExplorer::Target *target)
{
    Project * const project = target->project();
    QDir projectDir(project->projectDirectory());
    if (QFileInfo(debianDirPath(project)).exists())
        return true;
    if (!projectDir.exists(PackagingDirName)
            && !projectDir.mkdir(PackagingDirName)) {
        raiseError(tr("Error creating Maemo packaging directory '%1'.")
            .arg(PackagingDirName));
        return false;
    }

    QProcess dh_makeProc;
    QString error;
    const Qt4Target * const qt4Target = qobject_cast<const Qt4Target *>(target);
    Q_ASSERT_X(qt4Target, Q_FUNC_INFO, "Target ID does not match actual type.");
    const Qt4BuildConfiguration * const bc
        = qt4Target->activeBuildConfiguration();
    const MaemoToolChain * const tc
        = dynamic_cast<MaemoToolChain *>(bc->toolChain());
    if (!tc) {
        qDebug("Maemo target has no Maemo toolchain.");
        return false;
    }
    if (!MaemoPackageCreationStep::preparePackagingProcess(&dh_makeProc, bc,
        projectDir.path() + QLatin1Char('/') + PackagingDirName, &error)) {
        raiseError(error);
        return false;
    }

    const QString dhMakeDebianDir = projectDir.path() + QLatin1Char('/')
        + PackagingDirName + QLatin1String("/debian");
    QString removeError;
    MaemoGlobal::removeRecursively(dhMakeDebianDir, removeError);
    const QString command = QLatin1String("dh_make -s -n -p ")
        + MaemoPackageCreationStep::packageName(project) + QLatin1Char('_')
        + MaemoPackageCreationStep::DefaultVersionNumber;
    dh_makeProc.start(MaemoPackageCreationStep::packagingCommand(tc, command));
    if (!dh_makeProc.waitForStarted()) {
        raiseError(tr("Unable to create Debian templates: dh_make failed (%1)")
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

    if (!QFile::rename(dhMakeDebianDir, debianDirPath(project))) {
        raiseError(tr("Unable to move new debian directory to '%1'.")
            .arg(QDir::toNativeSeparators(debianDirPath(project))));
        MaemoGlobal::removeRecursively(dhMakeDebianDir, removeError);
        return false;
    }

    QDir debianDir(debianDirPath(project));
    const QStringList &files = debianDir.entryList(QDir::Files);
    foreach (const QString &fileName, files) {
        if (fileName.endsWith(QLatin1String(".ex"), Qt::CaseInsensitive)
            || fileName.compare(QLatin1String("README.debian"), Qt::CaseInsensitive) == 0
            || fileName.compare(QLatin1String("dirs"), Qt::CaseInsensitive) == 0
            || fileName.compare(QLatin1String("docs"), Qt::CaseInsensitive) == 0) {
            debianDir.remove(fileName);
        }
    }

    return adaptRulesFile(project) && adaptControlFile(project);
}

bool MaemoTemplatesManager::adaptRulesFile(const Project *project)
{
    const QString rulesFilePath = debianDirPath(project) + "/rules";
    QFile rulesFile(rulesFilePath);
    if (!rulesFile.open(QIODevice::ReadWrite)) {
        raiseError(tr("Packaging Error: Cannot open file '%1'.")
                   .arg(QDir::toNativeSeparators(rulesFilePath)));
        return false;
    }
    QByteArray rulesContents = rulesFile.readAll();
    rulesContents.replace("DESTDIR", "INSTALL_ROOT");
    rulesContents.replace("dh_shlibdeps", "# dh_shlibdeps");
//    rulesContents.replace("$(MAKE) clean", "# $(MAKE) clean");
//    const Qt4Project * const qt4Project
//        = static_cast<const Qt4Project *>(project);
//    const QString proFileName
//        = QFileInfo(qt4Project->rootProjectNode()->path()).fileName();
//    rulesContents.replace("# Add here commands to configure the package.",
//        "qmake " + proFileName.toLocal8Bit());

    // Would be the right solution, but does not work (on Windows),
    // because dpkg-genchanges doesn't know about it (and can't be told).
    // rulesContents.replace("dh_builddeb", "dh_builddeb --destdir=.");

    rulesFile.resize(0);
    rulesFile.write(rulesContents);
    rulesFile.close();
    if (rulesFile.error() != QFile::NoError) {
        raiseError(tr("Packaging Error: Cannot write file '%1'.")
                   .arg(QDir::toNativeSeparators(rulesFilePath)));
        return false;
    }
    return true;
}

bool MaemoTemplatesManager::adaptControlFile(const Project *project)
{
    QFile controlFile(controlFilePath(project));
    if (!controlFile.open(QIODevice::ReadWrite)) {
        raiseError(tr("Packaging Error: Cannot open file '%1'.")
                   .arg(QDir::toNativeSeparators(controlFilePath(project))));
        return false;
    }

    QByteArray controlContents = controlFile.readAll();

    adaptControlFileField(controlContents, "Section", "user/hidden");
    adaptControlFileField(controlContents, "Priority", "optional");
    adaptControlFileField(controlContents, NameFieldName,
        project->displayName().toUtf8());
    const int buildDependsOffset = controlContents.indexOf("Build-Depends:");
    if (buildDependsOffset == -1) {
        qDebug("Unexpected: no Build-Depends field in debian control file.");
    } else {
        int buildDependsNewlineOffset
            = controlContents.indexOf('\n', buildDependsOffset);
        if (buildDependsNewlineOffset == -1) {
            controlContents += '\n';
            buildDependsNewlineOffset = controlContents.length() - 1;
        }
        controlContents.insert(buildDependsNewlineOffset,
            ", libqt4-dev");
    }

    controlFile.resize(0);
    controlFile.write(controlContents);
    controlFile.close();
    if (controlFile.error() != QFile::NoError) {
        raiseError(tr("Packaging Error: Cannot write file '%1'.")
                   .arg(QDir::toNativeSeparators(controlFilePath(project))));
        return false;
    }
    return true;
}

bool MaemoTemplatesManager::adaptControlFileField(QByteArray &document,
    const QByteArray &fieldName, const QByteArray &newFieldValue)
{
    QByteArray adaptedLine = fieldName + ": " + newFieldValue;
    const int lineOffset = document.indexOf(fieldName + ":");
    if (lineOffset == -1) {
        document.append(adaptedLine).append('\n');
        return true;
    }

    int newlineOffset = document.indexOf('\n', lineOffset);
    bool updated = false;
    if (newlineOffset == -1) {
        newlineOffset = document.length();
        adaptedLine += '\n';
        updated = true;
    }
    const int replaceCount = newlineOffset - lineOffset;
    if (!updated && document.mid(lineOffset, replaceCount) != adaptedLine)
        updated = true;
    if (updated)
        document.replace(lineOffset, replaceCount, adaptedLine);
    return updated;
}

bool MaemoTemplatesManager::updateDesktopFiles(const Qt4Target *target)
{
    const Qt4Target * const qt4Target = qobject_cast<const Qt4Target *>(target);
    Q_ASSERT_X(qt4Target, Q_FUNC_INFO,
        "Impossible: Target has Maemo id, but could not be cast to Qt4Target.");
    const QList<Qt4ProFileNode *> &applicationProjects
        = qt4Target->qt4Project()->applicationProFiles();
    bool success = true;
    foreach (Qt4ProFileNode *proFileNode, applicationProjects)
        success &= updateDesktopFile(qt4Target, proFileNode);
    return success;
}

bool MaemoTemplatesManager::updateDesktopFile(const Qt4Target *target,
    Qt4ProFileNode *proFileNode)
{
    const QString appName = proFileNode->targetInformation().target;
    const QString desktopFilePath = QFileInfo(proFileNode->path()).path()
        + QLatin1Char('/') + appName + QLatin1String(".desktop");
    QFile desktopFile(desktopFilePath);
    const bool existsAlready = desktopFile.exists();
    if (!desktopFile.open(QIODevice::ReadWrite)) {
        qWarning("Failed to open '%s': %s", qPrintable(desktopFilePath),
            qPrintable(desktopFile.errorString()));
        return false;
    }

    const QByteArray desktopTemplate("[Desktop Entry]\nEncoding=UTF-8\n"
        "Version=1.0\nType=Application\nTerminal=false\nName=\nExec=\n"
        "Icon=\nX-Window-Icon=\nX-HildonDesk-ShowInToolbar=true\n"
        "X-Osso-Type=application/x-executable\n");
    QByteArray desktopFileContents
        = existsAlready ? desktopFile.readAll() : desktopTemplate;

    QString executable;
    const QSharedPointer<MaemoDeployables> &deployables
        = MaemoGlobal::buildStep<MaemoDeployStep>(target->activeDeployConfiguration())
            ->deployables();
    for (int i = 0; i < deployables->modelCount(); ++i) {
        const MaemoDeployableListModel * const model = deployables->modelAt(i);
        if (model->proFilePath() == proFileNode->path()) {
            executable = model->remoteExecutableFilePath();
            break;
        }
    }
    if (executable.isEmpty()) {
        qWarning("Strange: Project file node not managed by MaemoDeployables.");
    } else {
        int execNewLinePos, execValuePos;
        findLine("Exec=", desktopFileContents, execNewLinePos, execValuePos);
        desktopFileContents.replace(execValuePos, execNewLinePos - execValuePos,
            executable.toUtf8());
    }

    int nameNewLinePos, nameValuePos;
    findLine("Name=", desktopFileContents, nameNewLinePos, nameValuePos);
    if (nameNewLinePos == nameValuePos)
        desktopFileContents.insert(nameValuePos, appName.toUtf8());
    int iconNewLinePos, iconValuePos;
    findLine("Icon=", desktopFileContents, iconNewLinePos, iconValuePos);
    if (iconNewLinePos == iconValuePos)
        desktopFileContents.insert(iconValuePos, appName.toUtf8());

    desktopFile.resize(0);
    desktopFile.write(desktopFileContents);
    desktopFile.close();
    if (desktopFile.error() != QFile::NoError) {
        qWarning("Could not write '%s': %s", qPrintable(desktopFilePath),
            qPrintable(desktopFile.errorString()));
    }

    if (!existsAlready) {
        proFileNode->addFiles(UnknownFileType,
            QStringList() << desktopFilePath);
        QFile proFile(proFileNode->path());
        if (!proFile.open(QIODevice::ReadWrite)) {
            qWarning("Failed to open '%s': %s", qPrintable(proFileNode->path()),
                qPrintable(proFile.errorString()));
            return false;
        }
        QByteArray proFileContents = proFile.readAll();
        proFileContents += "\nunix:!symbian {\n"
            "    desktopfile.files = $${TARGET}.desktop\n"
            "    maemo5 {\n"
            "        desktopfile.path = /usr/share/applications/hildon\n"
            "    } else {\n"
            "        desktopfile.path = /usr/share/applications\n    }\n"
            "    INSTALLS += desktopfile\n}\n";
        proFile.resize(0);
        proFile.write(proFileContents);
        proFile.close();
        if (proFile.error() != QFile::NoError) {
            qWarning("Could not write '%s': %s", qPrintable(proFileNode->path()),
                qPrintable(proFile.errorString()));
            return false;
        }
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

void MaemoTemplatesManager::handleProFileUpdated()
{
    const MaemoDeployables * const deployables
        = qobject_cast<MaemoDeployables *>(sender());
    if (!deployables)
        return;
//    const Target * const target = deployables->buildStep()->target();
//    if (m_maemoProjects.contains(target->project()))
//        updateDesktopFiles(qobject_cast<const Qt4Target *>(target));
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

QString MaemoTemplatesManager::name(const Project *project) const
{
    return controlFileFieldValue(project, NameFieldName);
}

bool MaemoTemplatesManager::setName(const Project *project, const QString &name)
{
    return setFieldValue(project, NameFieldName, name.toUtf8());
}

QString MaemoTemplatesManager::shortDescription(const Project *project) const
{
    return controlFileFieldValue(project, ShortDescriptionFieldName);
}

bool MaemoTemplatesManager::setShortDescription(const Project *project,
    const QString &description)
{
    return setFieldValue(project, ShortDescriptionFieldName,
        description.toUtf8());
}

bool MaemoTemplatesManager::setFieldValue(const Project *project,
    const QByteArray &fieldName, const QByteArray &fieldValue)
{
    QFile controlFile(controlFilePath(project));
    if (!controlFile.open(QIODevice::ReadWrite))
        return false;
    QByteArray contents = controlFile.readAll();
    if (adaptControlFileField(contents, fieldName, fieldValue)) {
        controlFile.resize(0);
        controlFile.write(contents);
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
    return project->projectDirectory() + QLatin1Char('/') + PackagingDirName
        + QLatin1Char('/') + DebianDirNameFremantle;
}

QString MaemoTemplatesManager::changeLogFilePath(const Project *project) const
{
    return debianDirPath(project) + QLatin1String("/changelog");
}

QString MaemoTemplatesManager::controlFilePath(const Project *project) const
{
    return debianDirPath(project) + QLatin1String("/control");
}

QString MaemoTemplatesManager::controlFileFieldValue(const Project *project,
    const QString &key) const
{
    QFile controlFile(controlFilePath(project));
    if (!controlFile.open(QIODevice::ReadOnly))
        return QString();
    const QByteArray &contents = controlFile.readAll();
    const int keyPos = contents.indexOf(key.toUtf8() + ':');
    if (keyPos == -1)
        return QString();
    const int valueStartPos = keyPos + key.length() + 1;
    int valueEndPos = contents.indexOf('\n', keyPos);
    if (valueEndPos == -1)
        valueEndPos = contents.count();
    return QString::fromUtf8(contents.mid(valueStartPos,
        valueEndPos - valueStartPos)).trimmed();
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

void MaemoTemplatesManager::findLine(const QByteArray &string,
    QByteArray &document, int &lineEndPos, int &valuePos)
{
    int lineStartPos = document.indexOf(string);
    if (lineStartPos == -1) {
        lineStartPos = document.length();
        document += string + '\n';
    }
    valuePos = lineStartPos + string.length();
    lineEndPos = document.indexOf('\n', lineStartPos);
    if (lineEndPos == -1) {
        lineEndPos = document.length();
        document += '\n';
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
