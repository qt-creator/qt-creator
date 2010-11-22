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

#include "qmlobservertool.h"

#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <projectexplorer/project.h>
#include <QDesktopServices>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

namespace Qt4ProjectManager {

static inline QStringList validBinaryFilenames()
{
    return QStringList()
            << QLatin1String("debug/qmlobserver.exe")
            << QLatin1String("qmlobserver.exe")
            << QLatin1String("qmlobserver")
            << QLatin1String("QMLObserver.app/Contents/MacOS/QMLObserver");
}

bool QmlObserverTool::canBuild(const QtVersion *qtVersion)
{
    return (qtVersion->supportsTargetId(Constants::DESKTOP_TARGET_ID)
            || qtVersion->supportsTargetId(Constants::QT_SIMULATOR_TARGET_ID))
            && checkMinimumQtVersion(qtVersion->qtVersionString(), 4, 7, 1);
}

QString QmlObserverTool::toolForProject(ProjectExplorer::Project *project)
{
    if (project->id() == Qt4ProjectManager::Constants::QT4PROJECT_ID) {
        Qt4Project *qt4Project = static_cast<Qt4Project*>(project);
        if (qt4Project && qt4Project->activeTarget()
         && qt4Project->activeTarget()->activeBuildConfiguration()) {
            QtVersion *version = qt4Project->activeTarget()->activeBuildConfiguration()->qtVersion();
            if (version->isValid()) {
                QString qtInstallData = version->versionInfo().value("QT_INSTALL_DATA");
                QString toolPath = toolByInstallData(qtInstallData);
                return toolPath;
            }
        }
    }
    return QString();
}

QString QmlObserverTool::toolByInstallData(const QString &qtInstallData)
{
    if (!Core::ICore::instance())
        return QString();

    const QString mainFilename = Core::ICore::instance()->resourcePath()
            + QLatin1String("/qml/qmlobserver/main.cpp");
    const QStringList directories = installDirectories(qtInstallData);
    const QStringList binFilenames = validBinaryFilenames();

    return byInstallDataHelper(mainFilename, directories, binFilenames);
}

QStringList QmlObserverTool::locationsByInstallData(const QString &qtInstallData)
{
    QStringList result;
    QFileInfo fileInfo;
    const QStringList binFilenames = validBinaryFilenames();
    foreach(const QString &directory, installDirectories(qtInstallData)) {
        if (getHelperFileInfoFor(binFilenames, directory, &fileInfo))
            result << fileInfo.filePath();
    }
    return result;
}

bool  QmlObserverTool::build(const QString &directory, const QString &makeCommand,
                             const QString &qmakeCommand, const QString &mkspec,
                             const Utils::Environment &env, const QString &targetMode,
                             QString *output,  QString *errorMessage)
{
    return buildHelper(QCoreApplication::translate("Qt4ProjectManager::QmlObserverTool", "QMLObserver"),
                       QLatin1String("qmlobserver.pro"),
                       directory, makeCommand, qmakeCommand, mkspec, env, targetMode,
                       output, errorMessage);
}

static inline bool mkpath(const QString &targetDirectory, QString *errorMessage)
{
    if (!QDir().mkpath(targetDirectory)) {
        *errorMessage = QCoreApplication::translate("ProjectExplorer::QmlObserverTool", "The target directory %1 could not be created.").arg(targetDirectory);
        return false;
    }
    return true;
}

QString QmlObserverTool::copy(const QString &qtInstallData, QString *errorMessage)
{
    const QStringList directories = QmlObserverTool::installDirectories(qtInstallData);

    QStringList files;
    files << QLatin1String("main.cpp") << QLatin1String("qmlobserver.pro")
          << QLatin1String("deviceorientation.cpp") << QLatin1String("deviceorientation.h")
          << QLatin1String("deviceorientation_maemo5.cpp") << QLatin1String("Info_mac.plist")
          << QLatin1String("loggerwidget.cpp") << QLatin1String("loggerwidget.h")
          << QLatin1String("proxysettings.cpp") << QLatin1String("proxysettings.h")
          << QLatin1String("proxysettings.ui") << QLatin1String("proxysettings_maemo5.ui")
          << QLatin1String("qdeclarativetester.cpp") << QLatin1String("qdeclarativetester.h")
          << QLatin1String("qml.icns") << QLatin1String("qml.pri")
          << QLatin1String("qmlruntime.cpp") << QLatin1String("qmlruntime.h")
          << QLatin1String("qmlruntime.qrc") << QLatin1String("recopts.ui")
          << QLatin1String("recopts_maemo5.ui")
          << QLatin1String("texteditautoresizer_maemo5.h")
          << QLatin1String("content/Browser.qml") << QLatin1String("content/images/folder.png")
          << QLatin1String("content/images/titlebar.png") << QLatin1String("content/images/titlebar.sci")
          << QLatin1String("content/images/up.png")
          << QLatin1String("LICENSE.LGPL") << QLatin1String("LGPL_EXCEPTION.TXT");

    QStringList debuggerLibFiles;
    debuggerLibFiles << QLatin1String("jsdebuggeragent.cpp")
          << QLatin1String("qdeclarativeobserverservice.cpp") << QLatin1String("qdeclarativeviewobserver.cpp")
          << QLatin1String("qdeclarativeviewobserver_p.h") << QLatin1String("qmljsdebugger.pri")
          << QLatin1String("qmljsdebugger.pro") << QLatin1String("qmljsdebugger-lib.pri")
          << QLatin1String("include/jsdebuggeragent.h") << QLatin1String("include/qdeclarativeobserverservice.h")
          << QLatin1String("include/qdeclarativeviewobserver.h") << QLatin1String("include/qmljsdebugger_global.h")
          << QLatin1String("include/qmlobserverconstants.h")
          << QLatin1String("include/qt_private/qdeclarativedebughelper_p.h")
          << QLatin1String("include/qt_private/qdeclarativedebugservice_p.h");

    QStringList debuggerLibEditorFiles;
    debuggerLibEditorFiles << QLatin1String("abstractformeditortool.cpp") << QLatin1String("abstractformeditortool.h")
          << QLatin1String("boundingrecthighlighter.cpp") << QLatin1String("boundingrecthighlighter.h")
          << QLatin1String("colorpickertool.cpp") << QLatin1String("colorpickertool.h")
          << QLatin1String("editor.qrc")
          << QLatin1String("layeritem.cpp") << QLatin1String("layeritem.h")
          << QLatin1String("qmltoolbar.cpp") << QLatin1String("qmltoolbar.h")
          << QLatin1String("rubberbandselectionmanipulator.cpp")
          << QLatin1String("rubberbandselectionmanipulator.h") << QLatin1String("selectionindicator.cpp")
          << QLatin1String("selectionindicator.h") << QLatin1String("selectionrectangle.cpp")
          << QLatin1String("selectionrectangle.h") << QLatin1String("selectiontool.cpp")
          << QLatin1String("selectiontool.h") << QLatin1String("singleselectionmanipulator.cpp")
          << QLatin1String("singleselectionmanipulator.h") << QLatin1String("subcomponenteditortool.cpp")
          << QLatin1String("subcomponenteditortool.h") << QLatin1String("subcomponentmasklayeritem.cpp")
          << QLatin1String("subcomponentmasklayeritem.h") << QLatin1String("toolbarcolorbox.cpp")
          << QLatin1String("toolbarcolorbox.h") << QLatin1String("zoomtool.cpp")
          << QLatin1String("zoomtool.h") << QLatin1String("images/color-picker.png")
          << QLatin1String("images/color-picker-24.png") << QLatin1String("images/color-picker-hicontrast.png")
          << QLatin1String("images/from-qml.png") << QLatin1String("images/from-qml-24.png")
          << QLatin1String("images/observermode.png") << QLatin1String("images/observermode-24.png")
          << QLatin1String("images/pause.png") << QLatin1String("images/pause-24.png")
          << QLatin1String("images/play.png") << QLatin1String("images/play-24.png")
          << QLatin1String("images/reload.png") << QLatin1String("images/resize_handle.png")
          << QLatin1String("images/select.png") << QLatin1String("images/select-24.png")
          << QLatin1String("images/select-marquee.png") << QLatin1String("images/select-marquee-24.png")
          << QLatin1String("images/to-qml.png") << QLatin1String("images/to-qml-24.png")
          << QLatin1String("images/zoom.png") << QLatin1String("images/zoom-24.png");

    QString sourcePath = Core::ICore::instance()->resourcePath() + QLatin1String("/qml/qmlobserver/");
    QString libSourcePath = Core::ICore::instance()->resourcePath() + QLatin1String("/qml/qmljsdebugger/");
    QString libEditorSourcePath = Core::ICore::instance()->resourcePath() + QLatin1String("/qml/qmljsdebugger/editor/");

    // Try to find a writeable directory.
    foreach(const QString &directory, directories) {
        if (!mkpath(directory + QLatin1String("/content/images"), errorMessage)
            || !mkpath(directory + QLatin1String("/images"), errorMessage)
            || !mkpath(directory + QLatin1String("/qmljsdebugger/editor/images"), errorMessage)
            || !mkpath(directory + QLatin1String("/qmljsdebugger/include"), errorMessage)
            || !mkpath(directory + QLatin1String("/qmljsdebugger/include/qt_private"), errorMessage))
        {
            continue;
        } else {
            errorMessage->clear();
        }

        if (copyFiles(sourcePath, files, directory, errorMessage)
         && copyFiles(libSourcePath, debuggerLibFiles, directory + QLatin1String("/qmljsdebugger/"), errorMessage)
         && copyFiles(libEditorSourcePath, debuggerLibEditorFiles, directory + QLatin1String("/qmljsdebugger/editor/"), errorMessage))
        {
            errorMessage->clear();
            return directory;
        }
    }
    *errorMessage = QCoreApplication::translate("ProjectExplorer::QmlObserverTool",
                                                "QMLObserver could not be built in any of the directories:\n- %1\n\nReason: %2")
                    .arg(directories.join(QLatin1String("\n- ")), *errorMessage);
    return QString();
}

QStringList QmlObserverTool::installDirectories(const QString &qtInstallData)
{
    const QChar slash = QLatin1Char('/');
    const uint hash = qHash(qtInstallData);
    QStringList directories;
    directories
            << (qtInstallData + QLatin1String("/qtc-qmlobserver/"))
            << QDir::cleanPath((QCoreApplication::applicationDirPath() + QLatin1String("/../qtc-qmlobserver/") + QString::number(hash))) + slash
            << (QDesktopServices::storageLocation(QDesktopServices::DataLocation) + QLatin1String("/qtc-qmlobserver/") + QString::number(hash)) + slash;
    return directories;
}

} // namespace
