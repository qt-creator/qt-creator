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

#include "designmodewidget.h"
#include "qmldesignerconstants.h"

#include <model.h>
#include <rewriterview.h>
#include <formeditorwidget.h>

#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <QtCore/QSettings>
#include <QtCore/QEvent>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QScrollArea>
#include <QtGui/QTabWidget>
#include <QtGui/QToolButton>
#include <QtGui/QMenu>
#include <QtGui/QClipboard>
#include <QtGui/QLabel>
#include <QtGui/QProgressDialog>

using Core::MiniSplitter;
using Core::IEditor;
using Core::EditorManager;

using namespace QmlDesigner;

Q_DECLARE_METATYPE(Core::IEditor*)

enum {
    debug = false
};

namespace QmlDesigner {
namespace Internal {

DocumentWarningWidget::DocumentWarningWidget(DesignModeWidget *parent) :
        QFrame(parent),
        m_errorMessage(new QLabel("Placeholder", this)),
        m_goToError(new QLabel(this)),
        m_designModeWidget(parent)
{
    setFrameStyle(QFrame::Panel | QFrame::Raised);
    setLineWidth(1);
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setAutoFillBackground(true);

    m_errorMessage->setForegroundRole(QPalette::ToolTipText);
    m_goToError->setText(tr("<a href=\"goToError\">Go to error</a>"));
    m_goToError->setForegroundRole(QPalette::Link);
    connect(m_goToError, SIGNAL(linkActivated(QString)), this, SLOT(goToError()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(20);
    layout->setSpacing(5);
    layout->addWidget(m_errorMessage);
    layout->addWidget(m_goToError, 1, Qt::AlignRight);
}

void DocumentWarningWidget::setError(const RewriterView::Error &error)
{
    m_error = error;
    QString str = tr("%3 (%1:%2)").arg(QString::number(error.line()), QString::number(error.column()), error.description());

    m_errorMessage->setText(str);
    resize(layout()->totalSizeHint());
}

void DocumentWarningWidget::goToError()
{
    m_designModeWidget->textEditor()->gotoLine(m_error.line(), m_error.column());
    Core::EditorManager::instance()->ensureEditorManagerVisible();
}

// ---------- DesignModeWidget
DesignModeWidget::DesignModeWidget(QWidget *parent) :
    QWidget(parent),
    m_syncWithTextEdit(false),
    m_mainSplitter(0),
    m_leftSideBar(0),
    m_rightSideBar(0),
    m_initStatus(NotInitialized),
    m_warningWidget(0)
{
    m_undoAction = new QAction(tr("&Undo"), this);
    connect(m_undoAction, SIGNAL(triggered()), this, SLOT(undo()));
    m_redoAction = new QAction(tr("&Redo"), this);
    connect(m_redoAction, SIGNAL(triggered()), this, SLOT(redo()));
    m_deleteAction = new Utils::ParameterAction(tr("Delete"), tr("Delete \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(deleteSelected()));
    m_cutAction = new Utils::ParameterAction(tr("Cu&t"), tr("Cut \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    connect(m_cutAction, SIGNAL(triggered()), this, SLOT(cutSelected()));
    m_copyAction = new Utils::ParameterAction(tr("&Copy"), tr("Copy \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    connect(m_copyAction, SIGNAL(triggered()), this, SLOT(copySelected()));
    m_pasteAction = new Utils::ParameterAction(tr("&Paste"), tr("Paste \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    connect(m_pasteAction, SIGNAL(triggered()), this, SLOT(paste()));
    m_selectAllAction = new Utils::ParameterAction(tr("Select &All"), tr("Select All \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    connect(m_selectAllAction, SIGNAL(triggered()), this, SLOT(selectAll()));
}

DesignModeWidget::~DesignModeWidget()
{
};

void DesignModeWidget::showEditor(Core::IEditor *editor)
{
    //
    // Prevent recursive calls to function by explicitly managing initialization status
    // (QApplication::processEvents is called explicitly at a number of places)
    //
    if (m_initStatus == Initializing)
        return;

    if (m_initStatus == NotInitialized) {
        m_initStatus = Initializing;
        setup();
    }

    QString fileName;
    QPlainTextEdit *textEdit = 0;
    TextEditor::ITextEditor *textEditor = 0;

    if (editor) {
        fileName = editor->file()->fileName();
        textEdit = qobject_cast<QPlainTextEdit*>(editor->widget());
        textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);
        if (textEditor)
            m_fakeToolBar->addEditor(textEditor);
    }

    if (debug)
        qDebug() << Q_FUNC_INFO << fileName;

    if (textEdit)
        m_currentTextEdit = textEdit;

    if (textEditor)
        m_textEditor = textEditor;
    DesignDocumentController *document = 0;

    if (textEdit && textEditor && fileName.endsWith(QLatin1String(".qml"))) {
        if (m_documentHash.contains(textEdit)) {
            document = m_documentHash.value(textEdit).data();
        } else {
            DesignDocumentController *newDocument = new DesignDocumentController(this);

            newDocument->setAllPropertiesBox(m_allPropertiesBox.data());
            newDocument->setNavigator(m_navigator.data());
            newDocument->setStatesEditorWidget(m_statesEditorWidget.data());
            newDocument->setItemLibrary(m_itemLibrary.data());
            newDocument->setFormEditorView(m_formEditorView.data());

            newDocument->setFileName(fileName);

            document = newDocument;          

            m_documentHash.insert(textEdit, document);
        }
    }
    setCurrentDocument(document);

    m_initStatus = Initialized;
}

void DesignModeWidget::closeEditors(QList<Core::IEditor*> editors)
{
    foreach (Core::IEditor* editor, editors) {
        if (QPlainTextEdit *textEdit = qobject_cast<QPlainTextEdit*>(editor->widget())) {
            if (m_currentTextEdit.data() == textEdit) {
                setCurrentDocument(0);
            }
            if (m_documentHash.contains(textEdit)) {
                if (debug)
                    qDebug() << Q_FUNC_INFO << editor->file()->fileName();
                DesignDocumentController *document = m_documentHash.take(textEdit).data();
                delete document;
            }
        }
    }
}

QAction *DesignModeWidget::undoAction() const
{
    return m_undoAction;
}

QAction *DesignModeWidget::redoAction() const
{
    return m_redoAction;
}

QAction *DesignModeWidget::deleteAction() const
{
    return m_deleteAction;
}

QAction *DesignModeWidget::cutAction() const
{
    return m_cutAction;
}

QAction *DesignModeWidget::copyAction() const
{
    return m_copyAction;
}

QAction *DesignModeWidget::pasteAction() const
{
    return m_pasteAction;
}

QAction *DesignModeWidget::selectAllAction() const
{
    return m_selectAllAction;
}

void DesignModeWidget::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();

    settings->beginGroup("Bauhaus");
    m_leftSideBar->readSettings(settings, QLatin1String("LeftSideBar"));
    m_rightSideBar->readSettings(settings, QLatin1String("RightSideBar"));
    if (settings->contains("MainSplitter")) {
        const QByteArray splitterState = settings->value("MainSplitter").toByteArray();
        m_mainSplitter->restoreState(splitterState);
    }
    settings->endGroup();
}

void DesignModeWidget::saveSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();

    settings->beginGroup("Bauhaus");
    m_leftSideBar->saveSettings(settings, QLatin1String("LeftSideBar"));
    m_rightSideBar->saveSettings(settings, QLatin1String("RightSideBar"));
    settings->setValue("MainSplitter", m_mainSplitter->saveState());
    settings->endGroup();
}

void DesignModeWidget::undo()
{
    if (m_currentDesignDocumentController)
        m_currentDesignDocumentController->undo();
}

void DesignModeWidget::redo()
{
    if (m_currentDesignDocumentController)
        m_currentDesignDocumentController->redo();
}

void DesignModeWidget::deleteSelected()
{
    if (m_currentDesignDocumentController)
        m_currentDesignDocumentController->deleteSelected();
}

void DesignModeWidget::cutSelected()
{
    if (m_currentDesignDocumentController)
        m_currentDesignDocumentController->cutSelected();
}

void DesignModeWidget::copySelected()
{
    if (m_currentDesignDocumentController)
        m_currentDesignDocumentController->copySelected();
}

void DesignModeWidget::paste()
{
    if (m_currentDesignDocumentController)
        m_currentDesignDocumentController->paste();
}

void DesignModeWidget::selectAll()
{
    if (m_currentDesignDocumentController)
        m_currentDesignDocumentController->selectAll();
}

void DesignModeWidget::closeCurrentEditor()
{
}

void DesignModeWidget::undoAvailable(bool isAvailable)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    if (m_currentDesignDocumentController &&
        m_currentDesignDocumentController.data() == documentController) {
        m_undoAction->setEnabled(isAvailable);
    }
}

void DesignModeWidget::redoAvailable(bool isAvailable)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    if (m_currentDesignDocumentController &&
        m_currentDesignDocumentController.data() == documentController) {
        m_redoAction->setEnabled(isAvailable);
    }
}


void DesignModeWidget::enable()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    m_warningWidget->setVisible(false);
    m_formEditorView->widget()->setEnabled(true);
    m_statesEditorWidget->setEnabled(true);
    m_leftSideBar->setEnabled(true);
    m_rightSideBar->setEnabled(true);
    m_isDisabled = false;
}

void DesignModeWidget::disable(const QList<RewriterView::Error> &errors)
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    Q_ASSERT(!errors.isEmpty());
    m_warningWidget->setError(errors.first());
    m_warningWidget->setVisible(true);
    m_warningWidget->move(width() / 2, height() / 2);
    m_formEditorView->widget()->setEnabled(false);
    m_statesEditorWidget->setEnabled(false);
    m_leftSideBar->setEnabled(false);
    m_rightSideBar->setEnabled(false);
    m_isDisabled = true;
}

void DesignModeWidget::updateErrorStatus(const QList<RewriterView::Error> &errors)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << errors.count();

    if (m_isDisabled && errors.isEmpty()) {
        enable();
    } else if (!errors.isEmpty()) {
        disable(errors);
    }
}

void DesignModeWidget::setAutoSynchronization(bool sync)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << sync;

    m_currentDesignDocumentController->blockModelSync(!sync);

    if (sync) {
        // text editor -> visual editor
        if (!m_currentDesignDocumentController->model()) {
            m_currentDesignDocumentController->loadMaster(m_currentTextEdit.data());
        } else {
            m_currentDesignDocumentController->loadCurrentModel();
        }

        QList<RewriterView::Error> errors = m_currentDesignDocumentController->qmlErrors();
        if (errors.isEmpty()) {
            // set selection to text cursor
            RewriterView *rewriter = m_currentDesignDocumentController->rewriterView();
            const int cursorPos = m_currentTextEdit->textCursor().position();
            ModelNode node = nodeForPosition(cursorPos);
            if (node.isValid()) {
                rewriter->setSelectedModelNodes(QList<ModelNode>() << node);
            }
            enable();
        } else {
            disable(errors);
        }

        connect(m_currentDesignDocumentController.data(), SIGNAL(qmlErrorsChanged(QList<RewriterView::Error>)),
                this, SLOT(updateErrorStatus(QList<RewriterView::Error>)));

    } else {
        if (m_currentDesignDocumentController->model() && m_currentDesignDocumentController->qmlErrors().isEmpty()) {
            RewriterView *rewriter = m_currentDesignDocumentController->rewriterView();
            // visual editor -> text editor
            ModelNode selectedNode;
            if (!rewriter->selectedModelNodes().isEmpty())
                selectedNode = rewriter->selectedModelNodes().first();

            if (selectedNode.isValid()) {
                const int nodeOffset = rewriter->nodeOffset(selectedNode);
                if (nodeOffset > 0) {
                    const ModelNode currentSelectedNode
                            = nodeForPosition(m_currentTextEdit->textCursor().position());
                    if (currentSelectedNode != selectedNode) {
                        int line, column;
                        m_textEditor->convertPosition(nodeOffset, &line, &column);
                        m_textEditor->gotoLine(line, column);
                    }
                }
            }
        }

        disconnect(m_currentDesignDocumentController.data(), SIGNAL(qmlErrorsChanged(QList<RewriterView::Error>)),
                this, SLOT(updateErrorStatus(QList<RewriterView::Error>)));
    }
}

void DesignModeWidget::setCurrentDocument(DesignDocumentController *newDesignDocumentController)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << newDesignDocumentController;

    if (m_currentDesignDocumentController.data() == newDesignDocumentController)
        return;
    if (m_currentDesignDocumentController) {
        setAutoSynchronization(false);
        saveSettings();
    }

    if (currentDesignDocumentController()) {
        disconnect(currentDesignDocumentController(), SIGNAL(undoAvailable(bool)),
            this, SLOT(undoAvailable(bool)));
        disconnect(currentDesignDocumentController(), SIGNAL(redoAvailable(bool)),
            this, SLOT(redoAvailable(bool)));
    }

    m_currentDesignDocumentController = newDesignDocumentController;

    if (currentDesignDocumentController()) {
        connect(currentDesignDocumentController(), SIGNAL(undoAvailable(bool)),
            this, SLOT(undoAvailable(bool)));
        connect(currentDesignDocumentController(), SIGNAL(redoAvailable(bool)),
            this, SLOT(redoAvailable(bool)));
    }

    if (m_currentDesignDocumentController) {

        setAutoSynchronization(true);
        m_undoAction->setEnabled(m_currentDesignDocumentController->isUndoAvailable());
        m_redoAction->setEnabled(m_currentDesignDocumentController->isRedoAvailable());
    } else {
        //detach all views
        m_undoAction->setEnabled(false);
        m_redoAction->setEnabled(false);
    }
}

void DesignModeWidget::setup()
{
    QList<Core::INavigationWidgetFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<Core::INavigationWidgetFactory>();

    QWidget *openDocumentsWidget = 0;
    QWidget *projectsExplorer = 0;
    QWidget *fileSystemExplorer = 0;


    foreach(Core::INavigationWidgetFactory *factory, factories)
    {
        Core::NavigationView navigationView;
        navigationView.widget = 0;
        if (factory->displayName() == QLatin1String("Projects")) {
            navigationView = factory->createWidget();
            projectsExplorer = navigationView.widget;
            projectsExplorer->setWindowTitle(tr("Projects"));
        }
        if (factory->displayName() == QLatin1String("File System")) {
            navigationView = factory->createWidget();
            fileSystemExplorer = navigationView.widget;
            fileSystemExplorer->setWindowTitle(tr("File System"));
        }
        if (factory->displayName() == QLatin1String("Open Documents")) {
            navigationView = factory->createWidget();
            openDocumentsWidget = navigationView.widget;
            openDocumentsWidget->setWindowTitle(tr("Open Documents"));
        }

        if (navigationView.widget)
        {
            QFile file(":/qmldesigner/stylesheet.css");
            file.open(QFile::ReadOnly);
            QFile file2(":/qmldesigner/scrollbar.css");
            file2.open(QFile::ReadOnly);

            QString labelStyle = QLatin1String("QLabel { background-color: #4f4f4f; }");

            QString styleSheet = file.readAll() + file2.readAll() + labelStyle;
            navigationView.widget->setStyleSheet(styleSheet);
        }
    }

    m_navigator = new NavigatorView(this);

    m_allPropertiesBox = new AllPropertiesBox(this);
    m_statesEditorWidget = new StatesEditorWidget(this);
    
    m_formEditorView = new FormEditorView(this);

    m_itemLibrary = new ItemLibrary(this);

    m_designToolBar = new QToolBar;
    m_fakeToolBar = Core::EditorManager::createToolBar(this);


    m_mainSplitter = new MiniSplitter(this);
    m_mainSplitter->setObjectName("mainSplitter");

    // warning frame should be not in layout, but still child of the widget
    m_warningWidget = new DocumentWarningWidget(this);
    m_warningWidget->setVisible(false);

    // Left area:

    Core::SideBarItem *navigatorItem = new Core::SideBarItem(m_navigator->widget());
    Core::SideBarItem *libraryItem = new Core::SideBarItem(m_itemLibrary.data());
    Core::SideBarItem *propertiesItem = new Core::SideBarItem(m_allPropertiesBox.data());


    QList<Core::SideBarItem*> leftSideBarItems, rightSideBarItems;
    leftSideBarItems << navigatorItem << libraryItem;
    rightSideBarItems << propertiesItem;

    if (projectsExplorer) {
        Core::SideBarItem *projectExplorerItem = new Core::SideBarItem(projectsExplorer);
        rightSideBarItems << projectExplorerItem;
    }

    if (fileSystemExplorer) {
        Core::SideBarItem *fileSystemExplorerItem = new Core::SideBarItem(fileSystemExplorer);
        rightSideBarItems << fileSystemExplorerItem;
    }

     if (openDocumentsWidget) {
        Core::SideBarItem *openDocumentsItem = new Core::SideBarItem(openDocumentsWidget);
        rightSideBarItems << openDocumentsItem;
    }

    m_leftSideBar = new Core::SideBar(leftSideBarItems, QList<Core::SideBarItem*>() << navigatorItem << libraryItem);
    m_rightSideBar = new Core::SideBar(rightSideBarItems, QList<Core::SideBarItem*>() << propertiesItem);

    m_designToolBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);

    m_fakeToolBar->setToolbarCreationFlags(Core::EditorToolBar::FlagsStandalone);
    //m_fakeToolBar->addEditor(textEditor()); ### what does this mean?
    m_fakeToolBar->addCenterToolBar(m_designToolBar);
    m_fakeToolBar->setNavigationVisible(false);

    connect(m_fakeToolBar, SIGNAL(closeClicked()), this, SLOT(closeCurrentEditor()));

    // right area:
    QWidget *centerWidget = new QWidget;
    {
        QVBoxLayout *rightLayout = new QVBoxLayout(centerWidget);
        rightLayout->setMargin(0);
        rightLayout->setSpacing(0);
        rightLayout->addWidget(m_fakeToolBar);
        //### we now own these here
        rightLayout->addWidget(m_statesEditorWidget.data());
        rightLayout->addWidget(m_formEditorView->widget());
    }

    // m_mainSplitter area:
    m_mainSplitter->addWidget(m_leftSideBar);
    m_mainSplitter->addWidget(centerWidget);
    m_mainSplitter->addWidget(m_rightSideBar);

    // Finishing touches:
    m_mainSplitter->setOpaqueResize(false);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setSizes(QList<int>() << 150 << 300 << 150);

    QLayout *mainLayout = new QBoxLayout(QBoxLayout::RightToLeft, this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_mainSplitter);

    m_warningWidget->setVisible(false);
    m_statesEditorWidget->setEnabled(true);
    m_leftSideBar->setEnabled(true);
    m_rightSideBar->setEnabled(true);

    readSettings();

    show();
    QApplication::processEvents();
}

void DesignModeWidget::resizeEvent(QResizeEvent *event)
{
    if (m_warningWidget)
        m_warningWidget->move(QPoint(event->size().width() / 2, event->size().height() / 2));
    QWidget::resizeEvent(event);
}


bool DesignModeWidget::isInNodeDefinition(int nodeOffset, int nodeLength, int cursorPos) const {
    return (nodeOffset <= cursorPos) && (nodeOffset + nodeLength > cursorPos);
}


ModelNode DesignModeWidget::nodeForPosition(int cursorPos) const
{
    RewriterView *rewriter = m_currentDesignDocumentController->rewriterView();
    QList<ModelNode> nodes = rewriter->allModelNodes();

    ModelNode bestNode;
    int bestNodeOffset = -1;

    foreach (const ModelNode &node, nodes) {
        const int nodeOffset = rewriter->nodeOffset(node);
        const int nodeLength = rewriter->nodeLength(node);
        if (isInNodeDefinition(nodeOffset, nodeLength, cursorPos)
            && (nodeOffset > bestNodeOffset)) {
            bestNode = node;
            bestNodeOffset = nodeOffset;
        }
    }

    return bestNode;
}


QString DesignModeWidget::contextHelpId() const
{
    if (m_currentDesignDocumentController)
        return m_currentDesignDocumentController->contextHelpId();
    return QString();
}

} // namespace Internal
} // namespace Designer
