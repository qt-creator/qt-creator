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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "mobileapp.h"

#include <coreplugin/icore.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>

namespace Qt4ProjectManager {
namespace Internal {

const QString mainWindowBaseName(QLatin1String("mainwindow"));

const QString deploymentPriFileName(QLatin1String("deployment.pri"));
const QString deploymentPriOrigRelFilePath(QLatin1String("../shared/") + deploymentPriFileName);
const QString mainWindowCppFileName(mainWindowBaseName + QLatin1String(".cpp"));
const QString mainWindowHFileName(mainWindowBaseName + QLatin1String(".h"));
const QString mainWindowUiFileName(mainWindowBaseName + QLatin1String(".ui"));
const QString fileChecksum(QLatin1String("checksum"));
const QString fileStubVersion(QLatin1String("version"));


MobileAppGeneratedFileInfo::MobileAppGeneratedFileInfo()
    : file(MainWindowCppFile)
    , version(-1)
    , dataChecksum(0)
    , statedChecksum(0)
{
}

bool MobileAppGeneratedFileInfo::isUpToDate() const
{
    return !isOutdated() && !wasModified();
}

bool MobileAppGeneratedFileInfo::isOutdated() const
{
    return version < MobileApp::stubVersion();
}

bool MobileAppGeneratedFileInfo::wasModified() const
{
    return dataChecksum != statedChecksum;
}

MobileApp::MobileApp() : m_orientation(Auto), m_networkEnabled(false)
{
}

MobileApp::~MobileApp()
{
}

QString MobileApp::symbianUidForPath(const QString &path)
{
    quint32 hash = 5381;
    for (int i = 0; i < path.size(); ++i) {
        const char c = path.at(i).toAscii();
        hash ^= c + ((c - i) << i % 20) + ((c + i) << (i + 5) % 20) + ((c - 2 * i) << (i + 10) % 20) + ((c + 2 * i) << (i + 15) % 20);
    }
    return QString::fromLatin1("0xE")
            + QString::fromLatin1("%1").arg(hash, 7, 16, QLatin1Char('0')).right(7);
}

void MobileApp::setOrientation(Orientation orientation)
{
    m_orientation = orientation;
}

MobileApp::Orientation MobileApp::orientation() const
{
    return m_orientation;
}

void MobileApp::setProjectName(const QString &name)
{
    m_projectName = name;
}

QString MobileApp::projectName() const
{
    return m_projectName;
}

void MobileApp::setProjectPath(const QString &path)
{
    m_projectPath.setFile(path);
}

void MobileApp::setSymbianSvgIcon(const QString &icon)
{
    m_symbianSvgIcon = icon;
}

QString MobileApp::symbianSvgIcon() const
{
    return path(SymbianSvgIconOrigin);
}

void MobileApp::setMaemoPngIcon(const QString &icon)
{
    m_maemoPngIcon = icon;
}

QString MobileApp::maemoPngIcon() const
{
    return path(MaemoPngIconOrigin);
}

void MobileApp::setSymbianTargetUid(const QString &uid)
{
    m_symbianTargetUid = uid;
}

QString MobileApp::symbianTargetUid() const
{
    return !m_symbianTargetUid.isEmpty() ? m_symbianTargetUid
        : symbianUidForPath(path(AppPro));
}

void MobileApp::setNetworkEnabled(bool enabled)
{
    m_networkEnabled = enabled;
}

bool MobileApp::networkEnabled() const
{
    return m_networkEnabled;
}


QString MobileApp::path(Path path) const
{
    const QString originsRootMobileApp = templatesRoot(QLatin1String("mobileapp/"));
    const QString originsRootShared = templatesRoot(QLatin1String("shared/"));
    const QString mainCppFileName = QLatin1String("main.cpp");
    const QString symbianIconFileName = QLatin1String("symbianicon.svg");
    const QString pathBase = m_projectPath.absoluteFilePath() + QLatin1Char('/')
                             + m_projectName + QLatin1Char('/');

    switch (path) {
        case MainCpp:                       return pathBase + mainCppFileName;
        case MainCppOrigin:                 return originsRootMobileApp + mainCppFileName;
        case AppPro:                        return pathBase + m_projectName + QLatin1String(".pro");
        case AppProOrigin:                  return originsRootMobileApp + QLatin1String("app.pro");
        case AppProPath:                    return pathBase;
        case DeploymentPri:                 return pathBase + deploymentPriFileName;
        case DeploymentPriOrigin:           return originsRootMobileApp + deploymentPriOrigRelFilePath;
        case Desktop:                       return pathBase + m_projectName + QLatin1String(".desktop");
        case DesktopOrigin:                 return originsRootShared + QLatin1String("app.desktop");
        case MainWindowCpp:                 return pathBase + mainWindowCppFileName;
        case MainWindowCppOrigin:           return originsRootMobileApp + mainWindowCppFileName;
        case MainWindowH:                   return pathBase + mainWindowHFileName;
        case MainWindowHOrigin:             return originsRootMobileApp + mainWindowHFileName;
        case MainWindowUi:                  return pathBase + mainWindowUiFileName;
        case MainWindowUiOrigin:            return originsRootMobileApp + mainWindowUiFileName;
        case SymbianSvgIcon:                return pathBase + symbianIconFileName;
        case SymbianSvgIconOrigin:          return !m_symbianSvgIcon.isEmpty() ? m_symbianSvgIcon
                                                : originsRootShared + symbianIconFileName;
        case MaemoPngIcon:                  return pathBase + projectName() +  QLatin1String(".png");
        case MaemoPngIconOrigin:            return !m_maemoPngIcon.isEmpty() ? m_maemoPngIcon
                                                : originsRootShared + QLatin1String("maemoicon.png");
        default:                            qFatal("MobileApp::path() needs more work");
    }
    return QString();
}

static QString insertParameter(const QString &line, const QString &parameter)
{
    return QString(line).replace(QRegExp(QLatin1String("\\([^()]+\\)")),
                                 QLatin1Char('(') + parameter + QLatin1Char(')'));
}

QByteArray MobileApp::generateMainCpp(const QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    QFile sourceFile(path(MainCppOrigin));
    sourceFile.open(QIODevice::ReadOnly);
    Q_ASSERT(sourceFile.isOpen());
    QTextStream in(&sourceFile);

    QByteArray mainCppContent;
    QTextStream out(&mainCppContent, QIODevice::WriteOnly);

    QString line;
    while (!(line = in.readLine()).isNull()) {
        if (line.contains(QLatin1String("// ORIENTATION"))) {
            const char *orientationString;
            switch (m_orientation) {
            case LockLandscape:
                orientationString = "LockLandscape";
                break;
            case LockPortrait:
                orientationString = "LockPortrait";
                break;
            case Auto:
                orientationString = "Auto";
                break;
            }
            line = insertParameter(line, QLatin1String("MainWindow::")
                + QLatin1String(orientationString));
        }
        const int commentIndex = line.indexOf(QLatin1String(" //"));
        if (commentIndex != -1)
            line.truncate(commentIndex);
        out << line << endl;
    };

    return mainCppContent;
}

QByteArray MobileApp::generateProFile(const QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    const QChar comment = QLatin1Char('#');
    QFile proFile(path(AppProOrigin));
    proFile.open(QIODevice::ReadOnly);
    Q_ASSERT(proFile.isOpen());
    QTextStream in(&proFile);

    QByteArray proFileContent;
    QTextStream out(&proFileContent, QIODevice::WriteOnly);

    QString valueOnNextLine;
    bool uncommentNextLine = false;
    QString line;
    while (!(line = in.readLine()).isNull()) {
        if (line.contains(QLatin1String("# TARGETUID3"))) {
            valueOnNextLine = symbianTargetUid();
        } else if (line.contains(QLatin1String("# ORIENTATIONLOCK")) && m_orientation == MobileApp::Auto) {
            uncommentNextLine = true;
        } else if (line.contains(QLatin1String("# NETWORKACCESS")) && !m_networkEnabled) {
            uncommentNextLine = true;
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

        if (uncommentNextLine) {
            out << comment << line << endl;
            uncommentNextLine = false;
            continue;
        }
        out << line << endl;
    };

    return proFileContent.replace(deploymentPriOrigRelFilePath.toAscii(),
        deploymentPriFileName.toAscii());
}

QByteArray MobileApp::generateDesktopFile(const QString *errorMessage) const
{
    Q_UNUSED(errorMessage);

    QFile desktopTemplate(path(DesktopOrigin));
    desktopTemplate.open(QIODevice::ReadOnly);
    Q_ASSERT(desktopTemplate.isOpen());
    QByteArray desktopFileContent = desktopTemplate.readAll();
    return desktopFileContent.replace("thisApp", projectName().toUtf8());
}

QString MobileApp::templatesRoot(const QString &dirName)
{
    return Core::ICore::instance()->resourcePath()
        + QLatin1String("/templates/") + dirName;
}

static Core::GeneratedFile file(const QByteArray &data, const QString &targetFile)
{
    Core::GeneratedFile generatedFile(targetFile);
    generatedFile.setBinary(true);
    generatedFile.setBinaryContents(data);
    return generatedFile;
}

Core::GeneratedFiles MobileApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files;

    files.append(file(generateFile(MobileAppGeneratedFileInfo::AppProFile, errorMessage), path(AppPro)));
    files.last().setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    files.append(file(generateFile(MobileAppGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::SymbianSvgIconFile, errorMessage), path(SymbianSvgIcon)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::MaemoPngIconFile, errorMessage), path(MaemoPngIcon)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::DesktopFile, errorMessage), path(Desktop)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::DeploymentPriFile, errorMessage), path(DeploymentPri)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::MainWindowCppFile, errorMessage), path(MainWindowCpp)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::MainWindowHFile, errorMessage), path(MainWindowH)));
    files.append(file(generateFile(MobileAppGeneratedFileInfo::MainWindowUiFile, errorMessage), path(MainWindowUi)));
    files.last().setAttributes(Core::GeneratedFile::OpenEditorAttribute);

    return files;
}

QString MobileApp::error() const
{
    return m_error;
}

static QByteArray readBlob(const QString &source)
{
    QFile sourceFile(source);
    sourceFile.open(QIODevice::ReadOnly);
    Q_ASSERT(sourceFile.isOpen());
    return sourceFile.readAll();
}

QByteArray MobileApp::generateFile(MobileAppGeneratedFileInfo::File file,
                                          const QString *errorMessage) const
{
    QByteArray data;
    const QString cFileComment = QLatin1String("//");
    const QString proFileComment = QLatin1String("#");
    QString comment = cFileComment;
    bool versionAndChecksum = false;
    switch (file) {
        case MobileAppGeneratedFileInfo::MainCppFile:
            data = generateMainCpp(errorMessage);
            break;
        case MobileAppGeneratedFileInfo::SymbianSvgIconFile:
            data = readBlob(path(SymbianSvgIconOrigin));
            break;
        case MobileAppGeneratedFileInfo::MaemoPngIconFile:
            data = readBlob(path(MaemoPngIconOrigin));
            break;
        case MobileAppGeneratedFileInfo::DesktopFile:
            data = generateDesktopFile(errorMessage);
            break;
        case MobileAppGeneratedFileInfo::AppProFile:
            data = generateProFile(errorMessage);
            comment = proFileComment;
            break;
        case MobileAppGeneratedFileInfo::DeploymentPriFile:
            data = readBlob(path(DeploymentPriOrigin));
            comment = proFileComment;
            versionAndChecksum = true;
            break;
        case MobileAppGeneratedFileInfo::MainWindowCppFile:
            data = readBlob(path(MainWindowCppOrigin));
            versionAndChecksum = true;
            break;
        case MobileAppGeneratedFileInfo::MainWindowHFile:
            data = readBlob(path(MainWindowHOrigin));
            versionAndChecksum = true;
            break;
        case MobileAppGeneratedFileInfo::MainWindowUiFile:
            data = readBlob(path(MainWindowUiOrigin));
            break;
        default:
            Q_ASSERT_X(false, Q_FUNC_INFO, "Whoops, case missing!");
    }
    if (!versionAndChecksum)
        return data;
    QByteArray versioned = data;
    versioned.replace('\x0D', "");
    versioned.replace('\x0A', "");
    const quint16 checkSum = qChecksum(versioned.constData(), versioned.length());
    const QString checkSumString = QString::number(checkSum, 16);
    const QString versionString = QString::number(stubVersion());
    const QChar sep = QLatin1Char(' ');
    const QString versionLine =
            comment + sep + fileChecksum + sep + QLatin1String("0x") + checkSumString
            + sep + fileStubVersion + sep + versionString + QLatin1Char('\x0A');
    return versionLine.toAscii() + data;
}

int MobileApp::stubVersion()
{
    return 1;
}

} // namespace Internal
} // namespace Qt4ProjectManager
