/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt SDK.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

// constructor
function Component()
{
    component.loaded.connect(this, Component.prototype.loaded);
    installer.installationFinished.connect(this, Component.prototype.installationFinishedPageIsShown);
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
    component.qtCreatorBinaryPath = installer.value("TargetDir");

    if (installer.value("os") == "win") {
        component.qtCreatorBinaryPath = component.qtCreatorBinaryPath + "\\bin\\qtcreator.exe";
        component.qtCreatorBinaryPath = component.qtCreatorBinaryPath.replace(/\//g, "\\");
    }
    else if (installer.value("os") == "x11")
        component.qtCreatorBinaryPath = component.qtCreatorBinaryPath + "/bin/qtcreator";
    else if (installer.value("os") == "mac")
        component.qtCreatorBinaryPath = component.qtCreatorBinaryPath + "/Qt Creator.app/Contents/MacOS/Qt Creator";

    if ( installer.value("os") === "win" ) {
        component.setStopProcessForUpdateRequest(component.qtCreatorBinaryPath, true);
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
                                component.qtCreatorBinaryPath + " -client \"%1\"",
                                "C++ Header file",
                                "text/plain",
                                component.qtCreatorBinaryPath + ",3",
                                "ProgId=QtProject.QtCreator." + headerExtensions[i]);
    }

    var cppExtensions = new Array("cc", "cxx", "c++", "cp", "cpp");

    for (var i = 0; i < cppExtensions.length; ++i) {
        component.addOperation( "RegisterFileType",
                                cppExtensions[i],
                                component.qtCreatorBinaryPath + " -client \"%1\"",
                                "C++ Source file",
                                "text/plain",
                                component.qtCreatorBinaryPath + ",2",
                                "ProgId=QtProject.QtCreator." + cppExtensions[i]);
    }

    component.addOperation( "RegisterFileType",
                            "c",
                            component.qtCreatorBinaryPath + " -client \"%1\"",
                            "C Source file",
                            "text/plain",
                            component.qtCreatorBinaryPath + ",1",
                            "ProgId=QtProject.QtCreator.c");
}

registerWindowsFileTypeExtensions = function()
{
    component.addOperation( "RegisterFileType",
                            "ui",
                            component.qtCreatorBinaryPath + " -client \"%1\"",
                            "Qt UI file",
                            "text/plain",
                            component.qtCreatorBinaryPath + ",4",
                            "ProgId=QtProject.QtCreator.ui");
    component.addOperation( "RegisterFileType",
                            "pro",
                            component.qtCreatorBinaryPath + " -client \"%1\"",
                            "Qt Project file",
                            "text/plain",
                            component.qtCreatorBinaryPath + ",5",
                            "ProgId=QtProject.QtCreator.pro");
    component.addOperation( "RegisterFileType",
                            "pri",
                            component.qtCreatorBinaryPath + " -client \"%1\"",
                            "Qt Project Include file",
                            "text/plain",
                            component.qtCreatorBinaryPath + ",6",
                            "ProgId=QtProject.QtCreator.pri");
    component.addOperation( "RegisterFileType",
                            "qbs",
                            component.qtCreatorBinaryPath + " -client \"%1\"",
                            "Qbs Project file",
                            "text/plain",
                            component.qtCreatorBinaryPath + ",5",
                            "ProgId=QtProject.QtCreator.qbs");
    component.addOperation( "RegisterFileType",
                            "qs",
                            component.qtCreatorBinaryPath + " -client \"%1\"",
                            "Qt Script file",
                            "text/plain",
                            component.qtCreatorBinaryPath + ",0",
                            "ProgId=QtProject.QtCreator.qs");
    component.addOperation( "RegisterFileType",
                            "qml",
                            component.qtCreatorBinaryPath + " -client \"%1\"",
                            "Qt Quick Markup language file",
                            "text/plain",
                            component.qtCreatorBinaryPath + ",7",
                            "ProgId=QtProject.QtCreator.qml");
}

Component.prototype.createOperations = function()
{
    // Call the base createOperations and afterwards set some registry settings
    component.createOperations();
    if ( installer.value("os") == "win" )
    {
        component.addOperation( "CreateShortcut",
                                component.qtCreatorBinaryPath,
                                "@StartMenuDir@/Qt Creator.lnk",
                                "workingDirectory=@homeDir@" );

        // only install c runtime if it is needed, no minor version check of the c runtime till we need it
        if (installer.value("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\12.0\\VC\\Runtimes\\x86\\Installed") != 1) {
           // return value 3010 means it need a reboot, but in most cases it is not needed for run Qt application
           // return value 5100 means there's a newer version of the runtime already installed
           component.addElevatedOperation("Execute", "{0,1638,3010,5100}", "@TargetDir@\\lib\\vcredist_msvc2013\\vcredist_x86.exe", "/norestart", "/q");
        }

        registerWindowsFileTypeExtensions();

        if (component.userInterface("AssociateCommonFiletypesForm").AssociateCommonFiletypesCheckBox
            .checked) {
                registerCommonWindowsFileTypeExtensions();
        }
    }
    if ( installer.value("os") == "x11" )
    {
        component.addOperation( "InstallIcons", "@TargetDir@/share/icons" );
        component.addOperation( "CreateDesktopEntry",
                                "QtProject-qtcreator.desktop",
                                "Type=Application\nExec=" + component.qtCreatorBinaryPath + "\nPath=@TargetDir@\nName=Qt Creator\nGenericName=The IDE of choice for Qt development.\nGenericName[de]=Die IDE der Wahl zur Qt Entwicklung\nIcon=QtProject-qtcreator\nTerminal=false\nCategories=Development;IDE;Qt;\nMimeType=text/x-c++src;text/x-c++hdr;text/x-xsrc;application/x-designer;application/vnd.nokia.qt.qmakeprofile;application/vnd.nokia.xml.qt.resource;text/x-qml;text/x-qt.qml;text/x-qt.qbs;"
                                );
    }
}

function isRoot()
{
    if (installer.value("os") == "x11" || installer.value("os") == "mac")
    {
        var id = installer.execute("/usr/bin/id", new Array("-u"))[0];
        id = id.replace(/(\r\n|\n|\r)/gm,"");
        if (id === "0")
        {
            return true;
        }
    }
    return false;
}

Component.prototype.installationFinishedPageIsShown = function()
{
    isroot = isRoot();
    try {
        if (component.installed && installer.isInstaller() && installer.status == QInstaller.Success && !isroot) {
            installer.addWizardPageItem( component, "LaunchQtCreatorCheckBoxForm", QInstaller.InstallationFinished );
        }
    } catch(e) {
        print(e);
    }
}

Component.prototype.installationFinished = function()
{
    try {
        if (component.installed && installer.isInstaller() && installer.status == QInstaller.Success && !isroot) {
            var isLaunchQtCreatorCheckBoxChecked = component.userInterface("LaunchQtCreatorCheckBoxForm").launchQtCreatorCheckBox.checked;
            if (isLaunchQtCreatorCheckBoxChecked)
                installer.executeDetached(component.qtCreatorBinaryPath, new Array(), "@homeDir@");
        }
    } catch(e) {
        print(e);
    }
}

