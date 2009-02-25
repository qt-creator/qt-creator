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

#include "texteditor/basetexteditor.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QFileDialog>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QTextBlock>
#include <QtGui/QToolBar>


class MyEditor;

class MyEditable : public TextEditor::BaseTextEditorEditable
{
public:
    MyEditable(MyEditor *editor);
    ~MyEditable() {}

    // Core::IContext
    const QList<int>& context() const { return m_context; }
    // Core::IEditor
    const char* kind() const { return "MYEDITOR"; }
    bool duplicateSupported() const { return false; }
    Core::IEditor* duplicate(QWidget*) { return 0; }

    MyEditor *m_editor;
    QList<int> m_context;
};


class MyEditor : public TextEditor::BaseTextEditor
{
public:
    MyEditor(QWidget *parent)
      : TextEditor::BaseTextEditor(parent, 0)
    {}

    TextEditor::BaseTextEditorEditable* createEditableInterface()
    {
        return new MyEditable(this);
    }
    
    void setPlainText(const QString &contents)
    {   
        qDebug() << "Setting contents to:\n" << contents;
        QPlainTextEdit::setPlainText(contents);
    }
};


MyEditable::MyEditable(MyEditor *editor)
  : TextEditor::BaseTextEditorEditable(editor), m_editor(editor)
{}



///////////////////////////////////////////////////////////////////////////
//
// MainWindow
//
///////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow()
{
    //
    //  Source code view
    //
    m_textViewers = new QTabWidget(this);
    m_textViewers->setObjectName("Editors");
    m_textViewers->setElideMode(Qt::ElideLeft);
    setCentralWidget(m_textViewers);
    setGeometry(QRect(0, 0, 800, 600));


    m_fileOpenAction = new QAction("Open", this);
    m_quitAction = new QAction("Quit", this);

    //
    //  Menubar
    //
    QMenu *fileMenu = new QMenu(menuBar());
    fileMenu->setTitle(tr("File"));
    fileMenu->addAction(m_fileOpenAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_quitAction);
    menuBar()->addMenu(fileMenu);

    //
    //  Toolbar
    //
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setObjectName("ToolBar");
    toolbar->addAction(m_fileOpenAction);
    toolbar->addAction(m_quitAction);
    addToolBar(Qt::TopToolBarArea, toolbar);

    //
    //  Statusbar
    //
    QStatusBar *statusbar = new QStatusBar(this);
    statusbar->setObjectName("StatusBar");
    setStatusBar(statusbar);

    show();
    restoreState(settings().value("MainWindow/State").toByteArray());
    setGeometry(settings().value("MainWindow/Geometry").toRect());

    //
    // Connections
    //
    connect(m_fileOpenAction, SIGNAL(triggered()),
        this, SLOT(fileOpen()));
    connect(m_quitAction, SIGNAL(triggered()),
        this, SLOT(close()));
}

MainWindow::~MainWindow()
{
    settings().setValue("MainWindow/State", saveState());
    settings().setValue("MainWindow/Geometry", geometry());
    settings().sync();
}

QSettings &MainWindow::settings()
{
    static QSettings s("Nokia", "TextEditor");
    return s;
}

void MainWindow::loadFile(const QString &fileName)
{
   findOrCreateTextViewer(fileName);
}

void MainWindow::loadFiles(const QStringList &fileNames)
{
    foreach (const QString &fileName, fileNames)
        loadFile(fileName);
}

void MainWindow::fileOpen()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Executable File"),
        settings().value("FileOpen/LastDir").toString()
        // filterQString & filter = QString(),
        //QString * selectedFilter = 0, Options options = 0
    );
    settings().setValue("FileOpen/LastDir", QFileInfo(fileName).dir().dirName());
    if (fileName.isEmpty())
        return;
    loadFile(fileName);   
}


Editor *MainWindow::findOrCreateTextViewer(const QString &fileName)
{
    Editor *textViewer = m_textViewerFromName.value(fileName);
    if (textViewer)
        return textViewer;

    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QString contents = file.readAll();

    textViewer = new MyEditor(this);
    textViewer->setPlainText(contents);
    m_textViewerFromName[fileName] = textViewer;
    m_textViewers->addTab(textViewer, fileName);
    return textViewer;
}

