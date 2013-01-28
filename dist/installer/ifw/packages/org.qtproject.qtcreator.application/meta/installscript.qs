/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt SDK.
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

// constructor
function Component()
{
    component.loaded.connect(this, Component.prototype.loaded);
    installer.finishButtonClicked.connect(this, Component.prototype.installationFinished);
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
}

Component.prototype.loaded = function()
{
    try {
        if (installer.value("os") == "win" && installer.isInstaller())
            installer.addWizardPageItem(component, "AssociateCommonFiletypesForm", QInstaller.TargetDirectory);
    } catch(e) {
        print(e);
    }
}

Component.prototype.beginInstallation = function()
{
    if ( installer.value("os") === "win" ) {
        component.setStopProcessForUpdateRequest("@TargetDir@/bin/qtcreator.exe", true);
        component.setStopProcessForUpdateRequest("@TargetDir@/bin/linguist.exe", true);
        component.setStopProcessForUpdateRequest("@TargetDir@/bin/qmlviewer.exe", true);
    }
}

registerCommonWindowsFileTypeExtensions = function()
{
    var headerExtensions = new Array("h", "hh", "hxx", "h++", "hpp");

    for (var i = 0; i < headerExtensions.length; ++i) {
        component.addOperation( "RegisterFileType",
                                headerExtensions[i],
                                "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                                "C++ Header file",
                                "text/plain",
                                "@TargetDir@\\bin\\qtcreator.exe,3",
                                "ProgId=QtProject.QtCreator." + headerExtensions[i]);
    }

    var cppExtensions = new Array("cc", "cxx", "c++", "cp", "cpp");

    for (var i = 0; i < cppExtensions.length; ++i) {
        component.addOperation( "RegisterFileType",
                                cppExtensions[i],
                                "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                                "C++ Source file",
                                "text/plain",
                                "@TargetDir@\\bin\\qtcreator.exe,2",
                                "ProgId=QtProject.QtCreator." + cppExtensions[i]);
    }

    component.addOperation( "RegisterFileType",
                            "c",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "C Source file",
                            "text/plain",
                            "@TargetDir@\\bin\\qtcreator.exe,1",
                            "ProgId=QtProject.QtCreator.c");
}

registerWindowsFileTypeExtensions = function()
{
    component.addOperation( "RegisterFileType",
                            "ui",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt UI file",
                            "text/plain",
                            "@TargetDir@\\bin\\qtcreator.exe,4",
                            "ProgId=QtProject.QtCreator.ui");
    component.addOperation( "RegisterFileType",
                            "pro",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt Project file",
                            "text/plain",
                            "@TargetDir@\\bin\\qtcreator.exe,5",
                            "ProgId=QtProject.QtCreator.pro");
    component.addOperation( "RegisterFileType",
                            "pri",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt Project Include file",
                            "text/plain",
                            "@TargetDir@\\bin\\qtcreator.exe,6",
                            "ProgId=QtProject.QtCreator.pri");
    component.addOperation( "RegisterFileType",
                            "qs",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt Script file",
                            "text/plain",
                            "@TargetDir@\\bin\\qtcreator.exe,0",
                            "ProgId=QtProject.QtCreator.qs");
    component.addOperation( "RegisterFileType",
                            "qml",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt Quick Markup language file",
                            "text/plain",
                            "@TargetDir@\\bin\\qtcreator.exe,0",
                            "ProgId=QtProject.QtCreator.qml");
}

Component.prototype.createOperations = function()
{
    // Call the base createOperations and afterwards set some registry settings
    component.createOperations();
    if ( installer.value("os") == "win" )
    {
        component.addOperation( "SetPluginPathOnQtCore",
                                "@TargetDir@/bin",
                                "@TargetDir@/plugins");
        component.addOperation( "SetImportsPathOnQtCore",
                                "@TargetDir@/bin",
                                "@TargetDir@/bin");
        component.addOperation( "CreateShortcut",
                                "@TargetDir@\\bin\\qtcreator.exe",
                                "@StartMenuDir@/Qt Creator.lnk",
                                "workingDirectory=@homeDir@" );
        component.addElevatedOperation("Execute", "{0,3010,1638}", "@TargetDir@\\lib\\vcredist_msvc2010\\vcredist_x86.exe", "/norestart", "/q");
        registerWindowsFileTypeExtensions();

        try {
            if (component.userInterface("AssociateCommonFiletypesForm").AssociateCommonFiletypesCheckBox
                .checked) {
                    registerCommonWindowsFileTypeExtensions();
            }
        } catch(e) {
            print(e);
        }
    }
    if ( installer.value("os") == "x11" )
    {
        component.addOperation( "SetPluginPathOnQtCore",
                                "@TargetDir@/lib/qtcreator",
                                "@TargetDir@/lib/qtcreator/plugins");
        component.addOperation( "SetImportsPathOnQtCore",
                                "@TargetDir@/lib/qtcreator",
                                "@TargetDir@/bin");

        component.addOperation( "InstallIcons", "@TargetDir@/share/icons" );
        component.addOperation( "CreateDesktopEntry",
                                "QtProject-qtcreator.desktop",
                                "Type=Application\nExec=@TargetDir@/bin/qtcreator\nPath=@TargetDir@\nName=Qt Creator\nGenericName=The IDE of choice for Qt development.\nGenericName[de]=Die IDE der Wahl zur Qt Entwicklung\nIcon=QtProject-qtcreator\nTerminal=false\nCategories=Development;IDE;Qt;\nMimeType=text/x-c++src;text/x-c++hdr;text/x-xsrc;application/x-designer;application/vnd.nokia.qt.qmakeprofile;application/vnd.nokia.xml.qt.resource;text/x-qml;"
                                );
    }
}

Component.prototype.installationFinished = function()
{
    try {
        if (component.installed && installer.isInstaller() && installer.status == QInstaller.Success) {
            var isLaunchQtCreatorCheckBoxChecked = component.userInterface( "LaunchQtCreatorCheckBoxForm" ).launchQtCreatorCheckBox.checked;
            if (isLaunchQtCreatorCheckBoxChecked) {

                var qtCreatorBinary = installer.value("TargetDir");
                if (installer.value("os") == "win")
                    qtCreatorBinary = qtCreatorBinary + "\\bin\\qtcreator.exe";
                else if (installer.value("os") == "x11")
                    qtCreatorBinary = qtCreatorBinary + "/bin/qtcreator";
                else if (installer.value("os") == "mac")
                    qtCreatorBinary = "\"" + qtCreatorBinary + "/Qt Creator.app/Contents/MacOS/Qt Creator\"";

                if (installer.executeDetached)
                    installer.executeDetached(qtCreatorBinary);
            }
        }
    } catch(e) {
        print(e);
    }
}

