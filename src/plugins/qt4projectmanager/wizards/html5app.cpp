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

#include "html5app.h"

#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <QCoreApplication>

#ifndef CREATORLESSTEST
#include <coreplugin/icore.h>
#endif // CREATORLESSTEST

namespace Qt4ProjectManager {
namespace Internal {

const QString appViewerBaseName(QLatin1String("html5applicationviewer"));
const QString appViewerPriFileName(appViewerBaseName + QLatin1String(".pri"));
const QString appViewerCppFileName(appViewerBaseName + QLatin1String(".cpp"));
const QString appViewerHFileName(appViewerBaseName + QLatin1String(".h"));
const QString appViewerOriginsSubDir(appViewerBaseName + QLatin1Char('/'));

Html5App::Html5App()
    : AbstractMobileApp()
    , m_mainHtmlMode(ModeGenerate)
    , m_touchOptimizedNavigationEnabled(false)
{
}

Html5App::~Html5App()
{
}

void Html5App::setMainHtml(Mode mode, const QString &data)
{
    Q_ASSERT(mode != ModeGenerate || data.isEmpty());
    m_mainHtmlMode = mode;
    m_mainHtmlData = data;
}

Html5App::Mode Html5App::mainHtmlMode() const
{
    return m_mainHtmlMode;
}

void Html5App::setTouchOptimizedNavigationEnabled(bool enabled)
{
    m_touchOptimizedNavigationEnabled = enabled;
}

bool Html5App::touchOptimizedNavigationEnabled() const
{
    return m_touchOptimizedNavigationEnabled;
}

QString Html5App::pathExtended(int fileType) const
{
    const QString appViewerTargetSubDir = appViewerOriginsSubDir;
    const QString indexHtml = QLatin1String("index.html");
    const QString pathBase = outputPathBase();
    const QDir appProFilePath(pathBase);
    const bool generateHtml = m_mainHtmlMode == ModeGenerate;
    const bool importHtml = m_mainHtmlMode == ModeImport;
    const QFileInfo importedHtmlFile(m_mainHtmlData);
    const QString htmlSubDir = importHtml ? importedHtmlFile.canonicalPath().split(QLatin1Char('/')).last() + QLatin1Char('/')
                                          : QString::fromLatin1("html/");

    switch (fileType) {
        case MainHtml:                      return generateHtml ? pathBase + htmlSubDir + indexHtml
                                                                : importedHtmlFile.canonicalFilePath();
        case MainHtmlDeployed:              return generateHtml ? QString(htmlSubDir + indexHtml)
                                                                : htmlSubDir + importedHtmlFile.fileName();
        case MainHtmlOrigin:                return originsRoot() + QLatin1String("html/") + indexHtml;
        case AppViewerPri:                  return pathBase + appViewerTargetSubDir + appViewerPriFileName;
        case AppViewerPriOrigin:            return originsRoot() + appViewerOriginsSubDir + appViewerPriFileName;
        case AppViewerCpp:                  return pathBase + appViewerTargetSubDir + appViewerCppFileName;
        case AppViewerCppOrigin:            return originsRoot() + appViewerOriginsSubDir + appViewerCppFileName;
        case AppViewerH:                    return pathBase + appViewerTargetSubDir + appViewerHFileName;
        case AppViewerHOrigin:              return originsRoot() + appViewerOriginsSubDir + appViewerHFileName;
        case HtmlDir:                       return pathBase + htmlSubDir;
        case HtmlDirProFileRelative:        return generateHtml ? QString(htmlSubDir).remove(htmlSubDir.length() - 1, 1)
                                                                : appProFilePath.relativeFilePath(importedHtmlFile.canonicalPath());
        default:                            qFatal("Html5App::pathExtended() needs more work");
    }
    return QString();
}

QString Html5App::originsRoot() const
{
    return templatesRoot() + QLatin1String("html5app/");
}

QString Html5App::mainWindowClassName() const
{
    return QLatin1String("Html5ApplicationViewer");
}

bool Html5App::adaptCurrentMainCppTemplateLine(QString &line) const
{
    const QLatin1Char quote('"');
    bool adaptLine = true;
    if (line.contains(QLatin1String("// MAINHTMLFILE"))) {
        if (m_mainHtmlMode != ModeUrl)
            insertParameter(line, quote + path(MainHtmlDeployed) + quote);
        else
            adaptLine = false;
    } else if (line.contains(QLatin1String("// MAINHTMLURL"))) {
        if (m_mainHtmlMode == ModeUrl)
            insertParameter(line, quote + m_mainHtmlData + quote);
        else
            adaptLine = false;
    }
    return adaptLine;
}

void Html5App::handleCurrentProFileTemplateLine(const QString &line,
    QTextStream &proFileTemplate, QTextStream &proFile,
    bool &commentOutNextLine) const
{
    Q_UNUSED(proFileTemplate)
    Q_UNUSED(proFile)
    if (line.contains(QLatin1String("# TOUCH_OPTIMIZED_NAVIGATION")))
        commentOutNextLine = !m_touchOptimizedNavigationEnabled;
}

#ifndef CREATORLESSTEST
Core::GeneratedFiles Html5App::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files = AbstractMobileApp::generateFiles(errorMessage);
    if (m_mainHtmlMode == ModeGenerate) {
        files.append(file(generateFile(Html5AppGeneratedFileInfo::MainHtmlFile, errorMessage), path(MainHtml)));
        files.last().setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    }

    files.append(file(generateFile(Html5AppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri)));
    files.append(file(generateFile(Html5AppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp)));
    files.append(file(generateFile(Html5AppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH)));

    return files;
}
#endif // CREATORLESSTEST

QByteArray Html5App::appViewerCppFileCode(QString *errorMessage) const
{
    static const char* touchNavigavigationFiles[] = {
        "webtouchphysicsinterface.h",
        "webtouchphysics.h",
        "webtouchevent.h",
        "webtouchscroller.h",
        "webtouchnavigation.h",
        "webnavigation.h",
        "navigationcontroller.h",
        "webtouchphysicsinterface.cpp",
        "webtouchphysics.cpp",
        "webtouchevent.cpp",
        "webtouchscroller.cpp",
        "webtouchnavigation.cpp",
        "webnavigation.cpp",
        "navigationcontroller.cpp",
    };
    static const QString touchNavigavigationDir =
            originsRoot() + appViewerOriginsSubDir + QLatin1String("touchnavigation/");
    QByteArray touchNavigavigationCode;
    for (size_t i = 0; i < sizeof(touchNavigavigationFiles) / sizeof(touchNavigavigationFiles[0]); ++i) {
        QFile touchNavigavigationFile(touchNavigavigationDir + QLatin1String(touchNavigavigationFiles[i]));
        if (!touchNavigavigationFile.open(QIODevice::ReadOnly)) {
            if (errorMessage)
                *errorMessage = QCoreApplication::translate("Qt4ProjectManager::AbstractMobileApp",
                    "Could not open template file '%1'.").arg(QLatin1String(touchNavigavigationFiles[i]));
            return QByteArray();
        }
        QTextStream touchNavigavigationFileIn(&touchNavigavigationFile);
        QString line;
        while (!(line = touchNavigavigationFileIn.readLine()).isNull()) {
            if (line.startsWith(QLatin1String("#include")) ||
                    ((line.startsWith(QLatin1String("#ifndef"))
                      || line.startsWith(QLatin1String("#define"))
                      || line.startsWith(QLatin1String("#endif")))
                    && line.endsWith(QLatin1String("_H"))))
                continue;
            touchNavigavigationCode.append(line.toLatin1());
            touchNavigavigationCode.append('\n');
        }
    }

    QFile appViewerCppFile(path(AppViewerCppOrigin));
    if (!appViewerCppFile.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QCoreApplication::translate("Qt4ProjectManager::AbstractMobileApp",
                "Could not open template file '%1'.").arg(path(AppViewerCppOrigin));
        return QByteArray();
    }
    QTextStream in(&appViewerCppFile);
    QByteArray appViewerCppCode;
    bool touchNavigavigationCodeInserted = false;
    QString line;
    while (!(line = in.readLine()).isNull()) {
        if (!touchNavigavigationCodeInserted && line == QLatin1String("#ifdef TOUCH_OPTIMIZED_NAVIGATION")) {
            appViewerCppCode.append(line.toLatin1());
            appViewerCppCode.append('\n');
            while (!(line = in.readLine()).isNull()
                && !line.contains(QLatin1String("#endif // TOUCH_OPTIMIZED_NAVIGATION")))
            {
                if (!line.startsWith(QLatin1String("#include \""))) {
                    appViewerCppCode.append(line.toLatin1());
                    appViewerCppCode.append('\n');
                }
            }
            appViewerCppCode.append(touchNavigavigationCode);
            touchNavigavigationCodeInserted = true;
        }
        appViewerCppCode.append(line.toLatin1());
        appViewerCppCode.append('\n');
    }
    return appViewerCppCode;
}

QByteArray Html5App::generateFileExtended(int fileType,
    bool *versionAndCheckSum, QString *comment, QString *errorMessage) const
{
    QByteArray data;
    switch (fileType) {
        case Html5AppGeneratedFileInfo::MainHtmlFile:
            data = readBlob(path(MainHtmlOrigin), errorMessage);
            break;
        case Html5AppGeneratedFileInfo::AppViewerPriFile:
            data = readBlob(path(AppViewerPriOrigin), errorMessage);
            data.append(readBlob(path(DeploymentPriOrigin), errorMessage));
            *comment = ProFileComment;
            *versionAndCheckSum = true;
            break;
        case Html5AppGeneratedFileInfo::AppViewerCppFile:
            data = appViewerCppFileCode(errorMessage);
            *versionAndCheckSum = true;
            break;
        case Html5AppGeneratedFileInfo::AppViewerHFile:
        default:
            data = readBlob(path(AppViewerHOrigin), errorMessage);
            *versionAndCheckSum = true;
            break;
    }
    return data;
}

int Html5App::stubVersionMinor() const
{
    return StubVersion;
}

QList<AbstractGeneratedFileInfo> Html5App::updateableFiles(const QString &mainProFile) const
{
    QList<AbstractGeneratedFileInfo> result;
    static const struct {
        int fileType;
        QString fileName;
    } files[] = {
        {Html5AppGeneratedFileInfo::AppViewerPriFile, appViewerPriFileName},
        {Html5AppGeneratedFileInfo::AppViewerHFile, appViewerHFileName},
        {Html5AppGeneratedFileInfo::AppViewerCppFile, appViewerCppFileName}
    };
    const QFileInfo mainProFileInfo(mainProFile);
    const int size = sizeof(files) / sizeof(files[0]);
    for (int i = 0; i < size; ++i) {
        const QString fileName = mainProFileInfo.dir().absolutePath()
                + QLatin1Char('/') + appViewerOriginsSubDir + files[i].fileName;
        if (!QFile::exists(fileName))
            continue;
        Html5AppGeneratedFileInfo file;
        file.fileType = files[i].fileType;
        file.fileInfo = QFileInfo(fileName);
        file.currentVersion = AbstractMobileApp::makeStubVersion(Html5App::StubVersion);
        result.append(file);
    }
    if (result.count() != size)
        result.clear(); // All files must be found. No wrong/partial updates, please.
    return result;
}

QList<DeploymentFolder> Html5App::deploymentFolders() const
{
    QList<DeploymentFolder> result;
    if (m_mainHtmlMode != ModeUrl)
        result.append(DeploymentFolder(path(HtmlDirProFileRelative), QLatin1String(".")));
    return result;
}

const int Html5App::StubVersion = 11;

} // namespace Internal
} // namespace Qt4ProjectManager
