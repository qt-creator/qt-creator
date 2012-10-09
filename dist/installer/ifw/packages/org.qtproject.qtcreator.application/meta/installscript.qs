/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
    installer.finishButtonClicked.connect(this, Component.prototype.installationFinished);
}

Component.prototype.beginInstallation = function()
{
    if ( installer.value("os") === "win" ) {
        component.setStopProcessForUpdateRequest("@TargetDir@/bin/qtcreator.exe", true);
        component.setStopProcessForUpdateRequest("@TargetDir@/bin/linguist.exe", true);
        component.setStopProcessForUpdateRequest("@TargetDir@/bin/qmlviewer.exe", true);
    }
}

registerWindowsFileTypeExtensions = function()
{
    var headerExtensions = new Array("h", "hh", "hxx", "h++", "hpp", "hpp");

    for (var i = 0; i < headerExtensions.length; ++i) {
        component.addOperation( "RegisterFileType",
                                headerExtensions[i],
                                "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                                "C++ Header file",
                                "",
                                "@TargetDir@\\bin\\qtcreator.exe,3");
    }

    var cppExtensions = new Array("cc", "cxx", "c++", "cp", "cpp");

    for (var i = 0; i < cppExtensions.length; ++i) {
        component.addOperation( "RegisterFileType",
                                cppExtensions[i],
                                "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                                "C++ Source file",
                                "",
                                "@TargetDir@\\bin\\qtcreator.exe,2");
    }

    component.addOperation( "RegisterFileType",
                            "c",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "C Source file",
                            "",
                            "@TargetDir@\\bin\\qtcreator.exe,1");
    component.addOperation( "RegisterFileType",
                            "ui",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt UI file",
                            "",
                            "@TargetDir@\\bin\\qtcreator.exe,4");
    component.addOperation( "RegisterFileType",
                            "pro",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt Project file",
                            "",
                            "@TargetDir@\\bin\\qtcreator.exe,5");
    component.addOperation( "RegisterFileType",
                            "pri",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt Project Include file",
                            "",
                            "@TargetDir@\\bin\\qtcreator.exe,6");
    component.addOperation( "RegisterFileType",
                            "qs",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt Script file",
                            "",
                            "@TargetDir@\\bin\\qtcreator.exe,0");
    component.addOperation( "RegisterFileType",
                            "qml",
                            "@TargetDir@\\bin\\qtcreator.exe -client '%1'",
                            "Qt Quick Markup language file",
                            "",
                            "@TargetDir@\\bin\\qtcreator.exe,0");
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
        component.addElevatedOperation("Execute", "{0,3010}", "@TargetDir@\\lib\\vcredist_msvc2010\\vcredist_x86.exe", "/q");
        registerWindowsFileTypeExtensions();
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

