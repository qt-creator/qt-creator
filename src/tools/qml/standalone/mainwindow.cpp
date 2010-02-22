/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtCore/QtDebug>
#include <QtCore/QBuffer>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QBoxLayout>
#include <QtGui/QCloseEvent>
#include <QtGui/QErrorMessage>
#include <QtGui/QFrame>
#include <QtGui/QMessageBox>
#include <QtGui/QSplitter>
#include <QtGui/QStackedWidget>
#include <QtGui/QStatusBar>
#include <QFile>

#include "aboutdialog.h"
#include "allpropertiesviewcontroller.h"
#include "designdocumentcontroller.h"
#include "multipledocumentscontroller.h"
#include "mainwindow.h"
#include "stylemanager.h"
#include "widgetboxcontroller.h"
#include "navigatorcontroller.h"
#include "stateseditorcontroller.h"
#include "xuifiledialog.h"
#include "welcomescreen.h"

using namespace QmlDesigner;

class StyleAction: public QAction
{
public:
    StyleAction(const QString& text, const QString& styleName, QObject* parent): QAction(text, parent), m_styleName(styleName) {}
    StyleAction(const QString& text, QObject* parent): QAction(text, parent), m_styleName(QString()) {}

    QString styleName() const { return m_styleName; }

private:
    QString m_styleName;
};

MainWindow::MainWindow(QWidget* parent):
        QMainWindow(parent),
        m_shutdown(false),
        m_lastPath(QString()),
        m_documentCount(0),
        m_menuBar(new QMenuBar(this)),
        m_itemLibraryController(new ItemLibraryController(this)),
        m_navigatorController(new NavigatorController(this)),
        m_statesEditorController(new StatesEditorController(this)),
        m_propertiesViewController(new AllPropertiesViewController(this)),
        m_multipleDocumentsController(new MultipleDocumentsController(this)),
        m_recentFilesMenu(0),
        m_mainArea(new QStackedWidget(this)),
        m_welcomeScreen(new WelcomeScreen(this))
{
     //    setWindowFlags(Qt::MacWindowToolBarButtonHint);
//    setUnifiedTitleAndToolBarOnMac(true);
    setObjectName(QLatin1String("mainWindow"));
    setWindowTitle(tr("Bauhaus", "MainWindowClass"));
    resize(1046, 700);

    QFile file(":/qmldesigner/stylesheet.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    setMenuBar(m_menuBar);

    createMenus();
    createMainArea();
    createStatusBar();

    updateActions();
    updateMainArea();

    connect(m_multipleDocumentsController, SIGNAL(documentCountChanged(unsigned)), this, SLOT(documentCountChanged(unsigned)));
    connect(m_welcomeScreen, SIGNAL(newFile(QString)), this, SLOT(newFile(QString)));
    connect(m_welcomeScreen, SIGNAL(openFile(QString)), this, SLOT(openFile(QString)));
}

MainWindow::~MainWindow()
{
    m_documentActions.clear();
}

int MainWindow::documentCount() const
{
    return m_documentCount;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_shutdown = true;
    m_multipleDocumentsController->closeAll(true);
    event->setAccepted(false);
}

void MainWindow::doQuit()
{
    m_shutdown = true;
    m_multipleDocumentsController->closeAll(true);
}

void MainWindow::createMenus()
{
    // File menu:
    QMenu* fileMenu = new QMenu(tr("&File"), m_menuBar);
    m_menuBar->addMenu(fileMenu);

    QAction* newAction = new QAction(tr("&New..."), fileMenu);
    newAction->setShortcut(QKeySequence(tr("Ctrl+N")));
    connect(newAction, SIGNAL(triggered()), this, SLOT(doNew()));
    fileMenu->addAction(newAction);

    QAction* openAction = new QAction(tr("&Open..."), fileMenu);
    openAction->setShortcut(QKeySequence(tr("Ctrl+O")));
    connect(openAction, SIGNAL(triggered()), this, SLOT(doOpen()));
    fileMenu->addAction(openAction);

    m_recentFilesMenu = new QMenu(tr("Recent Files"), fileMenu);
    connect(m_recentFilesMenu, SIGNAL(aboutToShow()), this, SLOT(showRecentFilesMenu()));
    fileMenu->addMenu(m_recentFilesMenu);

    fileMenu->addSeparator();

    QAction* saveAction = new QAction(tr("&Save"), fileMenu);
    saveAction->setShortcut(QKeySequence(tr("Ctrl+S")));
    connect(saveAction, SIGNAL(triggered()), m_multipleDocumentsController, SLOT(doSave()));
    fileMenu->addAction(saveAction);
    m_documentActions.append(saveAction);

    QAction* saveAsAction = new QAction(tr("Save &As..."), fileMenu);
    connect(saveAsAction, SIGNAL(triggered()), m_multipleDocumentsController, SLOT(doSaveAs()));
    fileMenu->addAction(saveAsAction);
    m_documentActions.append(saveAsAction);

    fileMenu->addSeparator();

    m_previewAction = new QAction(tr("&Preview"), fileMenu);
    m_previewAction->setShortcut(QKeySequence(tr("Ctrl+R")));
//    m_previewAction->setIcon(QIcon(":/preview.png"));
    m_previewAction->setCheckable(true);
    connect(m_previewAction, SIGNAL(toggled(bool)), m_multipleDocumentsController, SLOT(doPreview(bool)));
    connect(m_multipleDocumentsController, SIGNAL(previewVisibilityChanged(bool)), m_previewAction, SLOT(setChecked(bool)));
    fileMenu->addAction(m_previewAction);
    m_documentActions.append(m_previewAction);

    m_previewWithDebugAction = new QAction(tr("&Preview with Debug"), fileMenu);
    m_previewWithDebugAction->setShortcut(QKeySequence(tr("Ctrl+D")));
    m_previewWithDebugAction->setCheckable(true);
    connect(m_previewWithDebugAction, SIGNAL(toggled(bool)), m_multipleDocumentsController, SLOT(doPreviewWithDebug(bool)));
    connect(m_multipleDocumentsController, SIGNAL(previewWithDebugVisibilityChanged(bool)), m_previewWithDebugAction, SLOT(setChecked(bool)));
    fileMenu->addAction(m_previewWithDebugAction);
    m_documentActions.append(m_previewWithDebugAction);

#ifndef Q_WS_MAC
    fileMenu->addSeparator();
#endif // !Q_WS_MAC

    QAction* quitAction = new QAction(tr("&Quit"), fileMenu);
    quitAction->setShortcut(QKeySequence(tr("Ctrl+Q")));
    fileMenu->addAction(quitAction);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(doQuit()));

    // Edit menu:
    QMenu* editMenu = new QMenu(tr("&Edit"), m_menuBar);
    m_menuBar->addMenu(editMenu);

    QAction* undoAction = m_multipleDocumentsController->undoAction();
    undoAction->setParent(editMenu);
    undoAction->setShortcut(tr("Ctrl+Z"));
    editMenu->addAction(undoAction);

    QAction* redoAction = m_multipleDocumentsController->redoAction();
    redoAction->setParent(editMenu);
#ifdef Q_WS_WIN
    redoAction->setShortcut(tr("Ctrl+Y"));
#else // !Q_WS_WIN
    redoAction->setShortcut(tr("Ctrl+Shift+Z"));
#endif // Q_WS_WIN
    editMenu->addAction(redoAction);

    editMenu->addSeparator();

    QAction *copyAction = new QAction(tr("&Copy"), editMenu);
    connect(copyAction, SIGNAL(triggered()), m_multipleDocumentsController, SLOT(doCopy()));
    copyAction->setShortcuts(QList<QKeySequence>() << QKeySequence(QKeySequence::Copy));
    editMenu->addAction(copyAction);
    m_documentActions.append(copyAction);

    QAction *cutAction = new QAction(tr("&Cut"), editMenu);
    connect(cutAction, SIGNAL(triggered()), m_multipleDocumentsController, SLOT(doCut()));
    cutAction->setShortcuts(QList<QKeySequence>() << QKeySequence(QKeySequence::Cut));
    editMenu->addAction(cutAction);
    m_documentActions.append(cutAction);

    QAction *pasteAction = new QAction(tr("&Paste"), editMenu);
    connect(pasteAction, SIGNAL(triggered()), m_multipleDocumentsController, SLOT(doPaste()));
    pasteAction->setShortcuts(QList<QKeySequence>() << QKeySequence(QKeySequence::Paste));
    editMenu->addAction(pasteAction);
    m_documentActions.append(pasteAction);

    QAction *deleteAction = new QAction(tr("&Delete"), editMenu);
    connect(deleteAction, SIGNAL(triggered()), m_multipleDocumentsController, SLOT(doDelete()));
    deleteAction->setShortcuts(QList<QKeySequence>() << QKeySequence(tr("Del")) << QKeySequence(tr("Backspace")));
    editMenu->addAction(deleteAction);
    m_documentActions.append(deleteAction);

    // View menu:
    QMenu* viewMenu = new QMenu(tr("&View"), m_menuBar);
    m_menuBar->addMenu(viewMenu);
//
//    m_showNewLookPropertiesAction = viewMenu->addAction(tr("&New Look Properties"));
//    m_showNewLookPropertiesAction->setCheckable(true);
//    m_showNewLookPropertiesAction->setEnabled(false);
//    connect(m_showNewLookPropertiesAction, SIGNAL(triggered()), m_propertiesViewController, SLOT(showNewLookProperties()));
//    m_documentActions.append(m_showNewLookPropertiesAction);
//
//    m_showTraditionalPropertiesAction = viewMenu->addAction(tr("&Traditional Properties"));
//    m_showTraditionalPropertiesAction->setCheckable(true);
//    m_showTraditionalPropertiesAction->setEnabled(false);
//    connect(m_showTraditionalPropertiesAction, SIGNAL(triggered()), m_propertiesViewController, SLOT(showTraditionalProperties()));
//    m_documentActions.append(m_showTraditionalPropertiesAction);
//
//    QActionGroup* propertiesLookGroup = new QActionGroup(this);
//    propertiesLookGroup->addAction(m_showNewLookPropertiesAction);
//    propertiesLookGroup->addAction(m_showTraditionalPropertiesAction);
//    m_showNewLookPropertiesAction->setChecked(true);

//    viewMenu->addSeparator();

#ifdef ENABLE_TEXT_VIEW
    QAction* showFormAction = m_multipleDocumentsController->showFormAction(viewMenu);
    viewMenu->addAction(showFormAction);
    QAction* showTextAction = m_multipleDocumentsController->showTextAction(viewMenu);
    viewMenu->addAction(showTextAction);

    QActionGroup* formTextGroup = new QActionGroup(this);
    formTextGroup->addAction(showFormAction);
    formTextGroup->addAction(showTextAction);
    showFormAction->setChecked(true);

    viewMenu->addSeparator();
#endif // ENABLE_TEXT_VIEW

    // Style selection:
//    QMenu* styleMenu = viewMenu->addMenu("&Style");
//    QActionGroup* styleGroup = new QActionGroup(this);
//
//    StyleAction* defaultStyleAction = new StyleAction("&Default", viewMenu);
//    styleMenu->addAction(defaultStyleAction);
//    defaultStyleAction->setCheckable(true);
//    styleGroup->addAction(defaultStyleAction);
//    connect(defaultStyleAction, SIGNAL(triggered()), this, SLOT(styleActionTriggered()));
//
//    styleMenu->addSeparator();
//
//    foreach (const QString styleName, StyleManager::styles()) {
//        StyleAction* styleAction = new StyleAction(styleName, styleName, viewMenu);
//        styleMenu->addAction(styleAction);
//        styleAction->setCheckable(true);
//        styleGroup->addAction(styleAction);
//        connect(styleAction, SIGNAL(triggered()), this, SLOT(styleActionTriggered()));
//    }
//
//    defaultStyleAction->setChecked(true);

    // Help menu:
    QMenu* helpMenu = new QMenu(tr("&Help"), this);
    helpMenu->setObjectName(QString::fromAscii("helpMenu"));
    m_menuBar->addMenu(helpMenu);

    QAction* aboutAppAction = new QAction(tr("&About..."), this);
    aboutAppAction->setObjectName(QString::fromAscii("aboutAppAction"));
    connect(aboutAppAction, SIGNAL(triggered()), this, SLOT(doAbout()));
    helpMenu->addAction(aboutAppAction);
}

void MainWindow::showRecentFilesMenu()
{
    m_recentFilesMenu->clear();
    foreach (const QString &path, recentFiles()) {
        QAction *action = m_recentFilesMenu->addAction(path);
        action->setData(path);
        connect(action, SIGNAL(triggered()), this, SLOT(doOpen()));
    }
}

void MainWindow::styleActionTriggered()
{
    //### remove setStyle()
//    StyleAction* source = dynamic_cast<StyleAction*>(sender());
//
//    if (source) {
//        QString styleName = source->styleName();
//
//        if (styleName.isNull()) {
//        } else {
//            StyleManager::setStyle(styleName);
//        }
//    }
}

void MainWindow::createMainArea()
{
    QSplitter* mainSplitter = new QSplitter(this);
    mainSplitter->setObjectName("mainSplitter");

    // Left area:
    QSplitter* leftSplitter = new QSplitter(mainSplitter);
    leftSplitter->setObjectName("leftSplitter");
    leftSplitter->setOrientation(Qt::Vertical);
    {
        QSizePolicy policy = leftSplitter->sizePolicy();
        policy.setHorizontalStretch(0);
        policy.setHorizontalPolicy(QSizePolicy::Preferred);
        leftSplitter->setSizePolicy(policy);
    }

    leftSplitter->addWidget(m_navigatorController->widget());

    QTabWidget *leftBottomTabWidget = new QTabWidget(this);
    leftBottomTabWidget->addTab(m_itemLibraryController->widget(), "Library");
    leftBottomTabWidget->addTab(m_propertiesViewController->widget(), tr("Properties"));
    leftSplitter->addWidget(leftBottomTabWidget);

    // right area:
    QSplitter *rightSplitter = new QSplitter(mainSplitter);
    rightSplitter->setObjectName("rightSplitter");
    rightSplitter->setOrientation(Qt::Vertical);

    rightSplitter->addWidget(m_statesEditorController->widget());
    rightSplitter->addWidget(m_multipleDocumentsController->tabWidget());
    {
        QSizePolicy policy = m_multipleDocumentsController->tabWidget()->sizePolicy();
        policy.setHorizontalStretch(1);
        policy.setHorizontalPolicy(QSizePolicy::Expanding);
        m_multipleDocumentsController->tabWidget()->setSizePolicy(policy);
    }

    // Finishing touches:
    mainSplitter->setSizes(QList<int>() << 240 << 530);
    rightSplitter->setSizes(QList<int>() << 100 << 400);
    leftSplitter->setSizes(QList<int>() << 300 << 300);

    // Wire everything together:
    connect(m_multipleDocumentsController, SIGNAL(activeDocumentChanged(DesignDocumentController*)),
            m_itemLibraryController, SLOT(show(DesignDocumentController*)));
    connect(m_multipleDocumentsController, SIGNAL(documentClosed(DesignDocumentController*)),
            m_itemLibraryController, SLOT(close(DesignDocumentController*)));

    connect(m_multipleDocumentsController, SIGNAL(activeDocumentChanged(DesignDocumentController*)),
            m_navigatorController, SLOT(show(DesignDocumentController*)));
    connect(m_multipleDocumentsController, SIGNAL(documentClosed(DesignDocumentController*)),
            m_navigatorController, SLOT(close(DesignDocumentController*)));

    connect(m_multipleDocumentsController, SIGNAL(activeDocumentChanged(DesignDocumentController*)),
            m_statesEditorController, SLOT(show(DesignDocumentController*)));
    connect(m_multipleDocumentsController, SIGNAL(documentClosed(DesignDocumentController*)),
            m_statesEditorController, SLOT(close(DesignDocumentController*)));

    connect(m_multipleDocumentsController, SIGNAL(activeDocumentChanged(DesignDocumentController*)),
            m_propertiesViewController, SLOT(show(DesignDocumentController*)));
    connect(m_multipleDocumentsController, SIGNAL(documentClosed(DesignDocumentController*)),
            m_propertiesViewController, SLOT(close(DesignDocumentController*)));

    m_mainArea->addWidget(m_welcomeScreen);
    m_mainArea->addWidget(mainSplitter);
    setCentralWidget(m_mainArea);
}

void MainWindow::createStatusBar()
{
//    statusBar();
}

void MainWindow::doNew()
{
//    QWizard wizard;
//    Internal::FormTemplateWizardPage page;
//    wizard.addPage(&page);
//    wizard.setWindowTitle("New Component Wizard");
//    if (wizard.exec() != QDialog::Accepted)
//        return;

    QFile file(":/qmldesigner/templates/General/Empty Fx");
    file.open(QFile::ReadOnly);
    newFile(file.readAll());
//    newFile(page.templateContents().toLatin1());
}

void MainWindow::doOpen()
{
    if (QAction *senderAction = qobject_cast<QAction*>(sender())) {
        if (senderAction->data().isValid()) {
            // from recent files menu
            QString path = senderAction->data().toString();
            openFile(path);
            return;
        }
    }
    XUIFileDialog::runOpenFileDialog(m_lastPath, this, this, SLOT(openFile(QString)));
}

void MainWindow::openFile(const QString &fileName)
{
//    qDebug() << "openFile("+fileName+")";
//
    if (fileName.isNull())
        return;

    m_lastPath = QFileInfo(fileName).path();

    QString errorMessage;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorMessage = tr("Could not open file <%1>").arg(fileName);
    } else {
        DesignDocumentController* controller = new DesignDocumentController(m_multipleDocumentsController);
        controller->setFileName(fileName);
        QList<QmlDesigner::RewriterView::Error> errors = controller->loadMaster(file.readAll());
        if (errors.isEmpty()) {
            connect(controller, SIGNAL(fileToOpen(QString)), this, SLOT(openFile(QString)));
            m_multipleDocumentsController->open(controller);
            addRecentFile(QFileInfo(file).absoluteFilePath());
        } else {
            errorMessage = serializeErrors(errors);
            delete controller;
        }
    }

    if (!errorMessage.isEmpty()) {
        QErrorMessage msgBox(this);
        msgBox.showMessage(errorMessage);
        msgBox.exec();
    }
}

void MainWindow::newFile(const QString &templateName)
{
    if (templateName.isNull())
        return;

    QFile file(templateName);
    if (!file.open(QFile::ReadOnly))
        return;

    newFile(file.readAll());
}

void MainWindow::doAbout()
{
    AboutDialog::go(this);
}

void MainWindow::documentCountChanged(unsigned newCount)
{
    if (m_documentCount == newCount)
        return;
    m_documentCount = newCount;

    if (!m_shutdown) {
        updateActions();
        updateMainArea();
    }
}

void MainWindow::updateActions()
{
    bool enable = m_documentCount != 0;
    foreach (QAction *documentAction, m_documentActions)
        documentAction->setEnabled(enable);
}

void MainWindow::updateMainArea()
{
    if (m_multipleDocumentsController->activeDocumentCount() == 0) {
        m_welcomeScreen->setRecentFiles(recentFiles());
        m_mainArea->setCurrentIndex(0); // welcome screen
    } else {
        m_mainArea->setCurrentIndex(1);
    }
}

void MainWindow::newFile(const QByteArray &templateContents)
{
    DesignDocumentController *controller = new DesignDocumentController(m_multipleDocumentsController);
    QList<QmlDesigner::RewriterView::Error> errors = controller->loadMaster(templateContents);

    if (errors.isEmpty()) {
        connect(controller, SIGNAL(fileToOpen(QString)), this, SLOT(openFile(QString)));
        m_multipleDocumentsController->open(controller);
    } else {
        delete controller;
        QErrorMessage msgBox(this);
        msgBox.showMessage(serializeErrors(errors));
        msgBox.exec();
    }
}

QStringList MainWindow::recentFiles() const
{
    const QSettings settings;
    return settings.value("recentFiles", QStringList()).toStringList();
}

void MainWindow::addRecentFile(const QString &path)
{
    QSettings settings;
    QStringList files = settings.value("recentFiles", QStringList()).toStringList();

    files.removeAll(path);
    if (files.size() > 10)
        files.removeLast();
    files.prepend(path);

    settings.setValue("recentFiles", files);
}

QString MainWindow::serializeErrors(const QList<QmlDesigner::RewriterView::Error> &errors)
{
    if (errors.isEmpty())
        return QString();
    QString errorMsg = tr("Qml Errors:");
    foreach (const QmlDesigner::RewriterView::Error &error, errors) {
        if (!error.url().isEmpty())
         errorMsg.append(tr("\n%1 %2:%3 - %4").arg(error.url().toString(), QString(error.line()), QString(error.column()), error.description()));
        else
         errorMsg.append(tr("\n%1:%2 - %3").arg(QString(error.line()), QString(error.column()), error.description()));
    }
    return errorMsg;
}
