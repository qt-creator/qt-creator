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

#include "abstractmobileapp.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#ifndef CREATORLESSTEST
#include <coreplugin/icore.h>
#endif // CREATORLESSTEST

namespace Qt4ProjectManager {
namespace Internal {

AbstractGeneratedFileInfo::AbstractGeneratedFileInfo()
    : fileType(ExtendedFile)
    , currentVersion(-1)
    , version(-1)
    , dataChecksum(0)
    , statedChecksum(0)
{
}

const QString AbstractMobileApp::CFileComment(QLatin1String("//"));
const QString AbstractMobileApp::ProFileComment(QLatin1String("#"));
const QString AbstractMobileApp::DeploymentPriFileName(QLatin1String("deployment.pri"));
const QString AbstractMobileApp::FileChecksum(QLatin1String("checksum"));
const QString AbstractMobileApp::FileStubVersion(QLatin1String("version"));
const int AbstractMobileApp::StubVersion = 3;

AbstractMobileApp::AbstractMobileApp()
    : m_orientation(ScreenOrientationAuto)
    , m_networkEnabled(true)
{
}

AbstractMobileApp::~AbstractMobileApp() { }

QString AbstractMobileApp::symbianUidForPath(const QString &path)
{
    quint32 hash = 5381;
    for (int i = 0; i < path.size(); ++i) {
        const char c = path.at(i).toAscii();
        hash ^= c + ((c - i) << i % 20) + ((c + i) << (i + 5) % 20) + ((c - 2 * i) << (i + 10) % 20) + ((c + 2 * i) << (i + 15) % 20);
    }
    return QString::fromLatin1("0xE")
            + QString::fromLatin1("%1").arg(hash, 7, 16, QLatin1Char('0')).right(7).toUpper();
}

void AbstractMobileApp::setOrientation(ScreenOrientation orientation)
{
    m_orientation = orientation;
}

AbstractMobileApp::ScreenOrientation AbstractMobileApp::orientation() const
{
    return m_orientation;
}

void AbstractMobileApp::setProjectName(const QString &name)
{
    m_projectName = name;
}

QString AbstractMobileApp::projectName() const
{
    return m_projectName;
}

void AbstractMobileApp::setProjectPath(const QString &path)
{
    m_projectPath.setFile(path);
}

void AbstractMobileApp::setSymbianSvgIcon(const QString &icon)
{
    m_symbianSvgIcon = icon;
}

QString AbstractMobileApp::symbianSvgIcon() const
{
    return path(SymbianSvgIconOrigin);
}

void AbstractMobileApp::setMaemoPngIcon(const QString &icon)
{
    m_maemoPngIcon = icon;
}

QString AbstractMobileApp::maemoPngIcon() const
{
    return path(MaemoPngIconOrigin);
}

void AbstractMobileApp::setSymbianTargetUid(const QString &uid)
{
    m_symbianTargetUid = uid;
}

QString AbstractMobileApp::symbianTargetUid() const
{
    return !m_symbianTargetUid.isEmpty() ? m_symbianTargetUid
        : symbianUidForPath(path(AppPro));
}

void AbstractMobileApp::setNetworkEnabled(bool enabled)
{
    m_networkEnabled = enabled;
}

bool AbstractMobileApp::networkEnabled() const
{
    return m_networkEnabled;
}

QString AbstractMobileApp::path(int fileType) const
{
    const QString originsRootApp = originsRoot();
    const QString originsRootShared = templatesRoot() + QLatin1String("shared/");
    const QString mainCppFileName = QLatin1String("main.cpp");
    const QString symbianIconFileName = QLatin1String("symbianicon.svg");
    QString cleanProjectName = m_projectName;
    cleanProjectName.replace(QLatin1Char('-'), QString());
    switch (fileType) {
        case MainCpp:               return outputPathBase() + mainCppFileName;
        case MainCppOrigin:         return originsRootApp + mainCppFileName;
        case AppPro:                return outputPathBase() + cleanProjectName + QLatin1String(".pro");
        case AppProOrigin:          return originsRootApp + QLatin1String("app.pro");
        case AppProPath:            return outputPathBase();
        case Desktop:               return outputPathBase() + cleanProjectName + QLatin1String(".desktop");
        case DesktopOrigin:         return originsRootShared + QLatin1String("app.desktop");
        case DeploymentPri:         return outputPathBase() + DeploymentPriFileName;
        case DeploymentPriOrigin:   return originsRootShared + DeploymentPriFileName;
        case SymbianSvgIcon:        return outputPathBase() + cleanProjectName + QLatin1String(".svg");
        case SymbianSvgIconOrigin:  return !m_symbianSvgIcon.isEmpty() ? m_symbianSvgIcon
                                        : originsRootShared + symbianIconFileName;
        case MaemoPngIcon:          return outputPathBase() + cleanProjectName +  QLatin1String(".png");
        case MaemoPngIconOrigin:    return !m_maemoPngIcon.isEmpty() ? m_maemoPngIcon
                                        : originsRootShared + QLatin1String("maemoicon.png");
        default:                    return pathExtended(fileType);
    }
    return QString();
}

QByteArray AbstractMobileApp::generateDesktopFile(QString *errorMessage) const
{
    QFile desktopTemplate(path(DesktopOrigin));
    if (!desktopTemplate.open(QIODevice::ReadOnly)) {
        *errorMessage = QCoreApplication::translate("Qt4ProjectManager::AbstractMobileApp",
            "Could not open desktop file template");
        return QByteArray();
    }
    QByteArray desktopFileContent = desktopTemplate.readAll();
    return desktopFileContent.replace("thisApp", projectName().toUtf8());
}

QByteArray AbstractMobileApp::generateMainCpp(QString *errorMessage) const
{
    QFile sourceFile(path(MainCppOrigin));
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        *errorMessage = QCoreApplication::translate("Qt4ProjectManager::AbstractMobileApp",
                                                    "Could not open main.cpp template '%1'.")
            .arg(sourceFile.fileName());
        return QByteArray();
    }
    QTextStream in(&sourceFile);

    QByteArray mainCppContent;
    QTextStream out(&mainCppContent, QIODevice::WriteOnly);

    QString line;
    while (!(line = in.readLine()).isNull()) {
        bool adaptLine = true;
        if (line.contains(QLatin1String("// ORIENTATION"))) {
            const char *orientationString;
            switch (orientation()) {
            case ScreenOrientationLockLandscape:
                orientationString = "ScreenOrientationLockLandscape";
                break;
            case ScreenOrientationLockPortrait:
                orientationString = "ScreenOrientationLockPortrait";
                break;
            case ScreenOrientationAuto:
                orientationString = "ScreenOrientationAuto";
                break;
            }
            insertParameter(line, mainWindowClassName() + QLatin1String("::")
                + QLatin1String(orientationString));
        } else if (line.contains(QLatin1String("// DELETE_LINE"))) {
            continue; // omit this line in the output
        } else {
            adaptLine = adaptCurrentMainCppTemplateLine(line);
        }
        if (adaptLine) {
            const int commentIndex = line.indexOf(QLatin1String(" //"));
            if (commentIndex != -1)
                line.truncate(commentIndex);
            out << line << endl;
        }
    }

    return mainCppContent;
}

QByteArray AbstractMobileApp::generateProFile(QString *errorMessage) const
{
    const QChar comment = QLatin1Char('#');
    QFile proFile(path(AppProOrigin));
    if (!proFile.open(QIODevice::ReadOnly)) {
        *errorMessage = QCoreApplication::translate("Qt4ProjectManager::AbstractMobileApp",
                                                    "Could not open project file template '%1'.")
            .arg(proFile.fileName());
        return QByteArray();
    }
    QTextStream in(&proFile);

    QByteArray proFileContent;
    QTextStream out(&proFileContent, QIODevice::WriteOnly);

    QString valueOnNextLine;
    bool commentOutNextLine = false;
    QString line;
    while (!(line = in.readLine()).isNull()) {
        if (line.contains(QLatin1String("# TARGETUID3"))) {
            valueOnNextLine = symbianTargetUid();
        } else if (line.contains(QLatin1String("# NETWORKACCESS"))
            && !networkEnabled()) {
            commentOutNextLine = true;
        } else if (line.contains(QLatin1String("# DEPLOYMENTFOLDERS"))) {
            // Eat lines
            QString nextLine;
            while (!(nextLine = in.readLine()).isNull()
                && !nextLine.contains(QLatin1String("# DEPLOYMENTFOLDERS_END")))
            { }
            if (nextLine.isNull())
                continue;

            int foldersCount = 0;
            QStringList folders;
            foreach (const DeploymentFolder &folder, deploymentFolders()) {
                foldersCount++;
                const QString folderName =
                    QString::fromLatin1("folder_%1").arg(foldersCount, 2, 10, QLatin1Char('0'));
                out << folderName << ".source = " << folder.first << endl;
                if (!folder.second.isEmpty())
                    out << folderName << ".target = " << folder.second << endl;
                folders.append(folderName);
            }
            if (foldersCount > 0)
                out << "DEPLOYMENTFOLDERS = " << folders.join(QLatin1String(" ")) << endl;
        } else {
            handleCurrentProFileTemplateLine(line, in, out, commentOutNextLine);
        }

        // Remove all marker comments
        if (line.trimmed().startsWith(comment)
            && line.trimmed().endsWith(comment))
            continue;

        if (!valueOnNextLine.isEmpty()) {
            out << line.left(line.indexOf(QLatin1Char('=')) + 2)
                << QDir::fromNativeSeparators(valueOnNextLine) << endl;
            valueOnNextLine.clear();
            continue;
        }

        if (commentOutNextLine) {
            out << comment << line << endl;
            commentOutNextLine = false;
            continue;
        }
        out << line << endl;
    };

    proFileContent.replace("../shared/" + DeploymentPriFileName.toAscii(),
        DeploymentPriFileName.toAscii());
    return proFileContent;
}

QList<AbstractGeneratedFileInfo> AbstractMobileApp::fileUpdates(const QString &mainProFile) const
{
    QList<AbstractGeneratedFileInfo> result;
    foreach (const AbstractGeneratedFileInfo &file, updateableFiles(mainProFile)) {
        AbstractGeneratedFileInfo newFile = file;
        QFile readFile(newFile.fileInfo.absoluteFilePath());
        if (!readFile.open(QIODevice::ReadOnly))
           continue;
        const QString firstLine = readFile.readLine();
        const QStringList elements = firstLine.split(QLatin1Char(' '));
        if (elements.count() != 5 || elements.at(1) != FileChecksum
                || elements.at(3) != FileStubVersion)
            continue;
        const QString versionString = elements.at(4);
        newFile.version = versionString.startsWith(QLatin1String("0x"))
            ? versionString.toInt(0, 16) : 0;
        newFile.statedChecksum = elements.at(2).toUShort(0, 16);
        QByteArray data = readFile.readAll();
        data.replace('\x0D', "");
        data.replace('\x0A', "");
        newFile.dataChecksum = qChecksum(data.constData(), data.length());
        if (newFile.dataChecksum != newFile.statedChecksum
                || newFile.version < newFile.currentVersion)
            result.append(newFile);
    }
    return result;
}


bool AbstractMobileApp::updateFiles(const QList<AbstractGeneratedFileInfo> &list, QString &error) const
{
    error.clear();
    foreach (const AbstractGeneratedFileInfo &info, list) {
        const QByteArray data = generateFile(info.fileType, &error);
        if (!error.isEmpty())
            return false;
        QFile file(info.fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::WriteOnly) || file.write(data) == -1) {
            error = QCoreApplication::translate(
                        "Qt4ProjectManager::Internal::QtQuickApp",
                        "Could not write file '%1'.").
                    arg(QDir::toNativeSeparators(info.fileInfo.canonicalFilePath()));
            return false;
        }
    }
    return true;
}

#ifndef CREATORLESSTEST
// The definition of QtQuickApp::templatesRoot() for
// CREATORLESSTEST is in tests/manual/appwizards/helpers.cpp
QString AbstractMobileApp::templatesRoot()
{
    return Core::ICore::instance()->resourcePath()
        + QLatin1String("/templates/");
}

Core::GeneratedFile AbstractMobileApp::file(const QByteArray &data,
    const QString &targetFile)
{
    Core::GeneratedFile generatedFile(targetFile);
    generatedFile.setBinary(true);
    generatedFile.setBinaryContents(data);
    return generatedFile;
}

Core::GeneratedFiles AbstractMobileApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files;
    files << file(generateFile(AbstractGeneratedFileInfo::AppProFile, errorMessage), path(AppPro));
    files.last().setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    files << file(generateFile(AbstractGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp));
    files << file(generateFile(AbstractGeneratedFileInfo::SymbianSvgIconFile, errorMessage), path(SymbianSvgIcon));
    files << file(generateFile(AbstractGeneratedFileInfo::MaemoPngIconFile, errorMessage), path(MaemoPngIcon));
    files << file(generateFile(AbstractGeneratedFileInfo::DesktopFile, errorMessage), path(Desktop));
    return files;
}
#endif // CREATORLESSTEST

QString AbstractMobileApp::error() const
{
    return m_error;
}

QByteArray AbstractMobileApp::readBlob(const QString &filePath,
    QString *errorMsg) const
{
    QFile sourceFile(filePath);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        *errorMsg = QCoreApplication::translate("Qt4ProjectManager::AbstractMobileApp",
            "Could not open template file '%1'.").arg(filePath);
        return QByteArray();
    }
    return sourceFile.readAll();
}

QByteArray AbstractMobileApp::generateFile(int fileType,
    QString *errorMessage) const
{
    QByteArray data;
    QString comment = CFileComment;
    bool versionAndChecksum = false;
    switch (fileType) {
        case AbstractGeneratedFileInfo::MainCppFile:
            data = generateMainCpp(errorMessage);
            break;
        case AbstractGeneratedFileInfo::AppProFile:
            data = generateProFile(errorMessage);
            comment = ProFileComment;
            break;
        case AbstractGeneratedFileInfo::SymbianSvgIconFile:
            data = readBlob(path(SymbianSvgIconOrigin), errorMessage);
            break;
        case AbstractGeneratedFileInfo::MaemoPngIconFile:
            data = readBlob(path(MaemoPngIconOrigin), errorMessage);
            break;
        case AbstractGeneratedFileInfo::DesktopFile:
            data = generateDesktopFile(errorMessage);
            break;
        case AbstractGeneratedFileInfo::DeploymentPriFile:
            data = readBlob(path(DeploymentPriOrigin), errorMessage);
            comment = ProFileComment;
            versionAndChecksum = true;
            break;
        default:
            data = generateFileExtended(fileType, &versionAndChecksum,
                &comment, errorMessage);
    }
    if (!versionAndChecksum)
        return data;
    QByteArray versioned = data;
    versioned.replace('\x0D', "");
    versioned.replace('\x0A', "");
    const QLatin1String hexPrefix("0x");
    const quint16 checkSum = qChecksum(versioned.constData(), versioned.length());
    const QString checkSumString = hexPrefix + QString::number(checkSum, 16);
    const QString versionString
        = hexPrefix + QString::number(makeStubVersion(stubVersionMinor()), 16);
    const QChar sep = QLatin1Char(' ');
    const QString versionLine =
            comment + sep + FileChecksum + sep + checkSumString
            + sep + FileStubVersion + sep + versionString + QLatin1Char('\x0A');
    return versionLine.toAscii() + data;
}

int AbstractMobileApp::makeStubVersion(int minor)
{
    return StubVersion << 16 | minor;
}

QString AbstractMobileApp::outputPathBase() const
{
    QString path = m_projectPath.absoluteFilePath();
    if (!path.endsWith(QLatin1Char('/')))
            path.append(QLatin1Char('/'));
    return path + projectName() + QLatin1Char('/');
}

void AbstractMobileApp::insertParameter(QString &line, const QString &parameter)
{
    line.replace(QRegExp(QLatin1String("\\([^()]+\\)")),
        QLatin1Char('(') + parameter + QLatin1Char(')'));
}

} // namespace Internal
} // namespace Qt4ProjectManager
