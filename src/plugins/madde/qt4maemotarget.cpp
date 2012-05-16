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

#include "qt4maemotarget.h"

#include "maemoconstants.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"
#include "maemorunconfiguration.h"
#include "maemotoolchain.h"
#include "qt4maemodeployconfiguration.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/toolchain.h>
#include <qt4projectmanager/qt4project.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4nodes.h>
#include <qtsupport/baseqtversion.h>
#include <remotelinux/deploymentsettingsassistant.h>
#include <utils/fileutils.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QMainWindow>
#include <QBuffer>
#include <QDateTime>
#include <QLocale>
#include <QRegExp>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QIcon>
#include <QMessageBox>

#include <cctype>

using namespace Qt4ProjectManager;
using namespace RemoteLinux;

namespace Madde {
namespace Internal {

namespace {
const QByteArray NameFieldName("Package");
const QByteArray IconFieldName("XB-Maemo-Icon-26");
const QByteArray ShortDescriptionFieldName("Description");
const QByteArray PackageFieldName("Package");
const QLatin1String PackagingDirName("qtc_packaging");
const QByteArray NameTag("Name");
const QByteArray SummaryTag("Summary");
const QByteArray VersionTag("Version");
const QByteArray ReleaseTag("Release");

bool adaptTagValue(QByteArray &document, const QByteArray &fieldName,
    const QByteArray &newFieldValue, bool caseSensitive)
{
    QByteArray adaptedLine = fieldName + ": " + newFieldValue;
    const QByteArray completeTag = fieldName + ':';
    const int lineOffset = caseSensitive ? document.indexOf(completeTag)
        : document.toLower().indexOf(completeTag.toLower());
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


} // anonymous namespace


AbstractQt4MaemoTarget::AbstractQt4MaemoTarget(Qt4Project *parent, const Core::Id id,
        const QString &qmakeScope) :
    AbstractEmbeddedLinuxTarget(parent, id),
    m_filesWatcher(new Utils::FileSystemWatcher(this)),
    m_deploymentSettingsAssistant(new DeploymentSettingsAssistant(qmakeScope,
        QLatin1String("/opt"), deploymentInfo())),
    m_isInitialized(false)
{
    m_filesWatcher->setObjectName(QLatin1String("Qt4MaemoTarget"));
    setIcon(QIcon(":/projectexplorer/images/MaemoDevice.png"));
    connect(parent, SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
    connect(parent, SIGNAL(fromMapFinished()),
        this, SLOT(handleFromMapFinished()));
}

AbstractQt4MaemoTarget::~AbstractQt4MaemoTarget()
{ }

QList<ProjectExplorer::ToolChain *> AbstractQt4MaemoTarget::possibleToolChains(ProjectExplorer::BuildConfiguration *bc) const
{
    QList<ProjectExplorer::ToolChain *> result;

    Qt4BuildConfiguration *qt4Bc = qobject_cast<Qt4BuildConfiguration *>(bc);
    if (!qt4Bc)
        return result;

    QList<ProjectExplorer::ToolChain *> candidates = Qt4BaseTarget::possibleToolChains(bc);
    foreach (ProjectExplorer::ToolChain *i, candidates) {
        MaemoToolChain *tc = dynamic_cast<MaemoToolChain *>(i);
        if (!tc || !qt4Bc->qtVersion())
            continue;
        if (tc->qtVersionId() == qt4Bc->qtVersion()->uniqueId())
            result.append(tc);
    }

    return result;
}

void AbstractQt4MaemoTarget::createApplicationProFiles(bool reparse)
{
    if (!reparse)
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

bool AbstractQt4MaemoTarget::setPackageName(const QString &name)
{
    bool success = true;
    foreach (Target * const target, project()->targets()) {
        AbstractQt4MaemoTarget * const maemoTarget
            = qobject_cast<AbstractQt4MaemoTarget *>(target);
        if (maemoTarget) {
            if (!maemoTarget->setPackageNameInternal(name))
                success = false;
        }
    }
    return success;
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

QSharedPointer<QFile> AbstractQt4MaemoTarget::openFile(const QString &filePath,
    QIODevice::OpenMode mode, QString *error) const
{
    const QString nativePath = QDir::toNativeSeparators(filePath);
    QSharedPointer<QFile> file(new QFile(filePath));
    if (!file->open(mode)) {
        if (error) {
            *error = tr("Cannot open file '%1': %2")
                .arg(nativePath, file->errorString());
        }
        file.clear();
    }
    return file;
}

void AbstractQt4MaemoTarget::handleFromMapFinished()
{
    handleTargetAdded(this);
}

void AbstractQt4MaemoTarget::handleTargetAdded(ProjectExplorer::Target *target)
{
    if (target != this)
        return;

    if (!project()->rootProjectNode()) {
        // Project is not fully setup yet, happens on new project
        // we wait for the fromMapFinished that comes afterwards
        return;
    }

    disconnect(project(), SIGNAL(fromMapFinished()),
        this, SLOT(handleFromMapFinished()));
    disconnect(project(), SIGNAL(addedTarget(ProjectExplorer::Target*)),
        this, SLOT(handleTargetAdded(ProjectExplorer::Target*)));
    connect(project(), SIGNAL(aboutToRemoveTarget(ProjectExplorer::Target*)),
        SLOT(handleTargetToBeRemoved(ProjectExplorer::Target*)));
    const ActionStatus status = createTemplates();
    if (status == ActionFailed)
        return;
    if (status == ActionSuccessful) // Don't do this when the packaging data already exists.
        initPackagingSettingsFromOtherTarget();
    handleTargetAddedSpecial();
    if (status == ActionSuccessful) {
        const QStringList &files = packagingFilePaths();
        if (!files.isEmpty()) {
            const QString list = QLatin1String("<ul><li>") + files.join(QLatin1String("</li><li>"))
                + QLatin1String("</li></ul>");
            QMessageBox::StandardButton button = QMessageBox::question(Core::ICore::mainWindow(),
                tr("Add Packaging Files to Project"),
                tr("<html>Qt Creator has set up the following files to enable "
                   "packaging:\n   %1\nDo you want to add them to the project?</html>")
                   .arg(list), QMessageBox::Yes | QMessageBox::No);
            if (button == QMessageBox::Yes) {
                ProjectExplorer::ProjectExplorerPlugin::instance()
                    ->addExistingFiles(project()->rootProjectNode(), files);
            }
        }
    }

    m_isInitialized = true;
}

void AbstractQt4MaemoTarget::handleTargetToBeRemoved(ProjectExplorer::Target *target)
{
    if (target != this)
        return;
    if (!targetCanBeRemoved())
        return;

    const int answer = QMessageBox::warning(Core::ICore::mainWindow(),
        tr("Qt Creator"), tr("Do you want to remove the packaging files "
           "associated with the target '%1'?").arg(displayName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::No)
        return;
    const QStringList pkgFilePaths = packagingFilePaths();
    if (!pkgFilePaths.isEmpty()) {
        project()->rootProjectNode()->removeFiles(ProjectExplorer::UnknownFileType,
            pkgFilePaths);
        Core::IVersionControl * const vcs = Core::ICore::vcsManager()
            ->findVersionControlForDirectory(QFileInfo(pkgFilePaths.first()).dir().path());
        if (vcs && vcs->supportsOperation(Core::IVersionControl::DeleteOperation)) {
            foreach (const QString &filePath, pkgFilePaths)
                vcs->vcsDelete(filePath);
        }
    }
    delete m_filesWatcher;
    removeTarget();
    QString error;
    const QString packagingPath = project()->projectDirectory()
        + QLatin1Char('/') + PackagingDirName;
    const QStringList otherContents = QDir(packagingPath).entryList(QDir::Dirs
        | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
    if (otherContents.isEmpty()) {
        if (!Utils::FileUtils::removeRecursively(packagingPath, &error))
            qDebug("%s", qPrintable(error));
    }
}

AbstractQt4MaemoTarget::ActionStatus AbstractQt4MaemoTarget::createTemplates()
{
    QDir projectDir(project()->projectDirectory());
    if (!projectDir.exists(PackagingDirName)
            && !projectDir.mkdir(PackagingDirName)) {
        raiseError(tr("Error creating packaging directory '%1'.")
            .arg(PackagingDirName));
        return ActionFailed;
    }

    return createSpecialTemplates();
}

bool AbstractQt4MaemoTarget::initPackagingSettingsFromOtherTarget()
{
    bool success = true;
    foreach (const Target * const target, project()->targets()) {
        const AbstractQt4MaemoTarget * const maemoTarget
            = qobject_cast<const AbstractQt4MaemoTarget *>(target);
        if (maemoTarget && maemoTarget != this && maemoTarget->m_isInitialized) {
            if (!setProjectVersionInternal(maemoTarget->projectVersion()))
                success = false;
            if (!setPackageNameInternal(maemoTarget->packageName()))
                success = false;
            if (!setShortDescriptionInternal(maemoTarget->shortDescription()))
                success = false;
            break;
        }
    }
    return initAdditionalPackagingSettingsFromOtherTarget() && success;
}

void AbstractQt4MaemoTarget::raiseError(const QString &reason)
{
    QMessageBox::critical(0, tr("Error creating MeeGo templates"), reason);
}

AbstractDebBasedQt4MaemoTarget::AbstractDebBasedQt4MaemoTarget(Qt4Project *parent,
                                                               const Core::Id id,
                                                               const QString &qmakeScope) :
    AbstractQt4MaemoTarget(parent, id, qmakeScope)
{
}

AbstractDebBasedQt4MaemoTarget::~AbstractDebBasedQt4MaemoTarget() {}

QString AbstractDebBasedQt4MaemoTarget::projectVersion(QString *error) const
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

bool AbstractDebBasedQt4MaemoTarget::setProjectVersionInternal(const QString &version,
    QString *error)
{
    const QString filePath = changeLogFilePath();
    Utils::FileReader reader;
    if (!reader.fetch(filePath, error))
        return false;
    QString content = QString::fromUtf8(reader.data());
    if (content.contains(QLatin1Char('(') + version + QLatin1Char(')'))) {
        if (error) {
            *error = tr("Refusing to update changelog file: Already contains version '%1'.")
                .arg(version);
        }
        return false;
    }

    int maintainerOffset = content.indexOf(QLatin1String("\n -- "));
    const int eolOffset = content.indexOf(QLatin1Char('\n'), maintainerOffset+1);
    if (maintainerOffset == -1 || eolOffset == -1) {
        if (error) {
            *error = tr("Cannot update changelog: Invalid format (no maintainer entry found).");
        }
        return false;
    }

    ++maintainerOffset;
    const QDateTime currentDateTime = QDateTime::currentDateTime();
    QDateTime utcDateTime = QDateTime(currentDateTime);
    utcDateTime.setTimeSpec(Qt::UTC);
    int utcOffsetSeconds = currentDateTime.secsTo(utcDateTime);
    QChar sign;
    if (utcOffsetSeconds < 0) {
        utcOffsetSeconds = -utcOffsetSeconds;
        sign = QLatin1Char('-');
    } else {
        sign = QLatin1Char('+');
    }
    const int utcOffsetMinutes = (utcOffsetSeconds / 60) % 60;
    const int utcOffsetHours = utcOffsetSeconds / 3600;
    const QString dateString = QString::fromLatin1("%1, %2 %3 %4 %5%6%7")
        .arg(shortDayOfWeekName(currentDateTime))
        .arg(currentDateTime.toString(QLatin1String("dd")))
        .arg(shortMonthName(currentDateTime))
        .arg(currentDateTime.toString(QLatin1String("yyyy hh:mm:ss"))).arg(sign)
        .arg(utcOffsetHours, 2, 10, QLatin1Char('0'))
        .arg(utcOffsetMinutes, 2, 10, QLatin1Char('0'));
    const QString maintainerLine = content.mid(maintainerOffset, eolOffset - maintainerOffset + 1)
        .replace(QRegExp(QLatin1String(">  [^\\n]*\n")),
                 QString::fromLatin1(">  %1").arg(dateString));
    QString versionLine = content.left(content.indexOf(QLatin1Char('\n')))
        .replace(QRegExp(QLatin1String("\\([a-zA-Z0-9_\\.]+\\)")),
                 QLatin1Char('(') + version + QLatin1Char(')'));
    const QString newEntry = versionLine + QLatin1String("\n  * <Add change description here>\n\n")
        + maintainerLine + QLatin1String("\n\n");
    content.prepend(newEntry);
    Core::FileChangeBlocker update(filePath);
    Utils::FileSaver saver(filePath);
    saver.write(content.toUtf8());
    return saver.finalize(error);
}

QIcon AbstractDebBasedQt4MaemoTarget::packageManagerIcon(QString *error) const
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

bool AbstractDebBasedQt4MaemoTarget::setPackageManagerIconInternal(const QString &iconFilePath,
    QString *error)
{
    const QString filePath = controlFilePath();
    Utils::FileReader reader;
    if (!reader.fetch(filePath, error))
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
    if (!pixmap.scaled(packageManagerIconSize()).save(&buffer,
        QFileInfo(iconFilePath).suffix().toAscii())) {
        if (error)
            *error = tr("Could not export image file '%1'.").arg(iconFilePath);
        return false;
    }
    buffer.close();
    iconAsBase64 = iconAsBase64.toBase64();
    QByteArray contents = reader.data();
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
    Core::FileChangeBlocker update(filePath);
    Utils::FileSaver saver(filePath);
    saver.write(contents);
    return saver.finalize(error);
}

QString AbstractDebBasedQt4MaemoTarget::packageName() const
{
    return QString::fromUtf8(controlFileFieldValue(NameFieldName, false));
}

bool AbstractDebBasedQt4MaemoTarget::setPackageNameInternal(const QString &packageName)
{
    const QString oldPackageName = this->packageName();

    if (!setControlFieldValue(NameFieldName, packageName.toUtf8()))
        return false;
    if (!setControlFieldValue("Source", packageName.toUtf8()))
        return false;

    Utils::FileReader reader;
    if (!reader.fetch(changeLogFilePath()))
        return false;
    QString changelogContents = QString::fromUtf8(reader.data());
    QRegExp pattern(QLatin1String("[^\\s]+( \\(\\d\\.\\d\\.\\d\\))"));
    changelogContents.replace(pattern, packageName + QLatin1String("\\1"));
    Utils::FileSaver saver(changeLogFilePath());
    saver.write(changelogContents.toUtf8());
    if (!saver.finalize())
        return false;

    if (!reader.fetch(rulesFilePath()))
        return false;
    QByteArray rulesContents = reader.data();
    const QString oldString = QLatin1String("debian/") + oldPackageName;
    const QString newString = QLatin1String("debian/") + packageName;
    rulesContents.replace(oldString.toUtf8(), newString.toUtf8());
    Utils::FileSaver rulesSaver(rulesFilePath());
    rulesSaver.write(rulesContents);
    return rulesSaver.finalize();
}

QString AbstractDebBasedQt4MaemoTarget::packageManagerName() const
{
    return QString::fromUtf8(controlFileFieldValue(packageManagerNameFieldName(), false));
}

bool AbstractDebBasedQt4MaemoTarget::setPackageManagerName(const QString &name,
    QString *error)
{
    bool success = true;
    foreach (Target * const t, project()->targets()) {
        AbstractDebBasedQt4MaemoTarget * const target
            = qobject_cast<AbstractDebBasedQt4MaemoTarget *>(t);
        if (target) {
            if (!target->setPackageManagerNameInternal(name, error))
                success = false;
        }
    }
    return success;
}

bool AbstractDebBasedQt4MaemoTarget::setPackageManagerNameInternal(const QString &name,
    QString *error)
{
    Q_UNUSED(error);
    return setControlFieldValue(packageManagerNameFieldName(), name.toUtf8());
}

QString AbstractDebBasedQt4MaemoTarget::shortDescription() const
{
    return QString::fromUtf8(controlFileFieldValue(ShortDescriptionFieldName, false));
}

QString AbstractDebBasedQt4MaemoTarget::packageFileName() const
{
    return QString::fromUtf8(controlFileFieldValue(PackageFieldName, false))
        + QLatin1Char('_') + projectVersion() + QLatin1String("_armel.deb");
}

bool AbstractDebBasedQt4MaemoTarget::setShortDescriptionInternal(const QString &description)
{
    return setControlFieldValue(ShortDescriptionFieldName, description.toUtf8());
}

QString AbstractDebBasedQt4MaemoTarget::debianDirPath() const
{
    return project()->projectDirectory() + QLatin1Char('/') + PackagingDirName
        + QLatin1Char('/') + debianDirName();
}

QStringList AbstractDebBasedQt4MaemoTarget::debianFiles() const
{
    return QDir(debianDirPath())
        .entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
}

QString AbstractDebBasedQt4MaemoTarget::changeLogFilePath() const
{
    return debianDirPath() + QLatin1String("/changelog");
}

QString AbstractDebBasedQt4MaemoTarget::controlFilePath() const
{
    return debianDirPath() + QLatin1String("/control");
}

QString AbstractDebBasedQt4MaemoTarget::rulesFilePath() const
{
    return debianDirPath() + QLatin1String("/rules");
}

QByteArray AbstractDebBasedQt4MaemoTarget::controlFileFieldValue(const QString &key,
    bool multiLine) const
{
    QByteArray value;
    Utils::FileReader reader;
    if (!reader.fetch(controlFilePath()))
        return value;
    const QByteArray &contents = reader.data();
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

bool AbstractDebBasedQt4MaemoTarget::setControlFieldValue(const QByteArray &fieldName,
    const QByteArray &fieldValue)
{
    Utils::FileReader reader;
    if (!reader.fetch(controlFilePath()))
        return false;
    QByteArray contents = reader.data();
    if (adaptControlFileField(contents, fieldName, fieldValue)) {
        Core::FileChangeBlocker update(controlFilePath());
        Utils::FileSaver saver(controlFilePath());
        saver.write(contents);
        return saver.finalize();
    }
    return true;
}

bool AbstractDebBasedQt4MaemoTarget::adaptControlFileField(QByteArray &document,
    const QByteArray &fieldName, const QByteArray &newFieldValue)
{
    return adaptTagValue(document, fieldName, newFieldValue, true);
}

void AbstractDebBasedQt4MaemoTarget::handleTargetAddedSpecial()
{
    if (controlFileFieldValue(IconFieldName, true).isEmpty()) {
        // Such a file is created by the mobile wizards.
        const QString iconPath = project()->projectDirectory()
            + QLatin1Char('/') + project()->displayName()
            + QLatin1String("64.png");
        if (QFileInfo(iconPath).exists())
            setPackageManagerIcon(iconPath);
    }

    m_filesWatcher->addDirectory(debianDirPath(), Utils::FileSystemWatcher::WatchAllChanges);
    m_controlFile = new WatchableFile(controlFilePath(), this);
    connect(m_controlFile, SIGNAL(modified()), SIGNAL(controlChanged()));
    m_changeLogFile = new WatchableFile(changeLogFilePath(), this);
    connect(m_changeLogFile, SIGNAL(modified()), SIGNAL(changeLogChanged()));
    Core::DocumentManager::addDocuments(QList<Core::IDocument *>()
        << m_controlFile << m_changeLogFile);
    connect(m_filesWatcher, SIGNAL(directoryChanged(QString)), this,
        SLOT(handleDebianDirContentsChanged()));
    handleDebianDirContentsChanged();
    emit controlChanged();
    emit changeLogChanged();
}

bool AbstractDebBasedQt4MaemoTarget::targetCanBeRemoved() const
{
    return QFileInfo(debianDirPath()).exists();
}

void AbstractDebBasedQt4MaemoTarget::removeTarget()
{
    QString error;
    if (!Utils::FileUtils::removeRecursively(debianDirPath(), &error))
        qDebug("%s", qPrintable(error));
}

void AbstractDebBasedQt4MaemoTarget::handleDebianDirContentsChanged()
{
    emit debianDirContentsChanged();
}

AbstractQt4MaemoTarget::ActionStatus AbstractDebBasedQt4MaemoTarget::createSpecialTemplates()
{
    if (QFileInfo(debianDirPath()).exists())
        return NoActionRequired;
    QDir projectDir(project()->projectDirectory());
    QProcess dh_makeProc;
    QString error;
    const Qt4BuildConfiguration * const bc = qobject_cast<Qt4BuildConfiguration * >(activeBuildConfiguration());
    AbstractMaemoPackageCreationStep::preparePackagingProcess(&dh_makeProc, bc,
        projectDir.path() + QLatin1Char('/') + PackagingDirName);
    const QString dhMakeDebianDir = projectDir.path() + QLatin1Char('/')
        + PackagingDirName + QLatin1String("/debian");
    Utils::FileUtils::removeRecursively(dhMakeDebianDir, &error);
    const QStringList dh_makeArgs = QStringList() << QLatin1String("dh_make")
        << QLatin1String("-s") << QLatin1String("-n") << QLatin1String("-p")
        << (defaultPackageFileName() + QLatin1Char('_')
            + AbstractMaemoPackageCreationStep::DefaultVersionNumber);
    QtSupport::BaseQtVersion *lqt = activeQt4BuildConfiguration()->qtVersion();
    if (!lqt) {
        raiseError(tr("Unable to create Debian templates: No Qt version set."));
        return ActionFailed;
    }
    if (!MaemoGlobal::callMad(dh_makeProc, dh_makeArgs, lqt->qmakeCommand().toString(), true)
            || !dh_makeProc.waitForStarted()) {
        raiseError(tr("Unable to create Debian templates: dh_make failed (%1).")
            .arg(dh_makeProc.errorString()));
        return ActionFailed;
    }
    dh_makeProc.write("\n"); // Needs user input.
    dh_makeProc.waitForFinished(-1);
    if (dh_makeProc.error() != QProcess::UnknownError
        || dh_makeProc.exitCode() != 0) {
        raiseError(tr("Unable to create debian templates: dh_make failed (%1).")
            .arg(dh_makeProc.errorString()));
        return ActionFailed;
    }

    if (!QFile::rename(dhMakeDebianDir, debianDirPath())) {
        raiseError(tr("Unable to move new debian directory to '%1'.")
            .arg(QDir::toNativeSeparators(debianDirPath())));
        Utils::FileUtils::removeRecursively(dhMakeDebianDir, &error);
        return ActionFailed;
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

    return adaptRulesFile() && adaptControlFile()
        ? ActionSuccessful : ActionFailed;
}

bool AbstractDebBasedQt4MaemoTarget::adaptRulesFile()
{
    Utils::FileReader reader;
    if (!reader.fetch(rulesFilePath())) {
        raiseError(reader.errorString());
        return false;
    }
    QByteArray rulesContents = reader.data();
    const QByteArray comment("# Uncomment this line for use without Qt Creator");
    rulesContents.replace("DESTDIR", "INSTALL_ROOT");
    rulesContents.replace("dh_shlibdeps", "# dh_shlibdeps " + comment);
    rulesContents.replace("# Add here commands to configure the package.",
        "# qmake PREFIX=/usr" + comment);
    rulesContents.replace("$(MAKE)\n", "# $(MAKE) " + comment + '\n');

    // Would be the right solution, but does not work (on Windows),
    // because dpkg-genchanges doesn't know about it (and can't be told).
    // rulesContents.replace("dh_builddeb", "dh_builddeb --destdir=.");

    Utils::FileSaver saver(rulesFilePath());
    saver.write(rulesContents);
    if (!saver.finalize()) {
        raiseError(saver.errorString());
        return false;
    }
    return true;
}

bool AbstractDebBasedQt4MaemoTarget::adaptControlFile()
{
    Utils::FileReader reader;
    if (!reader.fetch(controlFilePath())) {
        raiseError(reader.errorString());
        return false;
    }
    QByteArray controlContents = reader.data();

    adaptControlFileField(controlContents, "Section", defaultSection());
    adaptControlFileField(controlContents, "Priority", "optional");
    adaptControlFileField(controlContents, packageManagerNameFieldName(),
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

    addAdditionalControlFileFields(controlContents);
    Utils::FileSaver saver(controlFilePath());
    saver.write(controlContents);
    if (!saver.finalize()) {
        raiseError(saver.errorString());
        return false;
    }
    return true;
}

bool AbstractDebBasedQt4MaemoTarget::initAdditionalPackagingSettingsFromOtherTarget()
{
    foreach (const Target * const t, project()->targets()) {
        const AbstractDebBasedQt4MaemoTarget *target
            = qobject_cast<const AbstractDebBasedQt4MaemoTarget *>(t);
        if (target && target != this) {
            return setControlFieldValue(IconFieldName,
                target->controlFileFieldValue(IconFieldName, true));
        }
    }
    return true;
}

QStringList AbstractDebBasedQt4MaemoTarget::packagingFilePaths() const
{
    QStringList filePaths;
    const QString parentDir = debianDirPath();
    foreach (const QString &fileName, debianFiles())
        filePaths << parentDir + QLatin1Char('/') + fileName;
    return filePaths;
}

QString AbstractDebBasedQt4MaemoTarget::defaultPackageFileName() const
{
    QString packageName = project()->displayName().toLower();

    // We also replace dots, because OVI store chokes on them.
    QRegExp legalLetter(QLatin1String("[a-z0-9+-]"), Qt::CaseSensitive, QRegExp::WildcardUnix);

    for (int i = 0; i < packageName.length(); ++i) {
        if (!legalLetter.exactMatch(packageName.mid(i, 1)))
            packageName[i] = QLatin1Char('-');
    }
    return packageName;
}

bool AbstractDebBasedQt4MaemoTarget::setPackageManagerIcon(const QString &iconFilePath,
    QString *error)
{
    bool success = true;
    foreach (Target * const target, project()->targets()) {
        AbstractDebBasedQt4MaemoTarget* const maemoTarget
            = qobject_cast<AbstractDebBasedQt4MaemoTarget*>(target);
        if (maemoTarget) {
            if (!maemoTarget->setPackageManagerIconInternal(iconFilePath, error))
                success = false;
        }
    }
    return success;
}

// The QDateTime API can only deliver these in localized form...
QString AbstractDebBasedQt4MaemoTarget::shortMonthName(const QDateTime &dt) const
{
    switch (dt.date().month()) {
    case 1: return QLatin1String("Jan");
    case 2: return QLatin1String("Feb");
    case 3: return QLatin1String("Mar");
    case 4: return QLatin1String("Apr");
    case 5: return QLatin1String("May");
    case 6: return QLatin1String("Jun");
    case 7: return QLatin1String("Jul");
    case 8: return QLatin1String("Aug");
    case 9: return QLatin1String("Sep");
    case 10: return QLatin1String("Oct");
    case 11: return QLatin1String("Nov");
    case 12: return QLatin1String("Dec");
    default: QTC_ASSERT(false, return QString());
    }
}

QString AbstractDebBasedQt4MaemoTarget::shortDayOfWeekName(const QDateTime &dt) const
{
    switch (dt.date().dayOfWeek()) {
    case Qt::Monday: return QLatin1String("Mon");
    case Qt::Tuesday: return QLatin1String("Tue");
    case Qt::Wednesday: return QLatin1String("Wed");
    case Qt::Thursday: return QLatin1String("Thu");
    case Qt::Friday: return QLatin1String("Fri");
    case Qt::Saturday: return QLatin1String("Sat");
    case Qt::Sunday: return QLatin1String("Sun");
    default: QTC_ASSERT(false, return QString());
    }
}


AbstractRpmBasedQt4MaemoTarget::AbstractRpmBasedQt4MaemoTarget(Qt4Project *parent,
        const Core::Id id, const QString &qmakeScope) :
    AbstractQt4MaemoTarget(parent, id, qmakeScope)
{
}

AbstractRpmBasedQt4MaemoTarget::~AbstractRpmBasedQt4MaemoTarget()
{
}

QString AbstractRpmBasedQt4MaemoTarget::specFilePath() const
{
    const QLatin1Char sep('/');
    return project()->projectDirectory() + sep + PackagingDirName + sep
        + specFileName();
}

QString AbstractRpmBasedQt4MaemoTarget::projectVersion(QString *error) const
{
    return QString::fromUtf8(getValueForTag(VersionTag, error));
}

bool AbstractRpmBasedQt4MaemoTarget::setProjectVersionInternal(const QString &version,
    QString *error)
{
    return setValueForTag(VersionTag, version.toUtf8(), error);
}

QString AbstractRpmBasedQt4MaemoTarget::packageName() const
{
    return QString::fromUtf8(getValueForTag(NameTag, 0));
}

bool AbstractRpmBasedQt4MaemoTarget::setPackageNameInternal(const QString &name)
{
    return setValueForTag(NameTag, name.toUtf8(), 0);
}

QString AbstractRpmBasedQt4MaemoTarget::shortDescription() const
{
    return QString::fromUtf8(getValueForTag(SummaryTag, 0));
}

QString AbstractRpmBasedQt4MaemoTarget::packageFileName() const
{
    QtSupport::BaseQtVersion *lqt = activeQt4BuildConfiguration()->qtVersion();
    if (!lqt)
        return QString();
    return packageName() + QLatin1Char('-') + projectVersion() + QLatin1Char('-')
        + QString::fromUtf8(getValueForTag(ReleaseTag, 0)) + QLatin1Char('.')
        + MaemoGlobal::architecture(lqt->qmakeCommand().toString())
        + QLatin1String(".rpm");
}

bool AbstractRpmBasedQt4MaemoTarget::setShortDescriptionInternal(const QString &description)
{
    return setValueForTag(SummaryTag, description.toUtf8(), 0);
}

AbstractQt4MaemoTarget::ActionStatus AbstractRpmBasedQt4MaemoTarget::createSpecialTemplates()
{
    if (QFileInfo(specFilePath()).exists())
        return NoActionRequired;
    QByteArray initialContent(
        "Name: %%name%%\n"
        "Summary: <insert short description here>\n"
        "Version: 0.0.1\n"
        "Release: 1\n"
        "License: <Enter your application's license here>\n"
        "Group: <Set your application's group here>\n"
        "%description\n"
        "<Insert longer, multi-line description\n"
        "here.>\n"
        "\n"
        "%prep\n"
        "%setup -q\n"
        "\n"
        "%build\n"
        "# You can leave this empty for use with Qt Creator."
        "\n"
        "%install\n"
        "rm -rf %{buildroot}\n"
        "make INSTALL_ROOT=%{buildroot} install\n"
        "\n"
        "%clean\n"
        "rm -rf %{buildroot}\n"
        "\n"
        "BuildRequires: \n"
        "# %define _unpackaged_files_terminate_build 0\n"
        "%files\n"
        "%defattr(-,root,root,-)"
        "/usr\n"
        "/opt\n"
        "# Add additional files to be included in the package here.\n"
        "%pre\n"
        "# Add pre-install scripts here."
        "%post\n"
        "/sbin/ldconfig # For shared libraries\n"
        "%preun\n"
        "# Add pre-uninstall scripts here."
        "%postun\n"
        "# Add post-uninstall scripts here."
        );
    initialContent.replace("%%name%%", project()->displayName().toUtf8());
    Utils::FileSaver saver(specFilePath());
    saver.write(initialContent);
    return saver.finalize() ? ActionSuccessful : ActionFailed;
}

void AbstractRpmBasedQt4MaemoTarget::handleTargetAddedSpecial()
{
    m_specFile = new WatchableFile(specFilePath(), this);
    connect(m_specFile, SIGNAL(modified()), SIGNAL(specFileChanged()));
    Core::DocumentManager::addDocument(m_specFile);
    emit specFileChanged();
}

bool AbstractRpmBasedQt4MaemoTarget::targetCanBeRemoved() const
{
    return QFileInfo(specFilePath()).exists();
}

void AbstractRpmBasedQt4MaemoTarget::removeTarget()
{
    QFile::remove(specFilePath());
}

bool AbstractRpmBasedQt4MaemoTarget::initAdditionalPackagingSettingsFromOtherTarget()
{
    // Nothing to do here for now.
    return true;
}

QByteArray AbstractRpmBasedQt4MaemoTarget::getValueForTag(const QByteArray &tag,
    QString *error) const
{
    Utils::FileReader reader;
    if (!reader.fetch(specFilePath(), error))
        return QByteArray();
    const QByteArray &content = reader.data();
    const QByteArray completeTag = tag.toLower() + ':';
    int index = content.toLower().indexOf(completeTag);
    if (index == -1)
        return QByteArray();
    index += completeTag.count();
    int endIndex = content.indexOf('\n', index);
    if (endIndex == -1)
        endIndex = content.count();
    return content.mid(index, endIndex - index).trimmed();
}

bool AbstractRpmBasedQt4MaemoTarget::setValueForTag(const QByteArray &tag,
    const QByteArray &value, QString *error)
{
    Utils::FileReader reader;
    if (!reader.fetch(specFilePath(), error))
        return false;
    QByteArray content = reader.data();
    if (adaptTagValue(content, tag, value, false)) {
        Utils::FileSaver saver(specFilePath());
        saver.write(content);
        return saver.finalize(error);
    }
    return true;
}

Qt4Maemo5Target::Qt4Maemo5Target(Qt4Project *parent, const Core::Id id)
    : AbstractDebBasedQt4MaemoTarget(parent, id, QLatin1String("maemo5"))
{
    setDisplayName(defaultDisplayName());
}

Qt4Maemo5Target::~Qt4Maemo5Target() {}

bool Qt4Maemo5Target::supportsDevice(const ProjectExplorer::IDevice::ConstPtr &device) const
{
    return device->type() == Core::Id(Maemo5OsType);
}

QString Qt4Maemo5Target::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Maemo5",
        "Qt4 Maemo5 target display name");
}

void Qt4Maemo5Target::addAdditionalControlFileFields(QByteArray &controlContents)
{
    Q_UNUSED(controlContents);
}

QString Qt4Maemo5Target::debianDirName() const
{
    return QLatin1String("debian_fremantle");
}

QByteArray Qt4Maemo5Target::packageManagerNameFieldName() const
{
    return "XB-Maemo-Display-Name";
}

QSize Qt4Maemo5Target::packageManagerIconSize() const
{
    return QSize(48, 48);
}

QByteArray Qt4Maemo5Target::defaultSection() const
{
    return "user/hidden";
}

Qt4HarmattanTarget::Qt4HarmattanTarget(Qt4Project *parent, const Core::Id id) :
    AbstractDebBasedQt4MaemoTarget(parent, id, QLatin1String("contains(MEEGO_EDITION,harmattan)"))
{
    setDisplayName(defaultDisplayName());
}

Qt4HarmattanTarget::~Qt4HarmattanTarget() {}

bool Qt4HarmattanTarget::supportsDevice(const ProjectExplorer::IDevice::ConstPtr &device) const
{
    return device->type() == Core::Id(HarmattanOsType);
}

QString Qt4HarmattanTarget::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target", "Harmattan",
        "Qt4 Harmattan target display name");
}

QString Qt4HarmattanTarget::aegisManifestFileName()
{
    return QLatin1String("manifest.aegis");
}

void Qt4HarmattanTarget::handleTargetAddedSpecial()
{
    AbstractDebBasedQt4MaemoTarget::handleTargetAddedSpecial();
    const QFile aegisFile(debianDirPath() + QLatin1Char('/') + aegisManifestFileName());
    if (aegisFile.exists())
        return;

    Utils::FileReader reader;
    if (!reader.fetch(Core::ICore::resourcePath()
            + QLatin1String("/templates/shared/") + aegisManifestFileName())) {
        qDebug("Reading manifest template failed.");
        return;
    }
    QString content = QString::fromUtf8(reader.data());
    content.replace(QLatin1String("%%PROJECTNAME%%"), project()->displayName());
    Utils::FileSaver writer(aegisFile.fileName(), QIODevice::WriteOnly);
    writer.write(content.toUtf8());
    if (!writer.finalize()) {
        qDebug("Failure writing manifest file.");
        return;
    }
}

void Qt4HarmattanTarget::addAdditionalControlFileFields(QByteArray &controlContents)
{
    adaptControlFileField(controlContents, "XB-Maemo-Flags", "visible");
    adaptControlFileField(controlContents, "XB-MeeGo-Desktop-Entry-Filename", QString::fromLatin1("%1_harmattan").arg(project()->displayName()).toUtf8());
    adaptControlFileField(controlContents, "XB-MeeGo-Desktop-Entry", QString::fromLatin1("\n [Desktop Entry]\n Type=Application\n Name=%1\n Icon=/usr/share/icons/hicolor/80x80/apps/%1%2.png").arg(project()->displayName()).arg(80).toUtf8());
}

QString Qt4HarmattanTarget::debianDirName() const
{
    return QLatin1String("debian_harmattan");
}

QByteArray Qt4HarmattanTarget::packageManagerNameFieldName() const
{
    return "XSBC-Maemo-Display-Name";
}

QSize Qt4HarmattanTarget::packageManagerIconSize() const
{
    return QSize(64, 64);
}

QByteArray Qt4HarmattanTarget::defaultSection() const
{
    return "user/other";
}


Qt4MeegoTarget::Qt4MeegoTarget(Qt4Project *parent, const Core::Id id) :
    AbstractRpmBasedQt4MaemoTarget(parent, id,
                                   QLatin1String("!isEmpty(MEEGO_VERSION_MAJOR):!contains(MEEGO_EDITION,harmattan)"))
{
    setDisplayName(defaultDisplayName());
}

Qt4MeegoTarget::~Qt4MeegoTarget() {}

bool Qt4MeegoTarget::supportsDevice(const ProjectExplorer::IDevice::ConstPtr &device) const
{
    return device->type() == Core::Id(MeeGoOsType);
}

QString Qt4MeegoTarget::defaultDisplayName()
{
    return QApplication::translate("Qt4ProjectManager::Qt4Target",
        "MeeGo", "Qt4 MeeGo target display name");
}

QString Qt4MeegoTarget::specFileName() const
{
    return QLatin1String("meego.spec");
}

} // namespace Internal
} // namespace Madde
