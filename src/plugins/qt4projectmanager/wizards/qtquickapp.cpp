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

#include "qtquickapp.h"

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

QtQuickApp::QtQuickApp()
    : AbstractMobileApp()
    , m_mainQmlMode(ModeGenerate)
    , m_componentSet(QtQuick10Components)
{
    m_canSupportMeegoBooster = true;
}

void QtQuickApp::setComponentSet(ComponentSet componentSet)
{
    m_componentSet = componentSet;
}

QtQuickApp::ComponentSet QtQuickApp::componentSet() const
{
    return m_componentSet;
}

void QtQuickApp::setMainQml(Mode mode, const QString &file)
{
    Q_ASSERT(mode != ModeGenerate || file.isEmpty());
    m_mainQmlMode = mode;
    m_mainQmlFile.setFile(file);
}

QtQuickApp::Mode QtQuickApp::mainQmlMode() const
{
    return m_mainQmlMode;
}

QString QtQuickApp::pathExtended(int fileType) const
{
    const bool importQmlFile = m_mainQmlMode == ModeImport;
    const QString qmlSubDir = QLatin1String("qml/")
                              + (importQmlFile ? m_mainQmlFile.dir().dirName() : projectName())
                              + QLatin1Char('/');
    const QString appViewerTargetSubDir = appViewerOriginSubDir();

    const QString mainQmlFile = QLatin1String("main.qml");
    const QString mainPageQmlFile = QLatin1String("MainPage.qml");

    const QString qmlOriginDir = originsRoot() + QLatin1String("qml/app/")
                        + componentSetDir(componentSet()) + QLatin1Char('/');

    const QString pathBase = outputPathBase();
    const QDir appProFilePath(pathBase);

    switch (fileType) {
        case MainQml:
            return importQmlFile ? m_mainQmlFile.canonicalFilePath() : pathBase + qmlSubDir + mainQmlFile;
        case MainQmlDeployed:               return importQmlFile ? qmlSubDir + m_mainQmlFile.fileName()
                                                                 : QString(qmlSubDir + mainQmlFile);
        case MainQmlOrigin:                 return qmlOriginDir + mainQmlFile;
        case MainPageQml:                   return pathBase + qmlSubDir + mainPageQmlFile;
        case MainPageQmlOrigin:             return qmlOriginDir + mainPageQmlFile;
        case AppViewerPri:                  return pathBase + appViewerTargetSubDir + fileName(AppViewerPri);
        case AppViewerPriOrigin:            return originsRoot() + appViewerOriginSubDir() + fileName(AppViewerPri);
        case AppViewerCpp:                  return pathBase + appViewerTargetSubDir + fileName(AppViewerCpp);
        case AppViewerCppOrigin:            return originsRoot() + appViewerOriginSubDir() + fileName(AppViewerCpp);
        case AppViewerH:                    return pathBase + appViewerTargetSubDir + fileName(AppViewerH);
        case AppViewerHOrigin:              return originsRoot() + appViewerOriginSubDir() + fileName(AppViewerH);
        case QmlDir:                        return pathBase + qmlSubDir;
        case QmlDirProFileRelative:         return importQmlFile ? appProFilePath.relativeFilePath(m_mainQmlFile.canonicalPath())
                                                                 : QString(qmlSubDir).remove(qmlSubDir.length() - 1, 1);
        default:                            qFatal("QtQuickApp::pathExtended() needs more work");
    }
    return QString();
}

QString QtQuickApp::originsRoot() const
{
    const bool isQtQuick2 = m_componentSet == QtQuick20Components;
    return templatesRoot() + QLatin1String(isQtQuick2 ? "qtquick2app/" : "qtquickapp/");
}

QString QtQuickApp::mainWindowClassName() const
{
    return QLatin1String("QmlApplicationViewer");
}

bool QtQuickApp::adaptCurrentMainCppTemplateLine(QString &line) const
{
    const QLatin1Char quote('"');

    if (line.contains(QLatin1String("// MAINQML")))
        insertParameter(line, quote + path(MainQmlDeployed) + quote);

    return true;
}

void QtQuickApp::handleCurrentProFileTemplateLine(const QString &line,
    QTextStream &proFileTemplate, QTextStream &proFile,
    bool &commentOutNextLine) const
{
    Q_UNUSED(commentOutNextLine)
    if (line.contains(QLatin1String("# QML_IMPORT_PATH"))) {
        QString nextLine = proFileTemplate.readLine(); // eats 'QML_IMPORT_PATH ='
        if (!nextLine.startsWith(QLatin1String("QML_IMPORT_PATH =")))
            return;
        proFile << nextLine << endl;
    } else if (line.contains(QLatin1String("# HARMATTAN_BOOSTABLE"))) {
        QString nextLine = proFileTemplate.readLine(); // eats '# CONFIG += qdeclarative-boostable'
        if (supportsMeegoBooster())
            nextLine.remove(0, 2); // remove comment
        proFile << nextLine << endl;
    }
}

#ifndef CREATORLESSTEST
Core::GeneratedFiles QtQuickApp::generateFiles(QString *errorMessage) const
{
    Core::GeneratedFiles files = AbstractMobileApp::generateFiles(errorMessage);
    if (!useExistingMainQml()) {
        files.append(file(generateFile(QtQuickAppGeneratedFileInfo::MainQmlFile, errorMessage), path(MainQml)));
        if ((componentSet() == QtQuickApp::Meego10Components))
            files.append(file(generateFile(QtQuickAppGeneratedFileInfo::MainPageQmlFile, errorMessage), path(MainPageQml)));
        files.last().setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    }

    files.append(file(generateFile(QtQuickAppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri)));
    files.append(file(generateFile(QtQuickAppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp)));
    files.append(file(generateFile(QtQuickAppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH)));

    return files;
}
#endif // CREATORLESSTEST

bool QtQuickApp::useExistingMainQml() const
{
    return !m_mainQmlFile.filePath().isEmpty();
}

QString QtQuickApp::appViewerBaseName() const
{
    return QLatin1String(m_componentSet == QtQuick20Components ?
                             "qtquick2applicationviewer" : "qmlapplicationviewer");
}

QString QtQuickApp::fileName(QtQuickApp::ExtendedFileType type) const
{
    switch (type) {
        case AppViewerPri:      return appViewerBaseName() + QLatin1String(".pri");
        case AppViewerH:        return appViewerBaseName() + QLatin1String(".h");
        case AppViewerCpp:      return appViewerBaseName() + QLatin1String(".cpp");
        default:                return QString();
    }
}

QString QtQuickApp::appViewerOriginSubDir() const
{
    return appViewerBaseName() + QLatin1Char('/');
}

QByteArray QtQuickApp::generateFileExtended(int fileType,
    bool *versionAndCheckSum, QString *comment, QString *errorMessage) const
{
    QByteArray data;
    switch (fileType) {
        case QtQuickAppGeneratedFileInfo::MainQmlFile:
            data = readBlob(path(MainQmlOrigin), errorMessage);
            break;
        case QtQuickAppGeneratedFileInfo::MainPageQmlFile:
            data = readBlob(path(MainPageQmlOrigin), errorMessage);
            break;
        case QtQuickAppGeneratedFileInfo::AppViewerPriFile:
            data = readBlob(path(AppViewerPriOrigin), errorMessage);
            data.append(readBlob(path(DeploymentPriOrigin), errorMessage));
            *comment = ProFileComment;
            *versionAndCheckSum = true;
            break;
        case QtQuickAppGeneratedFileInfo::AppViewerCppFile:
            data = readBlob(path(AppViewerCppOrigin), errorMessage);
            *versionAndCheckSum = true;
            break;
        case QtQuickAppGeneratedFileInfo::AppViewerHFile:
        default:
            data = readBlob(path(AppViewerHOrigin), errorMessage);
            *versionAndCheckSum = true;
            break;
    }
    return data;
}

int QtQuickApp::stubVersionMinor() const
{
    return m_componentSet == QtQuick20Components ? 2 : 22;
}

QList<AbstractGeneratedFileInfo> QtQuickApp::updateableFiles(const QString &mainProFile) const
{
    QList<AbstractGeneratedFileInfo> result;
    static const struct {
        int fileType;
        QString fileName;
    } files[] = {
        {QtQuickAppGeneratedFileInfo::AppViewerPriFile, fileName(AppViewerPri)},
        {QtQuickAppGeneratedFileInfo::AppViewerHFile, fileName(AppViewerH)},
        {QtQuickAppGeneratedFileInfo::AppViewerCppFile, fileName(AppViewerCpp)}
    };
    const QFileInfo mainProFileInfo(mainProFile);
    const int size = sizeof(files) / sizeof(files[0]);
    for (int i = 0; i < size; ++i) {
        const QString fileName = mainProFileInfo.dir().absolutePath()
                + QLatin1Char('/') + appViewerOriginSubDir() + files[i].fileName;
        if (!QFile::exists(fileName))
            continue;
        QtQuickAppGeneratedFileInfo file;
        file.fileType = files[i].fileType;
        file.fileInfo = QFileInfo(fileName);
        file.currentVersion = AbstractMobileApp::makeStubVersion(stubVersionMinor());
        result.append(file);
    }
    if (result.count() != size)
        result.clear(); // All files must be found. No wrong/partial updates, please.
    return result;
}

QList<DeploymentFolder> QtQuickApp::deploymentFolders() const
{
    QList<DeploymentFolder> result;
    result.append(DeploymentFolder(path(QmlDirProFileRelative), QLatin1String("qml")));
    return result;
}

QString QtQuickApp::componentSetDir(ComponentSet componentSet) const
{
    switch (componentSet) {
    case Meego10Components:
        return QLatin1String("meego10");
    case QtQuick20Components:
        return QLatin1String("qtquick20");
    case QtQuick10Components:
    default:
        return QLatin1String("qtquick10");
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
