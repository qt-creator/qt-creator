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
#include <QtCore>

using namespace Qt4ProjectManager::Internal;

bool processXmlFile(const QString &xmlFile)
{
    QFile file(xmlFile);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QLatin1String tag_app("app");
    const QLatin1String attrib_mainQmlFile("mainqmlfile");
    const QLatin1String attrib_projectPath("projectpath");
    const QLatin1String attrib_projectName("projectname");
    const QLatin1String attrib_screenOrientation("screenorientation");
    const QLatin1String value_screenOrientationLockLandscape("LockLandscape");
    const QLatin1String value_screenOrientationLockPortrait("LockPortrait");
    const QLatin1String attrib_networkAccess("networkaccess");

    static const QString qtDir =
            QLibraryInfo::location(QLibraryInfo::PrefixPath) + QLatin1Char('/');

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        const QXmlStreamReader::TokenType token = reader.readNext();
        switch (token) {
            case QXmlStreamReader::StartElement:
                if (reader.name() == tag_app) {
                    QtQuickApp app;
                    QFileInfo projectPath;
                    if (!reader.attributes().hasAttribute(attrib_projectPath)) {
                        qDebug() << "Project without path found";
                        continue;
                    }
                    projectPath = qtDir + reader.attributes().value(attrib_projectPath).toString();
                    app.setProjectPath(projectPath.absoluteFilePath());
                    if (reader.attributes().hasAttribute(attrib_mainQmlFile)) {
                        const QFileInfo qmlFileOrigin(
                                qtDir + reader.attributes().value(attrib_mainQmlFile).toString());
                        if (!qmlFileOrigin.exists()) {
                            qDebug() << "Cannot find" <<
                                        QDir::toNativeSeparators(qmlFileOrigin.absoluteFilePath());
                            continue;
                        }
                        const QFileInfo qmlTargetPath(QString(projectPath.absoluteFilePath()
                                                              + QLatin1Char('/') + qmlFileOrigin.baseName()
                                                              + QLatin1String("/qml")));
#ifdef Q_OS_WIN
                        const QString sourcePath =
                                QDir::toNativeSeparators(qmlFileOrigin.canonicalPath() + QLatin1String("/*"));
                        const QString targetPath =
                                QDir::toNativeSeparators(qmlTargetPath.absoluteFilePath() + QLatin1Char('/'));
                        QProcess xcopy;
                        QStringList parameters;
                        parameters << QLatin1String("/E") << sourcePath << targetPath;
                        xcopy.start(QLatin1String("xcopy.exe"), parameters);
                        if (!xcopy.waitForStarted() || !xcopy.waitForFinished()) {
                            qDebug() << "Could not copy" <<
                                        QDir::toNativeSeparators(sourcePath);
                            continue;
                        }
#else // Q_OS_WIN
                        // Implement me!
#endif // Q_OS_WIN
                        app.setMainQmlFile(qmlTargetPath.absoluteFilePath()
                                              + QLatin1Char('/') + qmlFileOrigin.fileName());
                    }
                    app.setProjectName(reader.attributes().hasAttribute(attrib_projectName)
                                            ? reader.attributes().value(attrib_projectName).toString()
                                            : QFileInfo(app.mainQmlFile()).baseName());
                    if (reader.attributes().hasAttribute(attrib_screenOrientation)) {
                        const QStringRef orientation = reader.attributes().value(attrib_screenOrientation);
                        app.setOrientation(orientation == value_screenOrientationLockLandscape ?
                                                AbstractMobileApp::ScreenOrientationLockLandscape
                                              : orientation == value_screenOrientationLockPortrait ?
                                                AbstractMobileApp::ScreenOrientationLockPortrait
                                              : AbstractMobileApp::ScreenOrientationAuto);
                    }
                    if (reader.attributes().hasAttribute(attrib_networkAccess))
                        app.setNetworkEnabled(
                                    reader.attributes().value(attrib_networkAccess).toString() == QLatin1String("true"));
                    if (!app.generateFiles(0))
                        qDebug() << "Unable to generate the files for" << app.projectName();
                }
                break;
            default:
                break;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    if (!processXmlFile(QLatin1String(":/qtquickapps.xml")))
        return 1;

    return 0;
}
