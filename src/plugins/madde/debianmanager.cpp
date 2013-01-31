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

#include "debianmanager.h"

#include "maddedevice.h"
#include "maemoconstants.h"
#include "maemoglobal.h"
#include "maemopackagecreationstep.h"

#include <coreplugin/documentmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/filesystemwatcher.h>
#include <utils/qtcassert.h>

#include <QBuffer>
#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QProcess>

#include <QMessageBox>

#include <ctype.h>

// -----------------------------------------------------------------------
// Helpers:
// -----------------------------------------------------------------------

namespace {

const char IconFieldName[] = "XB-Maemo-Icon-26";
const char NameFieldName[] = "Package";
const char ShortDescriptionFieldName[] = "Description";

const char PackagingDirName[] = "qtc_packaging";

// The QDateTime API can only deliver these in localized form...
QString shortMonthName(const QDateTime &dt)
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

QString shortDayOfWeekName(const QDateTime &dt)
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

QByteArray packageManagerNameFieldName(Core::Id deviceType)
{
    if (deviceType == Madde::Internal::Maemo5OsType)
        return QByteArray("XB-Maemo-Display-Name");
    return QByteArray("XSBC-Maemo-Display-Name");
}

QList<QPair<QByteArray, QByteArray> > additionalFields(Core::Id deviceType, const QString &projectName)
{
    QList<QPair<QByteArray, QByteArray> > fields;
    if (deviceType == Madde::Internal::HarmattanOsType)
        fields << qMakePair(QByteArray("XB-Maemo-Flags"), QByteArray("visible"))
               << qMakePair(QByteArray("XB-MeeGo-Desktop-Entry-Filename"),
                            QString::fromLatin1("%1_harmattan").arg(projectName).toUtf8())
               << qMakePair(QByteArray("XB-MeeGo-Desktop-Entry"),
                            QString::fromLatin1("\n [Desktop Entry]\n Type=Application\n Name=%1\n Icon=/usr/share/icons/hicolor/80x80/apps/%1%2.png")
                            .arg(projectName).arg(80).toUtf8());
    return fields;
}

QByteArray section(Core::Id deviceType)
{
    if (deviceType == Madde::Internal::Maemo5OsType)
        return "user/hidden";
    if (deviceType == Madde::Internal::HarmattanOsType)
        return "user/other";
    return QByteArray();
}

void raiseError(const QString &reason)
{
    QMessageBox::critical(0, Madde::Internal::DebianManager::tr("Error Creating Debian Project Templates"), reason);
}

QString defaultPackageFileName(ProjectExplorer::Project *project)
{
    QString packageName = project->displayName().toLower();

    // We also replace dots later, because OVI store chokes on them (for the N900).
    QRegExp illegalLetter(QLatin1String("[^a-z0-9+-]"), Qt::CaseSensitive, QRegExp::WildcardUnix);

    return packageName.replace(illegalLetter, QLatin1String("-"));
}

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

QByteArray controlFileFieldValue(const Utils::FileName &control, const QString &key, bool multiLine)
{
    QByteArray value;
    Utils::FileReader reader;
    if (!reader.fetch(control.toString()))
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

bool setControlFieldValue(const Utils::FileName &control, const QByteArray &fieldName,
                          const QByteArray &fieldValue)
{
    Utils::FileReader reader;
    if (!reader.fetch(control.toString()))
        return false;
    QByteArray contents = reader.data();
    if (!adaptTagValue(contents, fieldName, fieldValue, true))
        return false;
    Core::FileChangeBlocker update(control.toString());
    Utils::FileSaver saver(control.toString());
    saver.write(contents);
    return saver.finalize();
}

bool adaptRulesFile(const Utils::FileName &rulesPath)
{
    Utils::FileReader reader;
    if (!reader.fetch(rulesPath.toString())) {
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

    Utils::FileSaver saver(rulesPath.toString());
    saver.write(rulesContents);
    if (!saver.finalize()) {
        raiseError(saver.errorString());
        return false;
    }
    return true;
}

bool adaptControlFile(const Utils::FileName &controlPath, Qt4ProjectManager::Qt4BuildConfiguration *bc,
                      const QByteArray &section, const QByteArray &packageManagerNameField,
                      QList<QPair<QByteArray, QByteArray> > additionalFields)
{
    Utils::FileReader reader;
    if (!reader.fetch(controlPath.toString())) {
        raiseError(reader.errorString());
        return false;
    }
    QByteArray controlContents = reader.data();

    adaptTagValue(controlContents, "Section", section, true);
    adaptTagValue(controlContents, "Priority", "optional", true);
    adaptTagValue(controlContents, packageManagerNameField,
                  bc->target()->project()->displayName().toUtf8(), true);
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

    for (int i = 0; i < additionalFields.count(); ++i)
        adaptTagValue(controlContents, additionalFields.at(i).first, additionalFields.at(i).second, true);

    Utils::FileSaver saver(controlPath.toString());
    saver.write(controlContents);
    if (!saver.finalize()) {
        raiseError(saver.errorString());
        return false;
    }
    return true;
}

} // namespace

namespace Madde {
namespace Internal {

// -----------------------------------------------------------------------
// DebianManager:
// -----------------------------------------------------------------------

DebianManager *DebianManager::m_instance = 0;

DebianManager::DebianManager(QObject *parent) :
    QObject(parent),
    m_watcher(new Utils::FileSystemWatcher(this))
{
    m_instance = this;

    m_watcher->setObjectName(QLatin1String("Madde::DebianManager"));
    connect(m_watcher, SIGNAL(directoryChanged(QString)),
            this, SLOT(directoryWasChanged(QString)));
}

DebianManager::~DebianManager()
{ }

DebianManager *DebianManager::instance()
{
    return m_instance;
}

void DebianManager::monitor(const Utils::FileName &debianDir)
{
    QFileInfo fi = debianDir.toFileInfo();
    if (!fi.isDir())
        return;

    if (!m_watches.contains(debianDir)) {
        m_watches.insert(debianDir, 1);
        m_watcher->addDirectory(debianDir.toString(), Utils::FileSystemWatcher::WatchAllChanges);

        WatchableFile *controlFile = new WatchableFile(controlFilePath(debianDir).toString(), this);
        connect(controlFile, SIGNAL(modified()), this, SLOT(controlWasChanged()));
        WatchableFile *changelogFile = new WatchableFile(changelogFilePath(debianDir).toString(), this);
        connect(changelogFile, SIGNAL(modified()), SLOT(changelogWasChanged()));
        Core::DocumentManager::addDocuments(QList<Core::IDocument *>() << controlFile << changelogFile);
    }
}

bool DebianManager::isMonitoring(const Utils::FileName &debianDir)
{
    return m_watches.contains(debianDir);
}

void DebianManager::ignore(const Utils::FileName &debianDir)
{
    int count = m_watches.value(debianDir, 0) - 1;
    if (count < 0)
        return;
    if (count > 0) {
        m_watches[debianDir] = 0;
    } else {
        m_watches.remove(debianDir);
        m_watcher->removeDirectory(debianDir.toString());
    }
}

QString DebianManager::projectVersion(const Utils::FileName &debianDir, QString *error)
{
    Utils::FileName path = changelogFilePath(debianDir);
    QFile changelog(path.toString());
    if (!changelog.open(QIODevice::ReadOnly)) {
        if (error)
            *error = tr("Failed to open debian changelog \"%1\" file for reading.").arg(path.toUserOutput());
        return QString();
    }

    const QByteArray &firstLine = changelog.readLine();
    const int openParenPos = firstLine.indexOf('(');
    if (openParenPos == -1) {
        if (error)
            *error = tr("Debian changelog file '%1' has unexpected format.").arg(path.toUserOutput());
        return QString();
    }
    const int closeParenPos = firstLine.indexOf(')', openParenPos);
    if (closeParenPos == -1) {
        if (error)
            *error = tr("Debian changelog file '%1' has unexpected format.").arg(path.toUserOutput());
        return QString();
    }
    return QString::fromUtf8(firstLine.mid(openParenPos + 1, closeParenPos - openParenPos - 1).data());
}

bool DebianManager::setProjectVersion(const Utils::FileName &debianDir, const QString &version, QString *error)
{
    const Utils::FileName filePath = changelogFilePath(debianDir);
    Utils::FileReader reader;
    if (!reader.fetch(filePath.toString(), error))
        return false;
    QString content = QString::fromUtf8(reader.data());
    if (content.contains(QLatin1Char('(') + version + QLatin1Char(')'))) {
        if (error)
            *error = tr("Refusing to update changelog file: Already contains version '%1'.").arg(version);
        return false;
    }

    int maintainerOffset = content.indexOf(QLatin1String("\n -- "));
    const int eolOffset = content.indexOf(QLatin1Char('\n'), maintainerOffset + 1);
    if (maintainerOffset == -1 || eolOffset == -1) {
        if (error)
            *error = tr("Cannot update changelog: Invalid format (no maintainer entry found).");
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
    Core::FileChangeBlocker update(filePath.toString());
    Utils::FileSaver saver(filePath.toString());
    saver.write(content.toUtf8());
    return saver.finalize(error);
}

QString DebianManager::packageName(const Utils::FileName &debianDir)
{
    return QString::fromUtf8(controlFileFieldValue(controlFilePath(debianDir), QLatin1String(NameFieldName), false));
}

bool DebianManager::setPackageName(const Utils::FileName &debianDir, const QString &newName)
{
    const QString oldPackageName = packageName(debianDir);

    Utils::FileName controlPath = controlFilePath(debianDir);
    if (!setControlFieldValue(controlPath, NameFieldName, newName.toUtf8()))
        return false;
    if (!setControlFieldValue(controlPath, "Source", newName.toUtf8()))
        return false;

    Utils::FileName changelogPath = changelogFilePath(debianDir);
    Utils::FileReader reader;
    if (!reader.fetch(changelogPath.toString()))
        return false;
    QString changelogContents = QString::fromUtf8(reader.data());
    QRegExp pattern(QLatin1String("[^\\s]+( \\(\\d\\.\\d\\.\\d\\))"));
    changelogContents.replace(pattern, newName + QLatin1String("\\1"));
    Core::FileChangeBlocker updateChangelog(changelogPath.toString());
    Utils::FileSaver saver(changelogPath.toString());
    saver.write(changelogContents.toUtf8());
    if (!saver.finalize())
        return false;

    Utils::FileName rulesPath = rulesFilePath(debianDir);
    if (!reader.fetch(rulesPath.toString()))
        return false;

    QByteArray rulesContents = reader.data();
    const QString oldString = QLatin1String("debian/") + oldPackageName;
    const QString newString = QLatin1String("debian/") + newName;
    rulesContents.replace(oldString.toUtf8(), newString.toUtf8());

    Core::FileChangeBlocker updateRules(rulesPath.toString());
    Utils::FileSaver rulesSaver(rulesPath.toString());
    rulesSaver.write(rulesContents);
    return rulesSaver.finalize();
}

QString DebianManager::shortDescription(const Utils::FileName &debianDir)
{
    return QString::fromUtf8(controlFileFieldValue(controlFilePath(debianDir),
                                                   QLatin1String(ShortDescriptionFieldName), false));
}

bool DebianManager::setShortDescription(const Utils::FileName &debianDir, const QString &description)
{
    return setControlFieldValue(controlFilePath(debianDir), ShortDescriptionFieldName, description.toUtf8());
}

QString DebianManager::packageManagerName(const Utils::FileName &debianDir, Core::Id deviceType)
{
    return QString::fromUtf8(controlFileFieldValue(controlFilePath(debianDir),
                                                   QLatin1String(packageManagerNameFieldName(deviceType)), false));
}

bool DebianManager::setPackageManagerName(const Utils::FileName &debianDir, Core::Id deviceType, const QString &name)
{
    return setControlFieldValue(controlFilePath(debianDir), packageManagerNameFieldName(deviceType),
                                name.toUtf8());
}

QIcon DebianManager::packageManagerIcon(const Utils::FileName &debianDir, QString *error)
{
    const QByteArray &base64Icon
            = controlFileFieldValue(controlFilePath(debianDir), QLatin1String(IconFieldName), true);
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

bool DebianManager::setPackageManagerIcon(const Utils::FileName &debianDir, Core::Id deviceType,
                                          const Utils::FileName &iconPath, QString *error)
{
    const Utils::FileName filePath = controlFilePath(debianDir);
    Utils::FileReader reader;
    if (!reader.fetch(filePath.toString(), error))
        return false;
    const QPixmap pixmap(iconPath.toString());
    if (pixmap.isNull()) {
        if (error)
            *error = tr("Could not read image file '%1'.").arg(iconPath.toUserOutput());
        return false;
    }

    QByteArray iconAsBase64;
    QBuffer buffer(&iconAsBase64);
    buffer.open(QIODevice::WriteOnly);
    if (!pixmap.scaled(MaddeDevice::packageManagerIconSize(deviceType))
            .save(&buffer, iconPath.toFileInfo().suffix().toLatin1())) {
        if (error)
            *error = tr("Could not export image file '%1'.").arg(iconPath.toUserOutput());
        return false;
    }
    buffer.close();
    iconAsBase64 = iconAsBase64.toBase64();
    QByteArray contents = reader.data();
    const QByteArray iconFieldNameWithColon = QByteArray(IconFieldName) + ':';
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
                || isspace(contents.at(nextEolPos + 1))))
            nextEolPos = contents.indexOf('\n', nextEolPos + 1);
        if (nextEolPos == -1)
            nextEolPos = contents.length();
        contents.replace(oldIconStartPos, nextEolPos - oldIconStartPos,
            ' ' + iconAsBase64);
    }
    Core::FileChangeBlocker update(filePath.toString());
    Utils::FileSaver saver(filePath.toString());
    saver.write(contents);
    return saver.finalize(error);
}

bool DebianManager::hasPackageManagerIcon(const Utils::FileName &debianDir)
{
    return !packageManagerIcon(debianDir).isNull();
}

Utils::FileName DebianManager::packageFileName(const Utils::FileName &debianDir)
{
    return Utils::FileName::fromString(packageName(debianDir)
                                       + QLatin1Char('_') + projectVersion(debianDir)
                                       + QLatin1String("_armel.deb"));
}

DebianManager::ActionStatus DebianManager::createTemplate(Qt4ProjectManager::Qt4BuildConfiguration *bc,
                                                          const Utils::FileName &debianDir)
{
    if (debianDir.toFileInfo().exists())
        return NoActionRequired;

    Utils::FileName location = debianDir.parentDir();
    if (!location.toFileInfo().isDir()) {
        if (!QDir::home().mkpath(location.toString())) {
            raiseError(tr("Failed to create directory \"%1\".")
                       .arg(location.toUserOutput()));
            return ActionFailed;
        }
    }

    QProcess dh_makeProc;
    QString error;
    AbstractMaemoPackageCreationStep::preparePackagingProcess(&dh_makeProc, bc, location.toString());
    const QString packageName = defaultPackageFileName(bc->target()->project());

    const QStringList dh_makeArgs =
            QStringList() << QLatin1String("dh_make")
                          << QLatin1String("-s") << QLatin1String("-n") << QLatin1String("-p")
                          << (packageName + QLatin1Char('_')
                              + AbstractMaemoPackageCreationStep::DefaultVersionNumber);

    QtSupport::BaseQtVersion *lqt = QtSupport::QtKitInformation::qtVersion(bc->target()->kit());
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

    if (!QFile::rename(location.appendPath(QLatin1String("debian")).toString(), debianDir.toString())) {
        raiseError(tr("Unable to move new debian directory to '%1'.").arg(debianDir.toUserOutput()));
        Utils::FileUtils::removeRecursively(location, &error);
        return ActionFailed;
    }

    QDir debian(debianDir.toString());
    const QStringList &files = debian.entryList(QDir::Files);
    foreach (const QString &fileName, files) {
        if (fileName.endsWith(QLatin1String(".ex"), Qt::CaseInsensitive)
                || fileName.compare(QLatin1String("README.debian"), Qt::CaseInsensitive) == 0
                || fileName.compare(QLatin1String("dirs"), Qt::CaseInsensitive) == 0
                || fileName.compare(QLatin1String("docs"), Qt::CaseInsensitive) == 0) {
            debian.remove(fileName);
        }
    }

    setPackageName(debianDir, packageName);

    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(bc->target()->kit());

    const QByteArray sec = section(deviceType);
    const QByteArray nameField = packageManagerNameFieldName(deviceType);
    QList<QPair<QByteArray, QByteArray> > fields
            = additionalFields(deviceType, bc->target()->project()->displayName());

    return adaptRulesFile(rulesFilePath(debianDir))
            && adaptControlFile(controlFilePath(debianDir), bc, sec, nameField, fields)
            ? ActionSuccessful : ActionFailed;
}

QStringList DebianManager::debianFiles(const Utils::FileName &debianDir)
{
    return QDir(debianDir.toString()).entryList(QDir::Files, QDir::Name | QDir::IgnoreCase);
}

Utils::FileName DebianManager::debianDirectory(ProjectExplorer::Target *target)
{
    Utils::FileName path = Utils::FileName::fromString(target->project()->projectDirectory());
    path.appendPath(QLatin1String(PackagingDirName));
    Core::Id deviceType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target->kit());
    if (deviceType == HarmattanOsType)
        path.appendPath(QLatin1String("debian_harmattan"));
    else if (deviceType == Maemo5OsType)
        path.appendPath(QLatin1String("debian_fremantle"));
    else
        path.clear();
    return path;
}

void DebianManager::directoryWasChanged(const QString &path)
{
    Utils::FileName fn = Utils::FileName::fromString(path);
    QTC_ASSERT(m_watches.contains(fn), return);
    emit debianDirectoryChanged(fn);
}

void DebianManager::controlWasChanged()
{
    WatchableFile *file = qobject_cast<WatchableFile *>(sender());
    if (!file)
        return;
    emit controlChanged(Utils::FileName::fromString(file->fileName()).parentDir());
}

void DebianManager::changelogWasChanged()
{
    WatchableFile *file = qobject_cast<WatchableFile *>(sender());
    if (!file)
        return;
    emit changelogChanged(Utils::FileName::fromString(file->fileName()).parentDir());
}

Utils::FileName DebianManager::changelogFilePath(const Utils::FileName &debianDir)
{
    Utils::FileName result = debianDir;
    return result.appendPath(QLatin1String("changelog"));
}

Utils::FileName DebianManager::controlFilePath(const Utils::FileName &debianDir)
{
    Utils::FileName result = debianDir;
    return result.appendPath(QLatin1String("control"));
}

Utils::FileName DebianManager::rulesFilePath(const Utils::FileName &debianDir)
{
    Utils::FileName result = debianDir;
    return result.appendPath(QLatin1String("rules"));
}

} // namespace Internal
} // namespace Madde
