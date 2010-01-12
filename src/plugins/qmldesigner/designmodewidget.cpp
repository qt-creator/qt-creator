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

#include "designmodewidget.h"
#include "designmode.h"
#include "qmldesignerconstants.h"

#include <model.h>
#include <rewriterview.h>

#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/modemanager.h>

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

/*!
  Mimic the look of the text editor toolbar as defined in e.g. EditorView::EditorView
  */
DocumentToolBar::DocumentToolBar(DocumentWidget *documentWidget, DesignModeWidget *mainWidget, QWidget *parent) :
        QWidget(parent),
        m_mainWidget(mainWidget),
        m_documentWidget(documentWidget),
        m_editorList(new QComboBox),
        m_closeButton(new QToolButton),
        m_lockButton(new QToolButton)
{
    Core::ICore *core = Core::ICore::instance();

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    m_editorsListModel = core->editorManager()->openedEditorsModel();

    // copied from EditorView::EditorView
    m_editorList->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_editorList->setMinimumContentsLength(20);
    m_editorList->setModel(m_editorsListModel);
    m_editorList->setMaxVisibleItems(40);
    m_editorList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_editorList->setCurrentIndex(m_editorsListModel->indexOf(documentWidget->textEditor()).row());

    QToolBar *editorListToolBar = new QToolBar;
    editorListToolBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    editorListToolBar->addWidget(m_editorList);

    QToolBar *designToolBar = new QToolBar;
    designToolBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);

    m_lockButton->setAutoRaise(true);
    m_lockButton->setProperty("type", QLatin1String("dockbutton"));

    m_closeButton->setAutoRaise(true);
    m_closeButton->setIcon(QIcon(":/core/images/closebutton.png"));
    m_closeButton->setProperty("type", QLatin1String("dockbutton"));

    QToolBar *rightToolBar = new QToolBar;
    rightToolBar->setLayoutDirection(Qt::RightToLeft);
    rightToolBar->addWidget(m_closeButton);
    rightToolBar->addWidget(m_lockButton);

    QHBoxLayout *toplayout = new QHBoxLayout(this);
    toplayout->setSpacing(0);
    toplayout->setMargin(0);

    toplayout->addWidget(editorListToolBar);
    toplayout->addWidget(designToolBar);
    toplayout->addWidget(rightToolBar);

    connect(m_editorList, SIGNAL(activated(int)), this, SLOT(listSelectionActivated(int)));
    connect(m_editorList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(listContextMenu(QPoint)));
    connect(m_lockButton, SIGNAL(clicked()), this, SLOT(makeEditorWritable()));
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(close()));

    connect(m_documentWidget->textEditor(), SIGNAL(changed()), this, SLOT(updateEditorStatus()));

    updateEditorStatus();
}

void DocumentToolBar::close()
{
    Core::ICore::instance()->editorManager()->closeEditors(QList<IEditor*>() << m_documentWidget->textEditor());
}

void DocumentToolBar::listSelectionActivated(int row)
{
    EditorManager *em = Core::ICore::instance()->editorManager();
    QAbstractItemModel *model = m_editorList->model();

    const QModelIndex modelIndex = model->index(row, 0);
    IEditor *editor = model->data(modelIndex, Qt::UserRole).value<IEditor*>();
    if (editor) {
        em->activateEditor(editor, EditorManager::NoModeSwitch);
    } else {
        QString fileName = model->data(modelIndex, Qt::UserRole + 1).toString();
        QByteArray kind = model->data(modelIndex, Qt::UserRole + 2).toByteArray();
        editor = em->openEditor(fileName, kind, EditorManager::NoModeSwitch);
    }
    if (editor) {
        m_mainWidget->showEditor(editor);
        m_editorList->setCurrentIndex(m_editorsListModel->indexOf(m_documentWidget->textEditor()).row());
    }
}

void DocumentToolBar::listContextMenu(QPoint pos)
{
    QModelIndex index = m_editorsListModel->index(m_editorList->currentIndex(), 0);
    QString fileName = m_editorsListModel->data(index, Qt::UserRole + 1).toString();
    if (fileName.isEmpty())
        return;
    QMenu menu;
    menu.addAction(tr("Copy full path to clipboard"));
    if (menu.exec(m_editorList->mapToGlobal(pos))) {
        QApplication::clipboard()->setText(fileName);
    }
}

void DocumentToolBar::makeEditorWritable()
{
    Core::ICore::instance()->editorManager()->makeEditorWritable(m_documentWidget->textEditor());
}

void DocumentToolBar::updateEditorStatus()
{
    Core::IEditor *editor = m_documentWidget->textEditor();

    static const QIcon lockedIcon(QLatin1String(":/core/images/locked.png"));
    static const QIcon unlockedIcon(QLatin1String(":/core/images/unlocked.png"));

    if (editor->file()->isReadOnly()) {
        m_lockButton->setIcon(lockedIcon);
        m_lockButton->setEnabled(!editor->file()->fileName().isEmpty());
        m_lockButton->setToolTip(tr("Make writable"));
    } else {
        m_lockButton->setIcon(unlockedIcon);
        m_lockButton->setEnabled(false);
        m_lockButton->setToolTip(tr("File is writable"));
    }
    m_editorList->setToolTip(
            editor->file()->fileName().isEmpty()
            ? editor->displayName()
                : QDir::toNativeSeparators(editor->file()->fileName())
                );
}

DocumentWarningWidget::DocumentWarningWidget(DocumentWidget *documentWidget, QWidget *parent) :
        QFrame(parent),
        m_errorMessage(new QLabel("Placeholder", this)),
        m_goToError(new QLabel(this)),
        m_documentWidget(documentWidget)
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
    m_documentWidget->textEditor()->gotoLine(m_error.line(), m_error.column());
    Core::EditorManager::instance()->ensureEditorManagerVisible();
}

DocumentWidget::DocumentWidget(TextEditor::ITextEditor *textEditor, QPlainTextEdit *textEdit, QmlDesigner::DesignDocumentController *document, DesignModeWidget *mainWidget) :
        QWidget(),
        m_textEditor(textEditor),
        m_textBuffer(textEdit),
        m_document(document),
        m_mainWidget(mainWidget),
        m_mainSplitter(0),
        m_leftSideBar(0),
        m_rightSideBar(0),
        m_isDisabled(false),
        m_warningWidget(0)
{
    setup();
}

DocumentWidget::~DocumentWidget()
{
    // Make sure component widgets are deleted first in SideBarItem::~SideBarItem
    // before the DesignDocumentController runs (and deletes them again).
    m_document->deleteLater();
}

QmlDesigner::DesignDocumentController *DocumentWidget::document() const
{
    return m_document;
}

TextEditor::ITextEditor *DocumentWidget::textEditor() const
{
    return m_textEditor;
}

void DocumentWidget::setAutoSynchronization(bool sync)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << sync;

    document()->blockModelSync(!sync);

    if (sync) {
        // text editor -> visual editor
        if (!document()->model()) {
            // first initialization
            QList<RewriterView::Error> errors = document()->loadMaster(m_textBuffer.data());
            if (!errors.isEmpty()) {
                disable(errors);
            }
        }
        if (document()->model() && document()->qmlErrors().isEmpty()) {
            // set selection to text cursor
            RewriterView *rewriter = document()->rewriterView();
            const int cursorPos = m_textBuffer->textCursor().position();
            ModelNode node = nodeForPosition(cursorPos);
            if (node.isValid()) {
                rewriter->setSelectedModelNodes(QList<ModelNode>() << node);
            }
            enable();
        }

        connect(document(), SIGNAL(qmlErrorsChanged(QList<RewriterView::Error>)),
                this, SLOT(updateErrorStatus(QList<RewriterView::Error>)));

    } else {
        if (document()->model() && document()->qmlErrors().isEmpty()) {
            RewriterView *rewriter = document()->rewriterView();
            // visual editor -> text editor
            ModelNode selectedNode;
            if (!rewriter->selectedModelNodes().isEmpty())
                selectedNode = rewriter->selectedModelNodes().first();

            if (selectedNode.isValid()) {
                int nodeOffset = rewriter->nodeOffset(selectedNode);
                QTextCursor editTextCursor = m_textBuffer->textCursor();
                if (nodeOffset > 0
                    && nodeForPosition(editTextCursor.position()) != selectedNode) {
                    if (debug)
                        qDebug() << "Moving text cursor to " << nodeOffset;
                    editTextCursor.setPosition(nodeOffset);
                    m_textBuffer->setTextCursor(editTextCursor);
                }
            }
        }

        disconnect(document(), SIGNAL(qmlErrorsChanged(QList<RewriterView::Error>)),
                this, SLOT(updateErrorStatus(QList<RewriterView::Error>)));
    }
}

void DocumentWidget::enable()
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    m_warningWidget->setVisible(false);
    m_document->documentWidget()->setEnabled(true);
    m_document->statesEditorWidget()->setEnabled(true);
    m_leftSideBar->setEnabled(true);
    m_rightSideBar->setEnabled(true);
    m_isDisabled = false;
}

void DocumentWidget::disable(const QList<RewriterView::Error> &errors)
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    Q_ASSERT(!errors.isEmpty());
    m_warningWidget->setError(errors.first());
    m_warningWidget->setVisible(true);
    m_document->documentWidget()->setEnabled(false);
    m_document->statesEditorWidget()->setEnabled(false);
    m_leftSideBar->setEnabled(false);
    m_rightSideBar->setEnabled(false);
    m_isDisabled = true;
}

void DocumentWidget::updateErrorStatus(const QList<RewriterView::Error> &errors)
{
    if (m_isDisabled && errors.isEmpty()) {
        enable();
    } else if (!errors.isEmpty()) {
        disable(errors);
    }
}

void DocumentWidget::readSettings()
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

void DocumentWidget::saveSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();

    settings->beginGroup("Bauhaus");
    m_leftSideBar->saveSettings(settings, QLatin1String("LeftSideBar"));
    m_rightSideBar->saveSettings(settings, QLatin1String("RightSideBar"));
    settings->setValue("MainSplitter", m_mainSplitter->saveState());
    settings->endGroup();
}


void DocumentWidget::resizeEvent(QResizeEvent *event)
{
    m_warningWidget->move(QPoint(event->size().width() / 2,
                                 event->size().height() / 2));
    QWidget::resizeEvent(event);
}

void DocumentWidget::setup()
{
    m_mainSplitter = new MiniSplitter(this);
    m_mainSplitter->setObjectName("mainSplitter");

    // warning frame should be not in layout, but still child of the widget
    m_warningWidget = new DocumentWarningWidget(this, this);
    m_warningWidget->setVisible(false);

    // Left area:
    Core::SideBarItem *navigatorItem = new Core::SideBarItem(m_document->navigator());
    Core::SideBarItem *libraryItem = new Core::SideBarItem(m_document->itemLibrary());
    Core::SideBarItem *propertiesItem = new Core::SideBarItem(m_document->allPropertiesBox());

    QList<Core::SideBarItem*> leftSideBarItems, rightSideBarItems;
    leftSideBarItems << navigatorItem << libraryItem;
    rightSideBarItems << propertiesItem;

    m_leftSideBar = new Core::SideBar(leftSideBarItems, QList<Core::SideBarItem*>() << navigatorItem << libraryItem);
    m_rightSideBar = new Core::SideBar(rightSideBarItems, QList<Core::SideBarItem*>() << propertiesItem);

    // right area:
    QWidget *centerWidget = new QWidget;
    {
        QVBoxLayout *rightLayout = new QVBoxLayout(centerWidget);
        rightLayout->setMargin(0);
        rightLayout->setSpacing(0);
        rightLayout->addWidget(new DocumentToolBar(this, m_mainWidget));
        rightLayout->addWidget(m_document->statesEditorWidget());
        rightLayout->addWidget(m_document->documentWidget());
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
}

bool DocumentWidget::isInNodeDefinition(int nodeOffset, int nodeLength, int cursorPos) const {
    return (nodeOffset <= cursorPos) && (nodeOffset + nodeLength > cursorPos);
}

ModelNode DocumentWidget::nodeForPosition(int cursorPos) const
{
    RewriterView *rewriter = m_document->rewriterView();
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

// ---------- DesignModeWidget
DesignModeWidget::DesignModeWidget(DesignMode *designMode, QWidget *parent) :
    QWidget(parent),
    m_designMode(designMode),
    m_documentWidgetStack(new QStackedWidget),
    m_currentDocumentWidget(0),
    m_currentTextEdit(0),
    m_syncWithTextEdit(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_documentWidgetStack);

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

    QLabel *defaultBackground = new QLabel(tr("Open/Create a qml file first."));
    defaultBackground->setAlignment(Qt::AlignCenter);

    m_documentWidgetStack->addWidget(defaultBackground);
}

DesignModeWidget::~DesignModeWidget()
{
};

void DesignModeWidget::showEditor(Core::IEditor *editor)
{
    QString fileName;
    QPlainTextEdit *textEdit = 0;
    TextEditor::ITextEditor *textEditor = 0;

    if (editor) {
        fileName = editor->file()->fileName();
        textEdit = qobject_cast<QPlainTextEdit*>(editor->widget());
        textEditor = qobject_cast<TextEditor::ITextEditor*>(editor);
    }

    if (debug)
        qDebug() << Q_FUNC_INFO << fileName;

    m_currentTextEdit = textEdit;
    DocumentWidget *documentWidget = 0;

    if (textEdit && textEditor && fileName.endsWith(".qml")) {
        if (m_documentHash.contains(textEdit)) {
            documentWidget = m_documentHash.value(textEdit);
            documentWidget->setAutoSynchronization(true);
        } else {
            DesignDocumentController *newDocument = new DesignDocumentController(0);
            newDocument->setFileName(fileName);

            documentWidget = new DocumentWidget(textEditor, textEdit, newDocument, this);
            connect(documentWidget->document(), SIGNAL(undoAvailable(bool)),
                    this, SLOT
(undoAvailable(bool)));
            connect(documentWidget->document(), SIGNAL(redoAvailable(bool)),
                    this, SLOT(redoAvailable(bool)));
//          connect(documentWidget->document(), SIGNAL(deleteAvailable(bool)),
//                  this, SLOT(deleteAvailable(bool)));

            m_documentHash.insert(textEdit, documentWidget);
            m_documentWidgetStack->addWidget(documentWidget);
        }
    }

    setCurrentDocumentWidget(documentWidget);
}

void DesignModeWidget::closeEditors(QList<Core::IEditor*> editors)
{
    foreach (Core::IEditor* editor, editors) {
        if (QPlainTextEdit *textEdit = qobject_cast<QPlainTextEdit*>(editor->widget())) {
            if (m_currentTextEdit == textEdit) {
                setCurrentDocumentWidget(0);
            }
            if (m_documentHash.contains(textEdit)) {
                if (debug)
                    qDebug() << Q_FUNC_INFO << editor->file()->fileName();
                DocumentWidget *document = m_documentHash.take(textEdit);
                m_documentWidgetStack->removeWidget(document);
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

DesignMode *DesignModeWidget::designMode() const
{
    return m_designMode;
}

void DesignModeWidget::undo()
{
    if (m_currentDocumentWidget)
        m_currentDocumentWidget->document()->undo();
}

void DesignModeWidget::redo()
{
    if (m_currentDocumentWidget)
        m_currentDocumentWidget->document()->redo();
}

void DesignModeWidget::deleteSelected()
{
    if (m_currentDocumentWidget)
        m_currentDocumentWidget->document()->deleteSelected();
}

void DesignModeWidget::cutSelected()
{
    if (m_currentDocumentWidget)
        m_currentDocumentWidget->document()->cutSelected();
}

void DesignModeWidget::copySelected()
{
    if (m_currentDocumentWidget)
        m_currentDocumentWidget->document()->copySelected();
}

void DesignModeWidget::paste()
{
    if (m_currentDocumentWidget)
        m_currentDocumentWidget->document()->paste();
}

void DesignModeWidget::undoAvailable(bool isAvailable)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    if (m_currentDocumentWidget &&
        m_currentDocumentWidget->document() == documentController) {
        m_undoAction->setEnabled(isAvailable);
    }
}

void DesignModeWidget::redoAvailable(bool isAvailable)
{
    DesignDocumentController *documentController = qobject_cast<DesignDocumentController*>(sender());
    if (m_currentDocumentWidget &&
        m_currentDocumentWidget->document() == documentController) {
        m_redoAction->setEnabled(isAvailable);
    }
}

void DesignModeWidget::setCurrentDocumentWidget(DocumentWidget *newDocumentWidget)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << newDocumentWidget;
    if (m_currentDocumentWidget) {
        m_currentDocumentWidget->setAutoSynchronization(false);
        m_currentDocumentWidget->saveSettings();
    }

    m_currentDocumentWidget = newDocumentWidget;

    if (m_currentDocumentWidget) {
        m_currentDocumentWidget->setAutoSynchronization(true);
        m_documentWidgetStack->setCurrentWidget(m_currentDocumentWidget);
        m_currentDocumentWidget->readSettings();
        m_undoAction->setEnabled(m_currentDocumentWidget->document()->isUndoAvailable());
        m_redoAction->setEnabled(m_currentDocumentWidget->document()->isRedoAvailable());
    } else {
        m_documentWidgetStack->setCurrentIndex(0);
        m_undoAction->setEnabled(false);
        m_redoAction->setEnabled(false);
    }
}

} // namespace Internal
} // namespace Designer
