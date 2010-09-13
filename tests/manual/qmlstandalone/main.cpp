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

#include "qmlstandaloneapp.h"
#include <QtCore>

using namespace Qt4ProjectManager::Internal;

static bool writeFile(const QByteArray &data, const QString &targetFile)
{
    const QFileInfo fileInfo(targetFile);
    QDir().mkpath(fileInfo.absolutePath());
    QFile file(fileInfo.absoluteFilePath());
    file.open(QIODevice::WriteOnly);
    Q_ASSERT(file.isOpen());
    return file.write(data) != -1;
}

bool QmlStandaloneApp::generateFiles(QString *errorMessage) const
{
    return     writeFile(generateFile(QmlAppGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::AppProFile, errorMessage), path(AppPro))
            && (useExistingMainQml() ? true : writeFile(generateFile(QmlAppGeneratedFileInfo::MainQmlFile, errorMessage), path(MainQml)))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::SymbianSvgIconFile, errorMessage), path(SymbianSvgIcon))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::MaemoPngIconFile, errorMessage), path(MaemoPngIcon));
}

QString QmlStandaloneApp::templatesRoot()
{
    return QLatin1String("../../../share/qtcreator/templates/");
}

int main(int argc, char *argv[])
{
    QString errorMessage;

    const QString projectPath = QLatin1String("testprojects");

    {
        QmlStandaloneApp sAppNew;
        sAppNew.setProjectPath(projectPath);
        sAppNew.setProjectName(QLatin1String("new_qml_app"));
        if (!sAppNew.generateFiles(&errorMessage))
           return 1;
    }

    {
        QmlStandaloneApp sAppImport01;
        sAppImport01.setProjectPath(projectPath);
        sAppImport01.setProjectName(QLatin1String("imported_scenario_01"));
        sAppImport01.setMainQmlFile(QLatin1String("../qmlstandalone/qmlimportscenario_01/myqmlapp.qml"));
        if (!sAppImport01.generateFiles(&errorMessage))
            return 1;
    }

    {
        const QString rootPath = QLatin1String("../qmlstandalone/qmlimportscenario_02/");
        QmlStandaloneApp sAppImport02;
        sAppImport02.setProjectPath(projectPath);
        sAppImport02.setProjectName(QLatin1String("imported_scenario_02"));
        sAppImport02.setMainQmlFile(rootPath + QLatin1String("subfolder1/myqmlapp.qml"));
        QStringList moduleNames;
        moduleNames.append(QLatin1String("no.trolltech.QmlModule01"));
        moduleNames.append(QLatin1String("com.nokia.QmlModule02"));
        QStringList importPaths;
        importPaths.append(rootPath + QLatin1String("subfolder2/"));
        importPaths.append(rootPath + QLatin1String("subfolder3/"));
        if (!sAppImport02.setExternalModules(moduleNames, importPaths)) {
            qDebug() << sAppImport02.error();
            return 2;
        }
        if (!sAppImport02.generateFiles(&errorMessage))
            return 1;
    }

    return 0;
}
