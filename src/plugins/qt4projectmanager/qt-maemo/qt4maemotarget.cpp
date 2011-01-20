/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qt4maemotarget.h"

#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemorunconfiguration.h"
#include "qt4maemodeployconfiguration.h"

#include <projectexplorer/customexecutablerunconfiguration.h>
#include <qt4projectmanager/qt4project.h>

#include <QtGui/QApplication>
#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QIcon>
#include <QtGui/QMessageBox>

#include <cctype>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

namespace {
const QByteArray IconFieldName("XB-Maemo-Icon-26");
const QByteArray NameFieldName("XB-Maemo-Display-Name");
const QByteArray ShortDescriptionFieldName("Description");
const QLatin1String PackagingDirName("qtc_packaging");
} // anonymous namespace


AbstractQt4MaemoTarget::AbstractQt4MaemoTarget(Qt4Project *parent, const QString &id) :
    Qt4BaseTarget(parent, id),
    m_buildConfigurationFactory(new Qt4BuildConfigurationFactory(this)),
    m_deployConfigurationFactory(new Qt4MaemoDeployConfigurationFactory(this)),
    m_debianFilesWatcher(new QFileSystemWatcher(this))
{
    setIcon(QIcon(":/projectexplorer/images/MaemoDevice.png"));
    connect(parent, SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
}

AbstractQt4MaemoTarget::~AbstractQt4MaemoTarget()
{

}

Qt4BuildConfigurationFactory *AbstractQt4MaemoTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

ProjectExplorer::DeployConfigurationFactory *AbstractQt4MaemoTarget::deployConfigurationFactory() const
{
    return m_deployConfigurationFactory;
}

QString AbstractQt4MaemoTarget::defaultBuildDirectory() const
{
    //TODO why?
#if defined(Q_OS_WIN)
    return project()->projectDirectory();
#endif
    return Qt4BaseTarget::defaultBuildDirectory();
}

void AbstractQt4MaemoTarget::createApplicationProFiles()
{
    removeUnconfiguredCustomExectutableRunConfigurations();

    QList<Qt4ProFileNode *> profiles = qt4Project()->applicationProFiles();
    QSet<QString> paths;
    foreach (Qt4ProFileNode *pro, profiles)
        paths << pro->path();

    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (MaemoRunConfiguration *qt4rc = qobject_cast<MaemoRunConfiguration *>(rc))
            paths.remove(qt4rc->proFilePath());

    // Only add new runconfigurations if there are none.
    foreach (const QString &path, paths)
        addRunConfiguration(new MaemoRunConfiguration(this, path));

    // Oh still none? Add a custom executable runconfiguration
    if (runConfigurations().isEmpty()) {
        addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(this));
    }
}

QList<ProjectExplorer::RunConfiguration *> AbstractQt4MaemoTarget::runConfigurationsForNode(ProjectExplorer::Node *n)
{
    QList<ProjectExplorer::RunConfiguration *> result;
    foreach (ProjectExplorer::RunConfiguration *rc, runConfigurations())
        if (MaemoRunConfiguration *mrc = qobject_cast<MaemoRunConfiguration *>(rc))
            if (mrc->proFilePath() == n->path())
                result << rc;
    return result;
}

QString AbstractQt4MaemoTarget::projectVersion(QString *error) const
{
    QSharedPointer<QFile> changeLog = openFile(changeLogFilePath(),
        QIODevice::ReadOnly, error);
    if (!changeLog)
        return QString();
    const QByteArray &firstLine = changeLog->readLine();
    const int openParenPos = firstLine.indexOf('(');
    if (openParenPos == -1) {
        if (error) {
            *error = tr("Debian changelog file '%1' has unexpected format.")
                .arg(QDir::toNativeSeparators(changeLog->fileName()));
        }
        return QString();
    }
    const int closeParenPos = firstLine.indexOf(')', openParenPos);
    if (closeParenPos == -1) {
        if (error) {
            *error = tr("Debian changelog file '%1' has unexpected format.")
                .arg(QDir::toNativeSeparators(changeLog->fileName()));
        }
        return QString();
    }
    return QString::fromUtf8(firstLine.mid(openParenPos + 1,
        closeParenPos - openParenPos - 1).data());
}

bool AbstractQt4MaemoTarget::setProjectVersion(const QString &version,
    QString *error)
{
    bool success = true;
    foreach (Target * const target, project()->targets()) {
        AbstractQt4MaemoTarget * const maemoTarget
            = qobject_cast<AbstractQt4MaemoTarget *>(target);
        if (maemoTarget) {
            if (!maemoTarget->setProjectVersionInternal(version, error))
                success = false;
        }
    }
    return success;
}

bool AbstractQt4MaemoTarget::setProjectVersionInternal(const QString &version,
    QString *error)
{
    const QString filePath = changeLogFilePath();
    MaemoGlobal::FileUpdate update(filePath);
    QSharedPointer<QFile> changeLog
        = openFile(filePath, QIODevice::ReadWrite, error);
    if (!changeLog)
        return false;

    QString content = QString::fromUtf8(changeLog->readAll());
    content.replace(QRegExp(QLatin1String("\\([a-zA-Z0-9_\\.]+\\)")),
        QLatin1Char('(') + version + QLatin1Char(')'));
    changeLog->resize(0);
    changeLog->write(content.toUtf8());
    changeLog->close();
    if (changeLog->error() != QFile::NoError) {
        if (error) {
            *error = tr("Error writing Debian changelog file '%1': %2")
                .arg(QDir::toNativeSeparators(changeLog->fileName()),
                     changeLog->errorString());
        }
        return false;
    }
    return true;
}

QIcon AbstractQt4MaemoTarget::packageManagerIcon(QString *error) const
{
    const QByteArray &base64Icon = controlFileFieldValue(IconFieldName, true);
    if (base64Icon.isEmpty())
        return QIcon();
    QPixmap pixmap;
    if (!pixmap.loadFromData(QByteArray::fromBase64(base64Icon))) {
        if (error)
            *error = tr("Invalid icon data in Debian control file.");
        return QIcon();
    }
    return QIcon(pixmap);
}

bool AbstractQt4MaemoTarget::setPackageManagerIcon(const QString &iconFilePath,
    QString *error)
{
    bool success = true;
    foreach (Target * const target, project()->targets()) {
        AbstractQt4MaemoTarget * const maemoTarget
            = qobject_cast<AbstractQt4MaemoTarget *>(target);
        if (maemoTarget) {
            if (!maemoTarget->setPackageManagerIconInternal(iconFilePath, error))
                success = false;
        }
    }
    return success;
}

bool AbstractQt4MaemoTarget::setPackageManagerIconInternal(const QString &iconFilePath,
    QString *error)
{
    const QString filePath = controlFilePath();
    MaemoGlobal::FileUpdate update(filePath);
    const QSharedPointer<QFile> controlFile
        = openFile(filePath, QIODevice::ReadWrite, error);
    if (!controlFile)
        return false;
    const QPixmap pixmap(iconFilePath);
    if (pixmap.isNull()) {
        if (error)
            *error = tr("Could not read image file '%1'.").arg(iconFilePath);
        return false;
    }

    QByteArray iconAsBase64;
    QBuffer buffer(&iconAsBase64);
    buffer.open(QIODevice::WriteOnly);
    if (!pixmap.scaled(48, 48).save(&buffer,
        QFileInfo(iconFilePath).suffix().toAscii())) {
        if (error)
            *error = tr("Could not export image file '%1'.").arg(iconFilePath);
        return false;
    }
    buffer.close();
    iconAsBase64 = iconAsBase64.toBase64();
    QByteArray contents = controlFile->readAll();
    const QByteArray iconFieldNameWithColon = IconFieldName + ':';
    const int iconFieldPos = contents.startsWith(iconFieldNameWithColon)
        ? 0 : contents.indexOf('\n' + iconFieldNameWithColon);
    if (iconFieldPos == -1) {
        if (!contents.endsWith('\n'))
            contents += '\n';
        contents.append(iconFieldNameWithColon).append(' ').append(iconAsBase64)
            .append('\n');
    } else {
        const int oldIconStartPos = (iconFieldPos != 0) + iconFieldPos
            + iconFieldNameWithColon.length();
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
        if (error) {
            *error = tr("Error writing file '%1': %2")
                .arg(QDir::toNativeSeparators(controlFile->fileName()),
                    controlFile->errorString());
        }
        return false;
    }
    return true;
}

QString AbstractQt4MaemoTarget::name() const
{
    return QString::fromUtf8(controlFileFieldValue(NameFieldName, false));
}

bool AbstractQt4MaemoTarget::setName(const QString &name)
{
    bool success = true;
    foreach (Target * const target, project()->targets()) {
        AbstractQt4MaemoTarget * const maemoTarget
            = qobject_cast<AbstractQt4MaemoTarget *>(target);
        if (maemoTarget) {
            if (!maemoTarget->setNameInternal(name))
                success = false;
        }
    }
    return success;
}

bool AbstractQt4MaemoTarget::setNameInternal(const QString &name)
{
    return setControlFieldValue(NameFieldName, name.toUtf8());
}

QString AbstractQt4MaemoTarget::shortDescription() const
{
    return QString::fromUtf8(controlFileFieldValue(ShortDescriptionFieldName, false));
}

bool AbstractQt4MaemoTarget::setShortDescription(const QString &description)
{
    bool success = true;
    foreach (Target * const target, project()->targets()) {
        AbstractQt4MaemoTarget * const maemoTarget
            = qobject_cast<AbstractQt4MaemoTarget *>(target);
        if (maemoTarget) {
            if (!maemoTarget->setShortDescriptionInternal(description))
                success = false;
        }
    }
    return success;
}

bool AbstractQt4MaemoTarget::setShortDescriptionInternal(const QString &description)
{
    return setControlFieldValue(ShortDescriptionFieldName, description.toUtf8());
}

QString AbstractQt4MaemoTarget::debianDirPath() const
{
    return project()->projectDirectory() + QLatin1Char('/') + PackagingDirName
        + QLatin1Char('/') + debianDirName();
}

QStringList AbstractQt4MaemoTarget::debianFiles() const
{
    return QDir(debianDirPath())
        .entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
}

QString AbstractQt4MaemoTarget::changeLogFilePath() const
{
    return debianDirPath() + QLatin1String("/changelog");
}

QString AbstractQt4MaemoTarget::controlFilePath() const
{
    return debianDirPath() + QLatin1String("/control");
}

QSharedPointer<QFile> AbstractQt4MaemoTarget::openFile(const QString &filePath,
    QIODevice::OpenMode mode, QString *error) const
{
    const QString nativePath = QDir::toNativeSeparators(filePath);
    QSharedPointer<QFile> file(new QFile(filePath));
    if (!file->exists()) {
        if (error)
            *error = tr("File '%1' does not exist").arg(nativePath);
        file.clear();
    } else if (!file->open(mode)) {
        if (error) {
            *error = tr("Cannot open file '%1': %2")
                .arg(nativePath, file->errorString());
        }
        file.clear();
    }
    return file;
}

QByteArray AbstractQt4MaemoTarget::controlFileFieldValue(const QString &key,
    bool multiLine) const
{
    QByteArray value;
    QFile controlFile(controlFilePath());
    if (!controlFile.open(QIODevice::ReadOnly))
        return value;
    const QByteArray &contents = controlFile.readAll();
    const int keyPos = contents.indexOf(key.toUtf8() + ':');
    if (keyPos == -1)
        return value;
    int valueStartPos = keyPos + key.length() + 1;
    int valueEndPos = contents.indexOf('\n', keyPos);
    if (valueEndPos == -1)
        valueEndPos = contents.count();
    value = contents.mid(valueStartPos, valueEndPos - valueStartPos).trimmed();
    if (multiLine) {
        Q_FOREVER {
            valueStartPos = valueEndPos + 1;
            if (valueStartPos >= contents.count())
                break;
            const char firstChar = contents.at(valueStartPos);
            if (firstChar == '#' || isspace(firstChar)) {
                valueEndPos = contents.indexOf('\n', valueStartPos);
                if (valueEndPos == -1)
                    valueEndPos = contents.count();
                if (firstChar != '#') {
                    value += contents.mid(valueStartPos,
                        valueEndPos - valueStartPos).trimmed();
                }
            } else {
                break;
            }
        }
    }
    return value;
}

bool AbstractQt4MaemoTarget::setControlFieldValue(const QByteArray &fieldName,
    const QByteArray &fieldValue)
{
    QFile controlFile(controlFilePath());
    MaemoGlobal::FileUpdate update(controlFile.fileName());
    if (!controlFile.open(QIODevice::ReadWrite))
        return false;
    QByteArray contents = controlFile.readAll();
    if (adaptControlFileField(contents, fieldName, fieldValue)) {
        controlFile.resize(0);
        controlFile.write(contents);
    }
    return true;
}

bool AbstractQt4MaemoTarget::adaptControlFileField(QByteArray &document,
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

void AbstractQt4MaemoTarget::handleTargetAdded(ProjectExplorer::Target *target)
{
    if (target != this)
        return;

    disconnect(project(), SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
    connect(project(), SIGNAL(aboutToRemoveTarget(ProjectExplorer::Target*)),
        SLOT(handleTargetToBeRemoved(ProjectExplorer::Target*)));
    if (!createDebianTemplatesIfNecessary())
        return;
    initPackagingSettingsFromOtherTarget();

    m_debianFilesWatcher->addPath(debianDirPath());
    m_debianFilesWatcher->addPath(changeLogFilePath());
    m_debianFilesWatcher->addPath(controlFilePath());
    connect(m_debianFilesWatcher, SIGNAL(directoryChanged(QString)), this,
        SLOT(handleDebianDirContentsChanged()));
    connect(m_debianFilesWatcher, SIGNAL(fileChanged(QString)), this,
        SLOT(handleDebianFileChanged(QString)));
    handleDebianDirContentsChanged();
    handleDebianFileChanged(changeLogFilePath());
    handleDebianFileChanged(controlFilePath());
}

void AbstractQt4MaemoTarget::handleTargetToBeRemoved(ProjectExplorer::Target *target)
{
    if (target != this)
        return;
    const QString debianPath = debianDirPath();
    if (!QFileInfo(debianPath).exists())
        return;
    const int answer = QMessageBox::warning(0, tr("Qt Creator"),
        tr("Do you want to remove the packaging directory\n"
           "associated with the target '%1'?").arg(displayName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::No)
        return;
    QString error;
    if (!MaemoGlobal::removeRecursively(debianPath, error))
        qDebug("%s", qPrintable(error));
    const QString packagingPath = project()->projectDirectory()
        + QLatin1Char('/') + PackagingDirName;
    const QStringList otherContents = QDir(packagingPath).entryList(QDir::Dirs
        | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
    if (otherContents.isEmpty()) {
        if (!MaemoGlobal::removeRecursively(packagingPath, error))
            qDebug("%s", qPrintable(error));
    }
}

bool AbstractQt4MaemoTarget::createDebianTemplatesIfNecessary()
{
    if (QFileInfo(debianDirPath()).exists())
        return true;
    QDir projectDir(project()->projectDirectory());
    if (!projectDir.exists(PackagingDirName)
            && !projectDir.mkdir(PackagingDirName)) {
        raiseError(tr("Error creating Maemo packaging directory '%1'.")
            .arg(PackagingDirName));
        return false;
    }

    QProcess dh_makeProc;
    QString error;
    const Qt4BuildConfiguration * const bc = activeBuildConfiguration();
    if (!MaemoPackageCreationStep::preparePackagingProcess(&dh_makeProc, bc,
        projectDir.path() + QLatin1Char('/') + PackagingDirName, &error)) {
        raiseError(error);
        return false;
    }

    const QString dhMakeDebianDir = projectDir.path() + QLatin1Char('/')
        + PackagingDirName + QLatin1String("/debian");
    MaemoGlobal::removeRecursively(dhMakeDebianDir, error);
    const QString command = QLatin1String("dh_make -s -n -p ")
        + MaemoPackageCreationStep::packageName(project()) + QLatin1Char('_')
        + MaemoPackageCreationStep::DefaultVersionNumber;
    dh_makeProc.start(MaemoPackageCreationStep::packagingCommand(bc, command));
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

    if (!QFile::rename(dhMakeDebianDir, debianDirPath())) {
        raiseError(tr("Unable to move new debian directory to '%1'.")
            .arg(QDir::toNativeSeparators(debianDirPath())));
        MaemoGlobal::removeRecursively(dhMakeDebianDir, error);
        return false;
    }

    QDir debianDir(debianDirPath());
    const QStringList &files = debianDir.entryList(QDir::Files);
    foreach (const QString &fileName, files) {
        if (fileName.endsWith(QLatin1String(".ex"), Qt::CaseInsensitive)
            || fileName.compare(QLatin1String("README.debian"), Qt::CaseInsensitive) == 0
            || fileName.compare(QLatin1String("dirs"), Qt::CaseInsensitive) == 0
            || fileName.compare(QLatin1String("docs"), Qt::CaseInsensitive) == 0) {
            debianDir.remove(fileName);
        }
    }

    return adaptRulesFile() && adaptControlFile();
}

bool AbstractQt4MaemoTarget::adaptRulesFile()
{
    const QString rulesFilePath = debianDirPath() + "/rules";
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

bool AbstractQt4MaemoTarget::adaptControlFile()
{
    QFile controlFile(controlFilePath());
    if (!controlFile.open(QIODevice::ReadWrite)) {
        raiseError(tr("Packaging Error: Cannot open file '%1'.")
                   .arg(QDir::toNativeSeparators(controlFilePath())));
        return false;
    }

    QByteArray controlContents = controlFile.readAll();

    adaptControlFileField(controlContents, "Section", "user/hidden");
    adaptControlFileField(controlContents, "Priority", "optional");
    adaptControlFileField(controlContents, NameFieldName,
        project()->displayName().toUtf8());
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
                   .arg(QDir::toNativeSeparators(controlFilePath())));
        return false;
    }
    return true;
}

bool AbstractQt4MaemoTarget::initPackagingSettingsFromOtherTarget()
{
    bool success = true;
    foreach (const Target * const target, project()->targets()) {
        const AbstractQt4MaemoTarget * const maemoTarget
            = qobject_cast<const AbstractQt4MaemoTarget *>(target);
        if (maemoTarget && maemoTarget != this) {
            if (!setProjectVersionInternal(maemoTarget->projectVersion()))
                success = false;
            if (!setControlFieldValue(IconFieldName, maemoTarget->controlFileFieldValue(IconFieldName, true)))
                success = false;
            if (!setNameInternal(maemoTarget->name()))
                success = false;
            if (!setShortDescriptionInternal(maemoTarget->shortDescription()))
                success = false;
            break;
        }
    }
    return success;
}

void AbstractQt4MaemoTarget::handleDebianFileChanged(const QString &filePath)
{
    if (filePath == changeLogFilePath())
        emit changeLogChanged();
    else if (filePath == controlFilePath())
        emit controlChanged();
}

void AbstractQt4MaemoTarget::handleDebianDirContentsChanged()
{
    emit debianDirContentsChanged();
}

void AbstractQt4MaemoTarget::raiseError(const QString &reason)
{
    QMessageBox::critical(0, tr("Error creating Maemo templates"), reason);
}


Qt4Maemo5Target::Qt4Maemo5Target(Qt4Project *parent, const QString &id)
        : AbstractQt4MaemoTarget(parent, id)
{
    setDisplayName(defaultDisplayName());
}

Qt4Maemo5Target::~Qt4Maemo5Target() {}

QString Qt4Maemo5Target::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Maemo5",
        "Qt4 Maemo5 target display name");
}

QString Qt4Maemo5Target::debianDirName() const
{
    return QLatin1String("debian_fremantle");
}

Qt4HarmattanTarget::Qt4HarmattanTarget(Qt4Project *parent,
    const QString &id) : AbstractQt4MaemoTarget(parent, id)
{
    setDisplayName(defaultDisplayName());
}

Qt4HarmattanTarget::~Qt4HarmattanTarget() {}

QString Qt4HarmattanTarget::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Harmattan",
        "Qt4 Harmattan target display name");
}

QString Qt4HarmattanTarget::debianDirName() const
{
    return QLatin1String("debian_harmattan");
}
