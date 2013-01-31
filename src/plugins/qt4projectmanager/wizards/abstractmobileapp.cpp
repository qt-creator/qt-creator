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

#include "abstractmobileapp.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

#ifndef CREATORLESSTEST
#include <coreplugin/icore.h>
#endif // CREATORLESSTEST

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

namespace Qt4ProjectManager {

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
const int AbstractMobileApp::StubVersion = 8;

AbstractMobileApp::AbstractMobileApp()
    : QObject()
    , m_canSupportMeegoBooster(false)
    , m_orientation(ScreenOrientationAuto)
    , m_supportsMeegoBooster(false)
{
}

AbstractMobileApp::~AbstractMobileApp() { }

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

void AbstractMobileApp::setPngIcon64(const QString &icon)
{
    m_pngIcon64 = icon;
}

QString AbstractMobileApp::pngIcon64() const
{
    return path(PngIconOrigin64);
}

void AbstractMobileApp::setPngIcon80(const QString &icon)
{
    m_pngIcon80 = icon;
}

QString AbstractMobileApp::pngIcon80() const
{
    return path(PngIconOrigin80);
}

QString AbstractMobileApp::path(int fileType) const
{
    const QString originsRootApp = originsRoot();
    const QString originsRootShared = templatesRoot() + QLatin1String("shared/");
    const QString mainCppFileName = QLatin1String("main.cpp");
    switch (fileType) {
        case MainCpp:               return outputPathBase() + mainCppFileName;
        case MainCppOrigin:         return originsRootApp + mainCppFileName;
        case AppPro:                return outputPathBase() + m_projectName + QLatin1String(".pro");
        case AppProOrigin:          return originsRootApp + QLatin1String("app.pro");
        case AppProPath:            return outputPathBase();
        case DesktopFremantle:      return outputPathBase() + m_projectName + QLatin1String(".desktop");
        case DesktopHarmattan:      return outputPathBase() + m_projectName + QLatin1String("_harmattan.desktop");
        case DesktopOrigin:         return originsRootShared + QLatin1String("app.desktop");
        case DeploymentPri:         return outputPathBase() + DeploymentPriFileName;
        case DeploymentPriOrigin:   return originsRootShared + DeploymentPriFileName;
        case PngIcon64:        return outputPathBase() + m_projectName +  QLatin1String("64.png");
        case PngIconOrigin64:  return !m_pngIcon64.isEmpty() ? m_pngIcon64
                                        : originsRootShared + QLatin1String("icon64.png");
        case PngIcon80:        return outputPathBase() + m_projectName +  QLatin1String("80.png");
        case PngIconOrigin80:  return !m_pngIcon80.isEmpty() ? m_pngIcon80
                                        : originsRootShared + QLatin1String("icon80.png");
        default:                    return pathExtended(fileType);
    }
    return QString();
}

bool AbstractMobileApp::readTemplate(int fileType, QByteArray *data, QString *errorMessage) const
{
    Utils::FileReader reader;
    if (!reader.fetch(path(fileType), errorMessage))
        return false;
    *data = reader.data();
    return true;
}

QByteArray AbstractMobileApp::generateDesktopFile(QString *errorMessage, int fileType) const
{
    QByteArray desktopFileContent;
    if (!readTemplate(DesktopOrigin, &desktopFileContent, errorMessage))
        return QByteArray();
    if (fileType == AbstractGeneratedFileInfo::DesktopFremantleFile) {
        desktopFileContent.replace("Icon=thisApp",
            "Icon=" + projectName().toUtf8() + "64");
    } else if (fileType == AbstractGeneratedFileInfo::DesktopHarmattanFile) {
        desktopFileContent.replace("Icon=thisApp",
            "Icon=/usr/share/icons/hicolor/80x80/apps/" + projectName().toUtf8() + "80.png");
        if (m_supportsMeegoBooster)
            desktopFileContent.replace("Exec=", "Exec=/usr/bin/invoker --type=d -s ");
        else
            desktopFileContent.replace("Exec=", "Exec=/usr/bin/single-instance ");
    }
    return desktopFileContent.replace("thisApp", projectName().toUtf8());
}

QByteArray AbstractMobileApp::generateMainCpp(QString *errorMessage) const
{
    QByteArray mainCppInput;
    if (!readTemplate(MainCppOrigin, &mainCppInput, errorMessage))
        return QByteArray();
    QTextStream in(&mainCppInput);

    QByteArray mainCppContent;
    QTextStream out(&mainCppContent, QIODevice::WriteOnly | QIODevice::Text);

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
            case ScreenOrientationImplicit:
            default:
                continue; // omit line
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
    QByteArray proFileInput;
    if (!readTemplate(AppProOrigin, &proFileInput, errorMessage))
        return QByteArray();
    QTextStream in(&proFileInput);

    QByteArray proFileContent;
    QTextStream out(&proFileContent, QIODevice::WriteOnly | QIODevice::Text);

    QString valueOnNextLine;
    bool commentOutNextLine = false;
    QString line;
    while (!(line = in.readLine()).isNull()) {
        if (line.contains(QLatin1String("# DEPLOYMENTFOLDERS"))) {
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
        } else if (line.contains(QLatin1String("# REMOVE_NEXT_LINE"))) {
            in.readLine(); // eats the following line
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

    proFileContent.replace("../shared/" + DeploymentPriFileName.toLatin1(),
        DeploymentPriFileName.toLatin1());
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
        const QString firstLine = QString::fromUtf8(readFile.readLine());
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
        Utils::FileSaver saver(QDir::cleanPath(info.fileInfo.absoluteFilePath()));
        saver.write(data);
        if (!saver.finalize(&error))
            return false;
    }
    return true;
}

#ifndef CREATORLESSTEST
// The definition of QtQuickApp::templatesRoot() for
// CREATORLESSTEST is in tests/manual/appwizards/helpers.cpp
QString AbstractMobileApp::templatesRoot()
{
    return Core::ICore::resourcePath()
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
    files << file(generateFile(AbstractGeneratedFileInfo::PngIcon64File, errorMessage), path(PngIcon64));
    files << file(generateFile(AbstractGeneratedFileInfo::PngIcon80File, errorMessage), path(PngIcon80));
    files << file(generateFile(AbstractGeneratedFileInfo::DesktopFremantleFile, errorMessage), path(DesktopFremantle));
    files << file(generateFile(AbstractGeneratedFileInfo::DesktopHarmattanFile, errorMessage), path(DesktopHarmattan));
    return files;
}
#endif // CREATORLESSTEST

QString AbstractMobileApp::error() const
{
    return m_error;
}

bool AbstractMobileApp::canSupportMeegoBooster() const
{
    return m_canSupportMeegoBooster;
}

bool AbstractMobileApp::supportsMeegoBooster() const
{
    return m_supportsMeegoBooster;
}

void AbstractMobileApp::setSupportsMeegoBooster(bool supportMeegoBooster)
{
    QTC_ASSERT(canSupportMeegoBooster(), return);
    m_supportsMeegoBooster = supportMeegoBooster;
}

QByteArray AbstractMobileApp::readBlob(const QString &filePath,
    QString *errorMsg) const
{
    Utils::FileReader reader;
    if (!reader.fetch(filePath, errorMsg))
        return QByteArray();
    return reader.data();
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
        case AbstractGeneratedFileInfo::PngIcon64File:
            data = readBlob(path(PngIconOrigin64), errorMessage);
            break;
        case AbstractGeneratedFileInfo::PngIcon80File:
            data = readBlob(path(PngIconOrigin80), errorMessage);
            break;
        case AbstractGeneratedFileInfo::DesktopFremantleFile:
        case AbstractGeneratedFileInfo::DesktopHarmattanFile:
            data = generateDesktopFile(errorMessage, fileType);
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
    return versionLine.toLatin1() + data;
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

} // namespace Qt4ProjectManager
