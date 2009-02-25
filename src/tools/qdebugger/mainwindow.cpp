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

#include "mainwindow.h"

#include "gdbdebugger.h"
#include "gdboutputwindow.h"
#include "lean.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDockWidget>
#include <QtGui/QFileDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QStandardItemModel>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QTextBlock>
#include <QtGui/QToolBar>
#include <QtGui/QTreeView>
#include <QtGui/QWidget>

using namespace GdbDebugger;
using namespace GdbDebugger::Internal;


///////////////////////////////////////////////////////////////////////
//
// LocationMark
//
///////////////////////////////////////////////////////////////////////

#ifdef USE_BASETEXTEDITOR

class LocationMark : public TextEditor::ITextMark
{
public:
    LocationMark() { m_editor = 0; }
    ~LocationMark() { qDebug() << "LOCATIONMARK DESTRUCTOR" << m_editor; }

    QIcon icon() const;
    QColor color() const;

    void contextMenu(const QPoint &/*position*/) {}
    void toggle() {}

    void updateLineNumber(int /*line*/) {}
    void removedFromEditor() { m_editor = 0; }

    TextViewer *editor() const { return m_editor.data(); }
    void setEditor(TextViewer *editor);

    void updateBlock(const QTextBlock &) {}
    void documentClosing() {}

private:
    QPointer<TextViewer> m_editor;
};

void LocationMark::setEditor(TextViewer *editor)
{
    if (m_editor)
        m_editor->markableInterface()->removeMark(this);
    if (editor)
        m_editor = editor;
    else
        m_editor = 0;
}

QIcon LocationMark::icon() const
{
    static const QIcon icon(":/gdbdebugger/images/location.svg");
    return icon;
}

QColor LocationMark::color() const
{
    return QColor(255, 255, 0, 20);
}

LocationMark *theLocationMark()
{
    static LocationMark *mark = new LocationMark;
    return mark;
}

#endif

///////////////////////////////////////////////////////////////////////////
//
// MainWindow
//
///////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow()
{
    m_manager = new DebuggerManager;
    m_manager->initialize();
    m_manager->createDockWidgets();
    m_manager->setSimpleDockWidgetArrangement();
    setCentralWidget(m_manager->mainWindow());

    //
    //  Source code view
    //
    m_textViewers = new QTabWidget(m_manager->mainWindow());
    m_textViewers->setObjectName("Editors");
    m_textViewers->setElideMode(Qt::ElideLeft);
    //setCentralWidget(m_textViewers);
    m_manager->mainWindow()->setCentralWidget(m_textViewers);
    setGeometry(QRect(0, 0, 800, 600));

    //
    // Actions
    //
    m_fileOpenAction = new QAction(tr("Open file"), this);
    m_fileOpenAction->setShortcut(QKeySequence(tr("Ctrl+O")));

    m_quitAction = new QAction(tr("Quit"), this);
    m_quitAction->setShortcut(QKeySequence(tr("Ctrl+Q")));

#if 0
    m_startDebuggerAction = new QAction(tr("Run to main()"), this);
    m_startDebuggerAction->setShortcut(QKeySequence(tr("Ctrl+F5")));
    connect(m_startDebuggerAction, SIGNAL(triggered()),
        this, SLOT(startDebuggerRequest()));
    connect(m_resetAction, SIGNAL(triggered()),
        m_manager, SLOT(resetDebugger()));
    connect(m_watchAction, SIGNAL(triggered()),
        m_manager, SLOT(addToWatchWindow()));
    connect(m_breakAction, SIGNAL(triggered()),
        this, SLOT(toggleBreakpoint()));
#endif

    m_manager->m_continueAction->setShortcut(QKeySequence(tr("F5")));
    m_manager->m_stopAction->setShortcut(QKeySequence(tr("Shift+F5")));

    //m_resetAction = new QAction(tr("Reset Debugger"), this);
    //m_resetAction->setShortcut(QKeySequence(tr("Ctrl+Shift+F5")));

    m_manager->m_nextAction->setShortcut(QKeySequence(tr("F6")));
    m_manager->m_stepAction->setShortcut(QKeySequence(tr("F7")));
    m_manager->m_nextIAction->setShortcut(QKeySequence(tr("Shift+F6")));
    m_manager->m_stepIAction->setShortcut(QKeySequence(tr("Shift+F9")));
    m_manager->m_stepOutAction->setShortcut(QKeySequence(tr("Shift+F7")));
    m_manager->m_runToLineAction->setShortcut(QKeySequence(tr("Shift+F8")));
    //m_manager->m_jumpToLineAction->setShortcut(QKeySequence(tr("Shift+F8")));
    m_manager->m_breakAction->setShortcut(QKeySequence(tr("F8")));
    m_manager->m_watchAction->setShortcut(QKeySequence(tr("ALT+D,ALT+W")));

    //
    //  Files
    //   
    QDockWidget *filesDock = new QDockWidget(this);
    filesDock->setObjectName("FilesDock");
    filesDock->setGeometry(QRect(0, 0, 200, 200));
    filesDock->setWindowTitle(tr("Files"));
    filesDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, filesDock);

    m_filesModel = new QStandardItemModel(this);
    m_filesWindow = new QTreeView(this);
    m_filesWindow->setModel(m_filesModel);
    m_filesWindow->header()->hide();
    m_filesWindow->setRootIsDecorated(false);
    filesDock->setWidget(m_filesWindow);
    connect(m_filesWindow, SIGNAL(activated(QModelIndex)),
        this, SLOT(changeCurrentFile(QModelIndex)));
    connect(m_filesWindow, SIGNAL(clicked(QModelIndex)),
        this, SLOT(changeCurrentFile(QModelIndex)));

    //
    //  Menubar
    //
    QMenu *fileMenu = new QMenu(menuBar());
    fileMenu->setTitle(tr("File"));
    fileMenu->addAction(m_fileOpenAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_quitAction);
    menuBar()->addMenu(fileMenu);

    QMenu *debugMenu = new QMenu(menuBar());
    debugMenu->setTitle(tr("Debug"));
    debugMenu->addAction(m_manager->m_continueAction);
    debugMenu->addAction(m_manager->m_stopAction);
    debugMenu->addSeparator();
    debugMenu->addAction(m_manager->m_nextAction);
    debugMenu->addAction(m_manager->m_stepAction);
    debugMenu->addAction(m_manager->m_nextIAction);
    debugMenu->addAction(m_manager->m_stepIAction);
    debugMenu->addAction(m_manager->m_stepOutAction);
    debugMenu->addAction(m_manager->m_runToLineAction);
    debugMenu->addAction(m_manager->m_runToFunctionAction);
    debugMenu->addAction(m_manager->m_jumpToLineAction);
    debugMenu->addSeparator();
    debugMenu->addAction(m_manager->m_breakAction);
    //debugMenu->addAction(m_startDebuggerAction);
    menuBar()->addMenu(debugMenu);
    
    //
    //  Toolbar
    //
    QToolBar *toolbar = m_manager->createToolBar();
    toolbar->setObjectName("ToolBar");
    addToolBar(Qt::TopToolBarArea, toolbar);

    //
    //  Statusbar
    //
    QStatusBar *statusbar = new QStatusBar(this);
    statusbar->setObjectName("StatusBar");
    statusbar->setGeometry(QRect(0, 578, 800, 22));
    setStatusBar(statusbar);

    show();
    restoreState(settings().value("MainWindow/State").toByteArray());
    setGeometry(settings().value("MainWindow/Geometry").toRect());

    connect(m_fileOpenAction, SIGNAL(triggered()),
        this, SLOT(fileOpen()));
    connect(m_quitAction, SIGNAL(triggered()),
        this, SLOT(quit()));

    connect(m_manager, SIGNAL(resetLocationRequested()),
        this, SLOT(resetLocation()));
    connect(m_manager, SIGNAL(gotoLocationRequested(QString,int,bool)),
        this, SLOT(gotoLocation(QString,int,bool)));
    connect(m_manager, SIGNAL(dataDumpersUnavailable()),
        this, SLOT(handleDataDumpersUnavailable()));

    // Application interaction
    connect(m_manager, SIGNAL(currentTextEditorRequested(QString*,int*,QObject**)),
        this, SLOT(queryCurrentTextEditor(QString*,int*,QObject**)));
}

MainWindow::~MainWindow()
{
    settings().setValue("MainWindow/State", saveState());
    settings().setValue("MainWindow/Geometry", geometry());
    settings().sync();
}

QSettings &MainWindow::settings()
{
    static QSettings s("Nokia", "Qdb");
    return s;
}

void MainWindow::loadFile(const QString &fileName)
{
    QFileInfo fi(fileName);
    if (fi.isExecutable() && !fileName.endsWith(".cpp") && !fileName.endsWith(".h")) {
        m_executable = fileName;
        QStandardItem *item = new QStandardItem(fileName);
        QStandardItem *childItem = new QStandardItem("<gdb not running>");
        item->appendRow(childItem); // force [+] on item
        item->setToolTip(fi.absoluteFilePath());
        //m_fileModel->appendRow(item);
    } else {
        (void)findOrCreateTextViewer(fileName);
    }
}

void MainWindow::loadFiles(const QStringList &fileNames)
{
    if (fileNames.isEmpty())
        return;
    foreach (const QString &fileName, fileNames)
        loadFile(fileName);
    startDebuggingRequest(); 
}

void MainWindow::startDebuggingRequest()
{
    if (m_manager->m_startIsContinue) {
        m_manager->continueInferior();
        return;
    }

    m_manager->settings()->m_autoRun = true;
    m_manager->settings()->m_autoQuit = true;
    m_manager->settings()->m_gdbCmd = "gdb";
    m_manager->settings()->m_breakOnMain = "gdb";
    qDebug() << "START REQUEST";

    if (m_executable.isEmpty()) {
        QMessageBox::warning(0,
             tr("Not a runnable project"),
             tr("The current startup project can not be run."));
        return;
    }
    StartData sd;
    sd.outputFile = m_executable;
    //Project *pro = project();
    //sd.outputFile = pro->executable(pro->activeConfiguration());
    // NBS TODO pid
    //         pid = pe->configLanguage(
    //             ProjectExplorer::Constants::P_RUNNINGPID, langID).toString();
    // sd.env = pro->environment(pro->activeConfiguration()).toStringList();
    // sd.workingDir = pro->workingDirectory(pro->activeConfiguration());
    // sd.processArgs = pro->executableArguments(pro->activeConfiguration());
    // const QFileInfo editorFile(textViewer->file()->fileName());
    // sd.currentEditorDir = editorFile.dir().absolutePath();

    m_manager->startDebuggerAndRunInferior(sd);
}

void MainWindow::startDebuggerRequest()
{
    // FIXME: Ignored
    StartData sd;
    sd.outputFile = m_executable;
    //Project *pro = project();
    //sd.outputFile = pro->executable(pro->activeConfiguration());
    // NBS TODO pid
    //         pid = pe->configLanguage(
    //             ProjectExplorer::Constants::P_RUNNINGPID, langID).toString();
    // sd.env = pro->environment(pro->activeConfiguration()).toStringList();
    // sd.workingDir = pro->workingDirectory(pro->activeConfiguration());
    // sd.processArgs = pro->executableArguments(pro->activeConfiguration());
    // const QFileInfo editorFile(textViewer->file()->fileName());
    // sd.currentEditorDir = editorFile.dir().absolutePath();

    m_manager->m_settings->m_autoRun = false;
    m_manager->m_settings->m_autoQuit = true;
    m_manager->startDebuggerAndRunInferior(sd);
}

void MainWindow::jumpToExec()
{
#if 0
    TextViewer *editor = currentTextViewer();
    if (!editor)
        return;

    int lineNumber = editor->currentLine();
    QString fileName = editor->file()->fileName();

    if (fileName.isEmpty())
        return;

    m_manager->jumpToLineExec(fileName, lineNumber);
#endif
}

void MainWindow::runToExec()
{
#if 0
    TextViewer *editor = currentTextViewer();
    if (!editor)
        return;

    int lineNumber = editor->currentLine();
    QString fileName = editor->file()->fileName();

    if (fileName.isEmpty())
        return;

    m_manager->runToLineExec(fileName, lineNumber);
#endif
}

void MainWindow::showStatusMessage(const QString &msg, int timeout)
{
    statusBar()->showMessage(msg, timeout);
}

void MainWindow::resetLocation()
{
    TextViewer *textViewer = currentTextViewer();
    if (!textViewer)
        return;
    const int blockNumber = m_textBlockFromName.value(textViewer, -1);
    if (blockNumber == 1)
        return;
    const QTextBlock &block = textViewer->document()->findBlockByNumber(blockNumber);
    if (block.isValid()) {
        QTextCursor cursor(block);
        //QTextBlockFormat format = block.blockFormat();
        //format.setBackground(QColor(200, 200, 250));
        cursor.setBlockFormat(QTextBlockFormat());
        textViewer->setTextCursor(cursor);
        textViewer->centerCursor();
    }
#ifdef USE_BASETEXTEDITOR
    if (TextViewer *editor = theLocationMark()->editor())
        editor->markableInterface()->removeMark(theLocationMark());
    theLocationMark()->setEditor(0);
#endif
}

void MainWindow::gotoLocation(const QString &fileName, int line, bool setMarker)
{
    //qDebug() << "GOTO " << fileName << line << setMarker;
    Q_UNUSED(setMarker);
    TextViewer *textViewer = findOrCreateTextViewer(fileName);
    m_textViewers->setCurrentWidget(textViewer);
    
    const int blockNumber = line - 1;
    const QTextBlock &block = textViewer->document()->findBlockByNumber(blockNumber);
    if (block.isValid()) {
        QTextCursor cursor(block);
        QTextBlockFormat format = block.blockFormat();
        format.setBackground(QColor(230, 200, 200));
        cursor.setBlockFormat(format);
        textViewer->setTextCursor(cursor);
        textViewer->centerCursor();
        m_textBlockFromName[textViewer] = blockNumber;

#ifdef USE_BASETEXTEDITOR
        if (line >= 1) {
            if (setMarker) {
                theLocationMark()->setEditor(textViewer);
                textViewer->markableInterface()->addMark(theLocationMark(), line);
            }
            textViewer->gotoLine(line + 8);
            textViewer->gotoLine(line);
            // FIXME: editor->centerCursor();
        }
#endif
    } else {
        qDebug() << "INVALID BLOCK FOR LINE " << line << " IN " << fileName;
    }
}

void MainWindow::quit()
{
    m_manager->exitDebugger();
    close();
}

void MainWindow::fileOpen()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open File"),
        settings().value("FileOpen/LastDir").toString()
        // filterQString & filter = QString(),
        //QString * selectedFilter = 0, Options options = 0
    );
    settings().setValue("FileOpen/LastDir", QFileInfo(fileName).dir().dirName());
    if (fileName.isEmpty())
        return;
    loadFile(fileName);   
}

void MainWindow::queryCurrentTextEditor(QString *fileName, int *line, QObject **object)
{
    //Core::IEditor *editor = core->editorManager()->currentEditor();
    //*textEditor = qobject_cast<ITextEditor*>(current);
    TextViewer *textEditor = currentTextViewer();
    if (fileName)
        *fileName = textEditor->fileName();
    if (line)
        *line = textEditor->currentLine();
    if (object)
        *object = textEditor;
}


TextViewer *MainWindow::findOrCreateTextViewer(const QString &fileName)
{
    TextViewer *textViewer = m_textViewerFromName.value(fileName);
    if (textViewer)
        return textViewer;

    textViewer = new TextViewer(this);
    textViewer->open(fileName);
    textViewer->setReadOnly(true);
    m_textViewerFromName[fileName] = textViewer;
    m_textViewers->addTab(textViewer, fileName);
    m_textViewers->setCurrentWidget(textViewer);

    if (m_filesModel->findItems(fileName).isEmpty())
        m_filesModel->appendRow(new QStandardItem(fileName));
    //m_manager->textEditorOpened(textViewer->editableInterface());

    return textViewer;
}

TextViewer *MainWindow::currentTextViewer()
{
   return qobject_cast<TextViewer *>(m_textViewers->currentWidget());
}

void MainWindow::changeCurrentFile(const QModelIndex &idx)
{
    QString fileName = m_filesModel->data(idx).toString();
    m_textViewers->setCurrentWidget(findOrCreateTextViewer(fileName));
}

void MainWindow::handleDataDumpersUnavailable()
{
    QMessageBox::warning(this, tr("Cannot find special data dumpers"),
         tr("The debugged binary does not contain information needed for "
                "nice display of Qt data types.\n\n" 
                "Make sure you use something like\n\n"
                "SOURCES *= .../ide/main/bin/gdbmacros/gdbmacros.cpp\n\n"
                "in your .pro file.")
            );
}
