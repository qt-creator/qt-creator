/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "designmodewidget.h"
#include "qmldesignerconstants.h"
#include "styledoutputpaneplaceholder.h"
#include "designmodecontext.h"

#include <model.h>
#include <rewriterview.h>
#include <formeditorwidget.h>
#include <stateseditorwidget.h>
#include <itemlibrarywidget.h>
#include <componentaction.h>
#include <toolbox.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/inavigationwidgetfactory.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/parameteraction.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/crumblepath.h>

#include <QSettings>
#include <QEvent>
#include <QDir>
#include <QApplication>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QToolButton>
#include <QMenu>
#include <QClipboard>
#include <QLabel>
#include <QProgressDialog>

using Core::MiniSplitter;
using Core::IEditor;
using Core::EditorManager;

using namespace QmlDesigner;

enum {
    debug = false
};

const char SB_NAVIGATOR[] = "Navigator";
const char SB_LIBRARY[] = "Library";
const char SB_PROPERTIES[] = "Properties";
const char SB_PROJECTS[] = "Projects";
const char SB_FILESYSTEM[] = "FileSystem";
const char SB_OPENDOCUMENTS[] = "OpenDocuments";

namespace QmlDesigner {
namespace Internal {

DesignModeWidget *DesignModeWidget::s_instance = 0;

DocumentWarningWidget::DocumentWarningWidget(DesignModeWidget *parent) :
        Utils::FakeToolTip(parent),
        m_errorMessage(new QLabel("Placeholder", this)),
        m_goToError(new QLabel(this)),
        m_designModeWidget(parent)
{
    setWindowFlags(Qt::Widget); //We only want the visual style from a ToolTip
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
    QString str;
    if (error.type() == RewriterView::Error::ParseError) {
        str = tr("%3 (%1:%2)").arg(QString::number(error.line()), QString::number(error.column()), error.description());
        m_goToError->show();
    }  else if (error.type() == RewriterView::Error::InternalError) {
        str = tr("Internal error (%1)").arg(error.description());
        m_goToError->hide();
    }

    m_errorMessage->setText(str);
    resize(layout()->totalSizeHint());
}

class ItemLibrarySideBarItem : public Core::SideBarItem
{
public:
    explicit ItemLibrarySideBarItem(ItemLibraryWidget *widget, const QString &id);
    virtual ~ItemLibrarySideBarItem();

    virtual QList<QToolButton *> createToolBarWidgets();
};

ItemLibrarySideBarItem::ItemLibrarySideBarItem(ItemLibraryWidget *widget, const QString &id) : Core::SideBarItem(widget, id) {}

ItemLibrarySideBarItem::~ItemLibrarySideBarItem()
{

}

QList<QToolButton *> ItemLibrarySideBarItem::createToolBarWidgets()
{
    return qobject_cast<ItemLibraryWidget*>(widget())->createToolBarWidgets();
}

class NavigatorSideBarItem : public Core::SideBarItem
{
public:
    explicit NavigatorSideBarItem(NavigatorWidget *widget, const QString &id);
    virtual ~NavigatorSideBarItem();

    virtual QList<QToolButton *> createToolBarWidgets();
};

NavigatorSideBarItem::NavigatorSideBarItem(NavigatorWidget *widget, const QString &id) : Core::SideBarItem(widget, id) {}

NavigatorSideBarItem::~NavigatorSideBarItem()
{

}

QList<QToolButton *> NavigatorSideBarItem::createToolBarWidgets()
{
    return qobject_cast<NavigatorWidget*>(widget())->createToolBarWidgets();
}

void DocumentWarningWidget::goToError()
{
    m_designModeWidget->textEditor()->gotoLine(m_error.line(), m_error.column() - 1);
    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
}

// ---------- DesignModeWidget
DesignModeWidget::DesignModeWidget(QWidget *parent) :
    QWidget(parent),
    m_syncWithTextEdit(false),
    m_mainSplitter(0),
    m_leftSideBar(0),
    m_rightSideBar(0),
    m_isDisabled(false),
    m_showSidebars(true),
    m_initStatus(NotInitialized),
    m_warningWidget(0),
    m_navigatorHistoryCounter(-1),
    m_keepNavigatorHistory(false)
{
    s_instance = this;
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
    m_hideSidebarsAction = new QAction(tr("Toggle Full Screen"), this);
    connect(m_hideSidebarsAction, SIGNAL(triggered()), this, SLOT(toggleSidebars()));
    m_restoreDefaultViewAction = new QAction(tr("&Restore Default View"), this);
    m_goIntoComponentAction  = new QAction(tr("&Go into Component"), this);
    connect(m_restoreDefaultViewAction, SIGNAL(triggered()), SLOT(restoreDefaultView()));
    connect(m_goIntoComponentAction, SIGNAL(triggered()), SLOT(goIntoComponent()));
    m_toggleLeftSidebarAction = new QAction(tr("Toggle &Left Sidebar"), this);
    connect(m_toggleLeftSidebarAction, SIGNAL(triggered()), SLOT(toggleLeftSidebar()));
    m_toggleRightSidebarAction = new QAction(tr("Toggle &Right Sidebar"), this);
    connect(m_toggleRightSidebarAction, SIGNAL(triggered()), SLOT(toggleRightSidebar()));

    m_outputPlaceholderSplitter = new Core::MiniSplitter;
    m_outputPanePlaceholder = new StyledOutputpanePlaceHolder(Core::DesignMode::instance(), m_outputPlaceholderSplitter);
}

DesignModeWidget::~DesignModeWidget()
{
    s_instance = 0;
}

void DesignModeWidget::restoreDefaultView()
{
    QSettings *settings = Core::ICore::settings();
    m_leftSideBar->closeAllWidgets();
    m_rightSideBar->closeAllWidgets();
    m_leftSideBar->readSettings(settings,  "none.LeftSideBar");
    m_rightSideBar->readSettings(settings, "none.RightSideBar");
    m_leftSideBar->show();
    m_rightSideBar->show();
}

void DesignModeWidget::toggleLeftSidebar()
{
    if (m_leftSideBar)
        m_leftSideBar->setVisible(!m_leftSideBar->isVisible());
}

void DesignModeWidget::toggleRightSidebar()
{
    if (m_rightSideBar)
        m_rightSideBar->setVisible(!m_rightSideBar->isVisible());
}

void DesignModeWidget::toggleSidebars()
{
    if (m_initStatus == Initializing)
        return;

    m_showSidebars = !m_showSidebars;

    if (m_leftSideBar)
        m_leftSideBar->setVisible(m_showSidebars);
    if (m_rightSideBar)
        m_rightSideBar->setVisible(m_showSidebars);
    if (!m_statesEditorView.isNull())
        m_statesEditorView->widget()->setVisible(m_showSidebars);

}

void DesignModeWidget::showEditor(Core::IEditor *editor)
{
    if (m_textEditor && editor)
        if (m_textEditor->document()->fileName() != editor->document()->fileName()) {
            if (!m_keepNavigatorHistory)
                addNavigatorHistoryEntry(editor->document()->fileName());
            setupNavigatorHistory();
        }

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
        fileName = editor->document()->fileName();
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

            newDocument->setNodeInstanceView(m_nodeInstanceView.data());
            newDocument->setPropertyEditorView(m_propertyEditorView.data());
            newDocument->setNavigator(m_navigatorView.data());
            newDocument->setStatesEditorView(m_statesEditorView.data());
            newDocument->setItemLibraryView(m_itemLibraryView.data());
            newDocument->setFormEditorView(m_formEditorView.data());
            newDocument->setComponentView(m_componentView.data());


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
                    qDebug() << Q_FUNC_INFO << editor->document()->fileName();
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

QAction *DesignModeWidget::hideSidebarsAction() const
{
    return m_hideSidebarsAction;
}

QAction *DesignModeWidget::toggleLeftSidebarAction() const
{
    return m_toggleLeftSidebarAction;
}

QAction *DesignModeWidget::toggleRightSidebarAction() const
{
    return m_toggleRightSidebarAction;
}


QAction *DesignModeWidget::restoreDefaultViewAction() const
{
    return m_restoreDefaultViewAction;
}

QAction *DesignModeWidget::goIntoComponentAction() const
{
    return m_goIntoComponentAction;
}

void DesignModeWidget::readSettings()
{
    QSettings *settings = Core::ICore::settings();

    settings->beginGroup("Bauhaus");
    m_leftSideBar->readSettings(settings, QLatin1String("LeftSideBar"));
    m_rightSideBar->readSettings(settings, QLatin1String("RightSideBar"));
    if (settings->contains("MainSplitter")) {
        const QByteArray splitterState = settings->value("MainSplitter").toByteArray();
        m_mainSplitter->restoreState(splitterState);
        m_mainSplitter->setOpaqueResize(); // force opaque resize since it used to be off
    }
    settings->endGroup();
}

void DesignModeWidget::saveSettings()
{
    QSettings *settings = Core::ICore::settings();

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

void DesignModeWidget::goIntoComponent()
{
    if (m_currentDesignDocumentController)
        m_currentDesignDocumentController->goIntoComponent();
}

void DesignModeWidget::enable()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    m_warningWidget->setVisible(false);
    m_formEditorView->widget()->setEnabled(true);
    m_statesEditorView->widget()->setEnabled(true);
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
    m_statesEditorView->widget()->setEnabled(false);
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

    RewriterView *rewriter = m_currentDesignDocumentController->rewriterView();

    m_currentDesignDocumentController->blockModelSync(!sync);

    if (sync) {
        if (rewriter && m_currentDesignDocumentController->model())
            rewriter->setSelectedModelNodes(QList<ModelNode>());
        // text editor -> visual editor
        if (!m_currentDesignDocumentController->model()) {
            m_currentDesignDocumentController->loadMaster(m_currentTextEdit.data());
        } else {
            m_currentDesignDocumentController->loadCurrentModel();
            m_componentView->resetView();
        }

        QList<RewriterView::Error> errors = m_currentDesignDocumentController->qmlErrors();
        if (errors.isEmpty()) {
            // set selection to text cursor
            const int cursorPos = m_currentTextEdit->textCursor().position();
            ModelNode node = nodeForPosition(cursorPos);
            if (rewriter && node.isValid()) {
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
    QList<Core::INavigationWidgetFactory *> factories =
            ExtensionSystem::PluginManager::getObjects<Core::INavigationWidgetFactory>();

    QWidget *openDocumentsWidget = 0;
    QWidget *projectsExplorer = 0;
    QWidget *fileSystemExplorer = 0;

    foreach (Core::INavigationWidgetFactory *factory, factories) {
        Core::NavigationView navigationView;
        navigationView.widget = 0;
        if (factory->id() == "Projects") {
            navigationView = factory->createWidget();
            projectsExplorer = navigationView.widget;
            projectsExplorer->setWindowTitle(tr("Projects"));
        } else if (factory->id() == "File System") {
            navigationView = factory->createWidget();
            fileSystemExplorer = navigationView.widget;
            fileSystemExplorer->setWindowTitle(tr("File System"));
        } else if (factory->id() == "Open Documents") {
            navigationView = factory->createWidget();
            openDocumentsWidget = navigationView.widget;
            openDocumentsWidget->setWindowTitle(tr("Open Documents"));
        }

        if (navigationView.widget) {
            QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
            sheet += Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css");
            sheet += "QLabel { background-color: #4f4f4f; }";
            navigationView.widget->setStyleSheet(QString::fromLatin1(sheet));
        }
    }

    m_nodeInstanceView = new NodeInstanceView(this);
    connect(m_nodeInstanceView.data(), SIGNAL(qmlPuppetCrashed()), this, SLOT(qmlPuppetCrashed()));
     // Sidebar takes ownership
    m_navigatorView = new NavigatorView;
    m_propertyEditorView = new PropertyEditor(this);
    m_itemLibraryView = new ItemLibraryView(this);

    m_statesEditorView = new StatesEditorView(this);

    m_formEditorView = new FormEditorView(this);
    connect(m_formEditorView->crumblePath(), SIGNAL(elementClicked(QVariant)), this, SLOT(onCrumblePathElementClicked(QVariant)));

    m_componentView = new ComponentView(this);
    m_formEditorView->widget()->toolBox()->addLeftSideAction(m_componentView->action());
    m_fakeToolBar = Core::EditorManager::createToolBar(this);

    m_mainSplitter = new MiniSplitter(this);
    m_mainSplitter->setObjectName("mainSplitter");

    // warning frame should be not in layout, but still child of the widget
    m_warningWidget = new DocumentWarningWidget(this);
    m_warningWidget->setVisible(false);

    Core::SideBarItem *navigatorItem = new NavigatorSideBarItem(m_navigatorView->widget(), QLatin1String(SB_NAVIGATOR));
    Core::SideBarItem *libraryItem = new ItemLibrarySideBarItem(m_itemLibraryView->widget(), QLatin1String(SB_LIBRARY));
    Core::SideBarItem *propertiesItem = new Core::SideBarItem(m_propertyEditorView->widget(), QLatin1String(SB_PROPERTIES));

    // default items
    m_sideBarItems << navigatorItem << libraryItem << propertiesItem;

    if (projectsExplorer) {
        Core::SideBarItem *projectExplorerItem = new Core::SideBarItem(projectsExplorer, QLatin1String(SB_PROJECTS));
        m_sideBarItems << projectExplorerItem;
    }

    if (fileSystemExplorer) {
        Core::SideBarItem *fileSystemExplorerItem = new Core::SideBarItem(fileSystemExplorer, QLatin1String(SB_FILESYSTEM));
        m_sideBarItems << fileSystemExplorerItem;
    }

    if (openDocumentsWidget) {
        Core::SideBarItem *openDocumentsItem = new Core::SideBarItem(openDocumentsWidget, QLatin1String(SB_OPENDOCUMENTS));
        m_sideBarItems << openDocumentsItem;
    }

    m_leftSideBar = new Core::SideBar(m_sideBarItems, QList<Core::SideBarItem*>() << navigatorItem << libraryItem);
    m_rightSideBar = new Core::SideBar(m_sideBarItems, QList<Core::SideBarItem*>() << propertiesItem);

    connect(m_leftSideBar, SIGNAL(availableItemsChanged()), SLOT(updateAvailableSidebarItemsRight()));
    connect(m_rightSideBar, SIGNAL(availableItemsChanged()), SLOT(updateAvailableSidebarItemsLeft()));

    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()),
            this, SLOT(deleteSidebarWidgets()));

    m_fakeToolBar->setToolbarCreationFlags(Core::EditorToolBar::FlagsStandalone);
    //m_fakeToolBar->addEditor(textEditor()); ### what does this mean?
    m_fakeToolBar->setNavigationVisible(true);

    connect(m_fakeToolBar, SIGNAL(closeClicked()), this, SLOT(closeCurrentEditor()));
    connect(m_fakeToolBar, SIGNAL(goForwardClicked()), this, SLOT(onGoForwardClicked()));
    connect(m_fakeToolBar, SIGNAL(goBackClicked()), this, SLOT(onGoBackClicked()));
    setupNavigatorHistory();

    // right area:
    QWidget *centerWidget = new QWidget;
    {
        QVBoxLayout *rightLayout = new QVBoxLayout(centerWidget);
        rightLayout->setMargin(0);
        rightLayout->setSpacing(0);
        rightLayout->addWidget(m_fakeToolBar);
        //### we now own these here
        rightLayout->addWidget(m_statesEditorView->widget());

        FormEditorContext *formEditorContext = new FormEditorContext(m_formEditorView->widget());
        Core::ICore::addContextObject(formEditorContext);

        NavigatorContext *navigatorContext = new NavigatorContext(m_navigatorView->widget());
        Core::ICore::addContextObject(navigatorContext);

        // editor and output panes
        m_outputPlaceholderSplitter->addWidget(m_formEditorView->widget());
        m_outputPlaceholderSplitter->addWidget(m_outputPanePlaceholder);
        m_outputPlaceholderSplitter->setStretchFactor(0, 10);
        m_outputPlaceholderSplitter->setStretchFactor(1, 0);
        m_outputPlaceholderSplitter->setOrientation(Qt::Vertical);

        rightLayout->addWidget(m_outputPlaceholderSplitter);
    }

    // m_mainSplitter area:
    m_mainSplitter->addWidget(m_leftSideBar);
    m_mainSplitter->addWidget(centerWidget);
    m_mainSplitter->addWidget(m_rightSideBar);

    // Finishing touches:
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setSizes(QList<int>() << 150 << 300 << 150);

    QLayout *mainLayout = new QBoxLayout(QBoxLayout::RightToLeft, this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_mainSplitter);

    m_warningWidget->setVisible(false);
    m_statesEditorView->widget()->setEnabled(true);
    m_leftSideBar->setEnabled(true);
    m_rightSideBar->setEnabled(true);
    m_leftSideBar->setCloseWhenEmpty(true);
    m_rightSideBar->setCloseWhenEmpty(true);

    readSettings();

    show();
    QApplication::processEvents();
}

void DesignModeWidget::updateAvailableSidebarItemsRight()
{
    // event comes from m_leftSidebar, so update right side.
    m_rightSideBar->setUnavailableItemIds(m_leftSideBar->unavailableItemIds());
}

void DesignModeWidget::updateAvailableSidebarItemsLeft()
{
    // event comes from m_rightSidebar, so update left side.
    m_leftSideBar->setUnavailableItemIds(m_rightSideBar->unavailableItemIds());
}

void DesignModeWidget::deleteSidebarWidgets()
{
    delete m_leftSideBar;
    delete m_rightSideBar;
    m_leftSideBar = 0;
    m_rightSideBar = 0;
}

void DesignModeWidget::qmlPuppetCrashed()
{
    QList<RewriterView::Error> errorList;
    RewriterView::Error error(tr("Qt Quick emulation layer crashed"));
    errorList << error;
    disable(errorList);
}

void DesignModeWidget::onGoBackClicked()
{
    if (m_navigatorHistoryCounter > 0) {
        --m_navigatorHistoryCounter;
        m_keepNavigatorHistory = true;
        Core::EditorManager::openEditor(m_navigatorHistory.at(m_navigatorHistoryCounter));
        m_keepNavigatorHistory = false;
    }
}

void DesignModeWidget::onGoForwardClicked()
{
    if (m_navigatorHistoryCounter < (m_navigatorHistory.size() - 1)) {
        ++m_navigatorHistoryCounter;
        m_keepNavigatorHistory = true;
        Core::EditorManager::openEditor(m_navigatorHistory.at(m_navigatorHistoryCounter));
        m_keepNavigatorHistory = false;
    }
}

void DesignModeWidget::onCrumblePathElementClicked(const QVariant &data)
{
    currentDesignDocumentController()->setCrumbleBarInfo(data.value<CrumbleBarInfo>());
}

DesignModeWidget *DesignModeWidget::instance()
{
    return s_instance;
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

void DesignModeWidget::setupNavigatorHistory()
{
    const bool canGoBack = m_navigatorHistoryCounter > 0;
    const bool canGoForward = m_navigatorHistoryCounter < (m_navigatorHistory.size() - 1);
    m_fakeToolBar->setCanGoBack(canGoBack);
    m_fakeToolBar->setCanGoForward(canGoForward);
}

void DesignModeWidget::addNavigatorHistoryEntry(const QString &fileName)
{
    if (m_navigatorHistoryCounter > 0)
        m_navigatorHistory.insert(m_navigatorHistoryCounter + 1, fileName);
    else
        m_navigatorHistory.append(fileName);

    ++m_navigatorHistoryCounter;
}


QString DesignModeWidget::contextHelpId() const
{
    if (m_currentDesignDocumentController)
        return m_currentDesignDocumentController->contextHelpId();
    return QString();
}

} // namespace Internal
} // namespace Designer
