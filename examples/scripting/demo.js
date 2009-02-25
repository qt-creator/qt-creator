/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

//  This script file demos the scripting features
//  of Qt Creator.
//  Choose "Run" from the context menu.

function introspect(o, indent)
{
    core.messageManager.printToOutputPane(indent + "+++++++++++++ Class " + o);
    for (i in o)  {
        var member = o[i];
        var t = typeof member;
       core.messageManager.printToOutputPane(indent + typeof o[i] + " " + i);
       if (t == "object")
         introspect(i,  indent + "  ");
   }

}

function introspectToString(o)
{
    var rc = "";
    for (i in o)  {
        rc = rc + " " + typeof o[i] + " " + i;
   }
   return rc;
}

function demoInputDialogs()
{
    var t = getText(core.mainWindow, "Input dialogs", "text", "default");
    if (t == null)
        return;

    core.messageManager.printToOutputPane("Input :" +t);
    var i = getInteger(core.mainWindow, "Input dialogs", "integer", 42, 0, 1000);
    if (i == null)
        return;

    core.messageManager.printToOutputPane("Input :" +i);
    var d = getDouble(core.mainWindow, "Input dialogs", "double", 42.0, 0.0, 1000.0);
    if (d == null)
        return;
    core.messageManager.printToOutputPane("Input :" +d);
}

function demoFileDialogs()
{
    var f = getOpenFileName(core.mainWindow, "Choose file", "", "All files (*.*)");
    if (f == null)
        return;

    core.messageManager.printToOutputPane("File:" + f);

    f = getOpenFileNames(core.mainWindow, "Choose files", "", "All files (*.*)");

    if (f == null)
        return;

    core.messageManager.printToOutputPane("Files:" + f);

    f = getSaveFileName(core.mainWindow, "Choose file to write to", "", "All files (*.*)");

    if (f == null)
        return;

    core.messageManager.printToOutputPane("File:" + f);

    f = getExistingDirectory(core.mainWindow, "Choose directory", "");

    if (f == null)
        return;

    core.messageManager.printToOutputPane("Directory:" + f);
}


function demoMessageBoxes()
{
    critical(core.mainWindow, "Critical", "critical");
    warning(core.mainWindow, "warning", "warning");
    information(core.mainWindow, "information", "information");
    var a = yesNoQuestion(core.mainWindow, "Question", "question");
    core.messageManager.printToOutputPane("Answer:" +a);
}

function demoWizard()
{
    var filters = new Array("ProjectExplorer.WizardType.Project", "QtCreator::WizardType::File");
    core.showNewItemDialog(filters);
}

function demoWidgets()
{
    core.mainWindow.windowTitle = "Accessing MainWindow";
    core.statusBar.showMessage("Accessing StatusBar", 0);
}

function demoIntrospect()
{
    // Not perfect yet
    introspect(core, "");
}

function demoFileManager()
{
    core.messageManager.printToOutputPane("Recent files:" + core.fileManager.recentFiles);
    var name = getText(core.mainWindow, "Input file name", "name", "");

    if (core.fileManager.isFileManaged(name) == 0) {
        core.messageManager.printToOutputPane(name + " is not managed.");
        return;
    }

    var mf = core.fileManager.managedFiles(name);
    var s = mf.length;
    core.messageManager.printToOutputPane(s + " managed files match " + name);
    for (var i = 0; i < mf.length; i++) {
        core.messageManager.printToOutputPane(mf[i].fileName);
    }
}

function printEditor(e, indent)
{
    var m = indent + "Editor " + e.displayName + ", " + e.kind ;
    var f = e.file;
    m = m + " (" + f.fileName + ")";
    core.messageManager.printToOutputPane(m);
}

function printEditorList(header, l, indent)
{
    core.messageManager.printToOutputPane(header + " (" + l.length + ")");
    for (var i = 0; i < l.length; i++) {
        printEditor(l[i],"  ");
    }
}

function printEditorGroup(g)
{
    var m = "Editor Group: " + g.editorCount + " editor(s)";
    core.messageManager.printToOutputPane(m);
    printEditorList("Editors of the group", g.editors);
    var ce = g.currentEditor;
    if (ce == null) {
        core.messageManager.printToOutputPane("No current editor in group");
    } else {
        printEditor(ce, "  ");
    }
}

function demoEditorManager()
{
    var og = core.editorManager.editorGroups;
    core.messageManager.printToOutputPane("Editor groups " + og.length);
    for (var i = 0; i < og.length; i++) {
        printEditorGroup(og[i]);
    }

    printEditorList("Opened editors", core.editorManager.openedEditors);

    var ce = core.editorManager.currentEditor;
    if (ce == null) {
        core.messageManager.printToOutputPane("No current editor");
        return;
    }

    core.messageManager.printToOutputPane("Current editor");
    printEditor(ce, "");

    var f = getOpenFileName(core.mainWindow, "Choose file to open", "", "All files (*.*)");
    if (f  == null)
        return;

    printEditor(core.editorManager.openEditor(f, ""), "");
//    printEditor(core.editorManager.newFile("Text", "title", "contents"));
//    var dup = ce.duplicate(core.mainWindow);
}

function demoDebugger()
{
    var state = gdbdebugger.status;
    core.messageManager.printToOutputPane("State " + state);
    // TODO: Start debugger on demand?
    if (state != 0)
        gdbdebugger.sendCommand("help");
}

// -- ProjectExplorer
function printProjectItem(pi, indent, recursively)
{
    var m = indent + "ProjectItem " + pi.kind + " " + pi.name;
    core.messageManager.printToOutputPane(m);
    if (recursively != 0) {
        var rIndent = indent + "    ";
        var c =  projectExplorer.childrenOf(pi);
        for (var i = 0; i < c.length; i++) {
            printProjectItem(c[i], rIndent, 1);
        }
    }
}

function printSession(s, indent)
{
    core.messageManager.printToOutputPane(indent + "Session " + s.name + " startup project " + s.startupProject);
    var p = s.projects;
    var pIndent = indent + "    ";
    for (var i = 0; i < p.length; i++) {
        printProjectItem(p[i], pIndent, 1);
    }
}

function demoProjectExplorer()
{
    core.messageManager.printToOutputPane("ProjectExplorer");
    projectExplorer.buildManager.showOutputWindow(1);
    projectExplorer.buildManager.addMessage("Build manager message");
    projectExplorer.applicationOutputWindow.clear();
    projectExplorer.applicationOutputWindow.appendOutput("Hi, there! .. This the projectExplorer demo");

    var ci = projectExplorer.currentItem;
    if (ci != null) {
        core.messageManager.printToOutputPane("Current Item");
        printProjectItem(ci, "    ", 0);
    } else {
        core.messageManager.printToOutputPane("No current Item");
    }
    var cp = projectExplorer.currentProject;
    if (cp != null) {
        core.messageManager.printToOutputPane("Current Project");
        printProjectItem(cp, "    ", 0);
    } else {
        core.messageManager.printToOutputPane("No current Project");
    }

    var cs = projectExplorer.session;
    if (cs != null) {
        core.messageManager.printToOutputPane("Current Session");
        printSession(cs, "    ");
        // Ask to build
        var p = projectExplorer.needsBuild(cs.projects[0]);
        for (var i = 0; i < p.length; i++) {
            if (yesNoQuestion(core.mainWindow, "Rebuild", "Do you want to rebuild " + p[i].name + "?") != 65536) {
                if (p[i].supportsProjectCommand(2)) {
                    p[i].executeProjectCommand(2);
                } else {
                    core.messageManager.printToOutputPane("Build not supported.");
                }
            }
        }
    } else {
        core.messageManager.printToOutputPane("No current Session");
        var a = yesNoQuestion(core.mainWindow, "Open Session", "Do you want to open a session?");
        if (a != 65536) {
            var f = getOpenFileNames(core.mainWindow, "Choose a session", "", "All projects (*.qws *.pro)");
            if (f == null)
                return;
            var o = projectExplorer.openProject(f);
            return;
        }
    }
    if (yesNoQuestion(core.mainWindow, "Build manager", "Do you want run a command using build mananger?") !=  65536) {
        var cmd = new BuildManagerCommand("ls", "-l");
        var cmds =new Array(cmd);
        core.messageManager.printToOutputPane("Let build mananger run a command " + cmds + "  (see compile window)");
        projectExplorer.buildManager.start(cmds);
    }
}

// --------------- MAIN

var features = new Array("Input dialogs",
                         "File dialogs",
                         "Messages",
                         "Project Explorer",
                         "Message Manager",
                         "Wizard",
                         "Editor manager",
                         "File manager",
                         "Introspect",
                         "Widgets magic",
                         "Debugger");

core.messageManager.printToOutputPane(" +++ demo.js " + Date());

while (1) {
    var f = getItem(core.mainWindow, "Choose a demo",  "Available demos", features, 0);
    if (f == null)
       return;

    while (1) {
        if (f == features[0]) {
            demoInputDialogs();
            break;
        }

        if (f == features[1]) {
            demoFileDialogs();
            break;
        }

        if (f == features[2]) {
            demoMessageBoxes();
            break;
        }

        if (f == features[3]) {
            demoProjectExplorer();
            break;
        }

        if (f == features[4]) {
            core.messageManager.printToOutputPane("Hi there!",1);
            break;
        }

        if (f == features[5]) {
            demoWizard();
            break;
        }

        if (f == features[6]) {
            demoEditorManager();
            break;
        }

        if (f == features[7]) {
            demoFileManager();
            break;
        }

        if (f == features[8]) {
            demoIntrospect();
            break;
        }

        if (f == features[9]) {
            demoWidgets();
            break;
        }

        if (f == features[10]) {
            demoDebugger();
            break;
        }
        break;
    }
}
