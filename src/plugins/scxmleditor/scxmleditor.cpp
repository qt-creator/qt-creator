// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mainwidget.h"
#include "scxmleditorconstants.h"
#include "scxmleditordocument.h"
#include "scxmleditortr.h"
#include "scxmltexteditor.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/designmode.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/outputpane.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/fsengine/fileiconprovider.h>
#include <utils/icon.h>
#include <utils/infobar.h>
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QGuiApplication>
#include <QStackedWidget>
#include <QToolBar>
#include <QUndoGroup>
#include <QVBoxLayout>

using namespace Core;
using namespace ScxmlEditor::Common;
using namespace ScxmlEditor::PluginInterface;
using namespace Utils;

namespace ScxmlEditor::Internal {

class ScxmlEditorStack final : public QStackedWidget
{
public:
    ScxmlEditorStack() { setObjectName("ScxmlEditorStack"); }

    void add(ScxmlTextEditor *editor, QWidget *widget)
    {
        connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeAboutToChange,
                this, &ScxmlEditorStack::modeAboutToChange);

        m_editors.append(editor);
        addWidget(widget);
        connect(editor, &ScxmlTextEditor::destroyed,
                this, &ScxmlEditorStack::removeScxmlTextEditor);
    }

    QWidget *widgetForEditor(ScxmlTextEditor *xmlEditor)
    {
        const int i = m_editors.indexOf(xmlEditor);
        QTC_ASSERT(i >= 0, return nullptr);

        return widget(i);
    }

    void removeScxmlTextEditor(QObject *xmlEditor)
    {
        const int i = m_editors.indexOf(xmlEditor);
        QTC_ASSERT(i >= 0, return);

        QWidget *widget = this->widget(i);
        if (widget) {
            removeWidget(widget);
            widget->deleteLater();
        }
        m_editors.removeAt(i);
    }

    bool setVisibleEditor(Core::IEditor *xmlEditor)
    {
        const int i = m_editors.indexOf(xmlEditor);
        QTC_ASSERT(i >= 0, return false);

        if (i != currentIndex())
            setCurrentIndex(i);

        return true;
    }

private:
    void modeAboutToChange(Utils::Id m)
    {
        // Sync the editor when entering edit mode
        if (m == Core::Constants::MODE_EDIT) {
            for (auto editor: std::as_const(m_editors))
                if (auto document = qobject_cast<ScxmlEditorDocument*>(editor->textDocument()))
                    document->syncXmlFromDesignWidget();
        }
    }

    QList<ScxmlTextEditor*> m_editors;
};

class ScxmlTextEditorWidget : public TextEditor::TextEditorWidget
{
public:
    ScxmlTextEditorWidget() = default;

    void finalizeInitialization() override
    {
        setReadOnly(true);
    }
};

class ScxmlTextEditorFactory : public TextEditor::TextEditorFactory
{
public:
    ScxmlTextEditorFactory()
    {
        setId(ScxmlEditor::Constants::K_SCXML_EDITOR_ID);
        setEditorCreator([] { return new ScxmlTextEditor; });
        setEditorWidgetCreator([] { return new ScxmlTextEditorWidget; });
        setUseGenericHighlighter(true);
        setDuplicatedSupported(false);
    }

    ScxmlTextEditor *create(ScxmlEditor::Common::MainWidget *designWidget)
    {
        setDocumentCreator([designWidget] { return new ScxmlEditorDocument(designWidget); });
        return qobject_cast<ScxmlTextEditor*>(createEditor());
    }
};

class ScxmlEditorData : public QObject
{
public:
    ScxmlEditorData();
    ~ScxmlEditorData() override;

    void fullInit();
    IEditor *createEditor();

private:
    void updateToolBar();
    QWidget *createModeWidget();
    EditorToolBar *createMainToolBar();

    Context m_contexts;
    QWidget *m_modeWidget = nullptr;
    ScxmlEditorStack *m_widgetStack = nullptr;
    QToolBar *m_widgetToolBar = nullptr;
    EditorToolBar *m_mainToolBar = nullptr;
    QUndoGroup *m_undoGroup = nullptr;
    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;

    ScxmlTextEditorFactory *m_xmlEditorFactory = nullptr;
};

ScxmlEditorData::ScxmlEditorData()
{
    m_contexts.add(ScxmlEditor::Constants::C_SCXMLEDITOR);

    QObject::connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
                     this, [this](IEditor *editor) {
        if (editor && editor->document()->id() == Constants::K_SCXML_EDITOR_ID) {
            auto xmlEditor = qobject_cast<ScxmlTextEditor*>(editor);
            QTC_ASSERT(xmlEditor, return );
            QWidget *dw = m_widgetStack->widgetForEditor(xmlEditor);
            QTC_ASSERT(dw, return );
            m_widgetStack->setVisibleEditor(xmlEditor);
            m_mainToolBar->setCurrentEditor(xmlEditor);
            updateToolBar();
            auto designWidget = static_cast<MainWidget*>(m_widgetStack->currentWidget());
            if (designWidget)
                designWidget->refresh();
        }
    });

    m_xmlEditorFactory = new ScxmlTextEditorFactory;
}

ScxmlEditorData::~ScxmlEditorData()
{
    if (m_modeWidget) {
        DesignMode::unregisterDesignWidget(m_modeWidget);
        delete m_modeWidget;
        m_modeWidget = nullptr;
    }

    delete m_xmlEditorFactory;
}

void ScxmlEditorData::fullInit()
{
    // Create widget-stack, toolbar, mainToolbar and whole design-mode widget
    m_widgetStack = new ScxmlEditorStack;
    m_widgetToolBar = new QToolBar;
    m_mainToolBar = createMainToolBar();
    m_modeWidget = createModeWidget();

    // Create undo/redo group/actions
    m_undoGroup = new QUndoGroup(m_widgetToolBar);
    m_undoAction = m_undoGroup->createUndoAction(m_widgetToolBar);
    m_undoAction->setIcon(Utils::Icons::UNDO_TOOLBAR.icon());
    m_undoAction->setToolTip(Tr::tr("Undo (Ctrl + Z)"));

    m_redoAction = m_undoGroup->createRedoAction(m_widgetToolBar);
    m_redoAction->setIcon(Utils::Icons::REDO_TOOLBAR.icon());
    m_redoAction->setToolTip(Tr::tr("Redo (Ctrl + Y)"));

    ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, m_contexts);
    ActionManager::registerAction(m_redoAction, Core::Constants::REDO, m_contexts);

    Context scxmlContexts = m_contexts;
    scxmlContexts.add(Core::Constants::C_EDITORMANAGER);
    auto context = new IContext(this);
    context->setContext(scxmlContexts);
    context->setWidget(m_modeWidget);
    ICore::addContextObject(context);

    DesignMode::registerDesignWidget(m_modeWidget, QStringList(Utils::Constants::SCXML_MIMETYPE), m_contexts);
}

IEditor *ScxmlEditorData::createEditor()
{
    auto designWidget = new MainWidget;
    ScxmlTextEditor *xmlEditor = m_xmlEditorFactory->create(designWidget);

    m_undoGroup->addStack(designWidget->undoStack());
    m_widgetStack->add(xmlEditor, designWidget);
    m_mainToolBar->addEditor(xmlEditor);

    if (xmlEditor) {
        Utils::InfoBarEntry info(Id(Constants::INFO_READ_ONLY),
                                 Tr::tr("This file can only be edited in <b>Design</b> mode."));
        info.addCustomButton(Tr::tr("Switch Mode"),
                             [] { ModeManager::activateMode(Core::Constants::MODE_DESIGN); });
        xmlEditor->document()->infoBar()->addInfo(info);
    }

    return xmlEditor;
}

void ScxmlEditorData::updateToolBar()
{
    auto designWidget = static_cast<MainWidget*>(m_widgetStack->currentWidget());
    if (designWidget && m_widgetToolBar) {
        m_undoGroup->setActiveStack(designWidget->undoStack());
        m_widgetToolBar->clear();
        m_widgetToolBar->addAction(m_undoAction);
        m_widgetToolBar->addAction(m_redoAction);
        m_widgetToolBar->addSeparator();
        m_widgetToolBar->addAction(designWidget->action(ActionCopy));
        m_widgetToolBar->addAction(designWidget->action(ActionCut));
        m_widgetToolBar->addAction(designWidget->action(ActionPaste));
        m_widgetToolBar->addAction(designWidget->action(ActionScreenshot));
        m_widgetToolBar->addAction(designWidget->action(ActionExportToImage));
        m_widgetToolBar->addAction(designWidget->action(ActionFullNamespace));
        m_widgetToolBar->addSeparator();
        m_widgetToolBar->addAction(designWidget->action(ActionZoomIn));
        m_widgetToolBar->addAction(designWidget->action(ActionZoomOut));
        m_widgetToolBar->addAction(designWidget->action(ActionPan));
        m_widgetToolBar->addAction(designWidget->action(ActionFitToView));
        m_widgetToolBar->addSeparator();
        m_widgetToolBar->addWidget(designWidget->toolButton(ToolButtonAdjustment));
        m_widgetToolBar->addWidget(designWidget->toolButton(ToolButtonAlignment));
        m_widgetToolBar->addWidget(designWidget->toolButton(ToolButtonStateColor));
        m_widgetToolBar->addWidget(designWidget->toolButton(ToolButtonFontColor));
        m_widgetToolBar->addWidget(designWidget->toolButton(ToolButtonColorTheme));
        m_widgetToolBar->addSeparator();
        m_widgetToolBar->addAction(designWidget->action(ActionMagnifier));
        m_widgetToolBar->addAction(designWidget->action(ActionNavigator));
        m_widgetToolBar->addSeparator();
        m_widgetToolBar->addAction(designWidget->action(ActionStatistics));
    }
}

EditorToolBar *ScxmlEditorData::createMainToolBar()
{
    auto toolBar = new EditorToolBar;
    toolBar->setToolbarCreationFlags(EditorToolBar::FlagsStandalone);
    toolBar->setNavigationVisible(false);
    toolBar->addCenterToolBar(m_widgetToolBar);

    return toolBar;
}

QWidget *ScxmlEditorData::createModeWidget()
{
    auto widget = new QWidget;

    widget->setObjectName("ScxmlEditorDesignModeWidget");
    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_mainToolBar);
    // Avoid mode switch to 'Edit' mode when the application started by
    // 'Run' in 'Design' mode emits output.
    auto splitter = new MiniSplitter(Qt::Vertical);
    splitter->addWidget(m_widgetStack);
    auto outputPane = new OutputPanePlaceHolder(Core::Constants::MODE_DESIGN, splitter);
    outputPane->setObjectName("DesignerOutputPanePlaceHolder");
    splitter->addWidget(outputPane);
    layout->addWidget(splitter);
    widget->setLayout(layout);

    return widget;
}

class ScxmlEditorFactory final : public QObject, public Core::IEditorFactory
{
public:
    ScxmlEditorFactory(QObject *guard)
        : QObject(guard)
    {
        setId(Constants::K_SCXML_EDITOR_ID);
        setDisplayName(::Core::Tr::tr(Constants::C_SCXMLEDITOR_DISPLAY_NAME));
        addMimeType(Utils::Constants::SCXML_MIMETYPE);

        Utils::FileIconProvider::registerIconOverlayForSuffix(":/projectexplorer/images/fileoverlay_scxml.png", "scxml");

        setEditorCreator([this] {
            if (!m_editorData) {
                m_editorData = new ScxmlEditorData;
                QGuiApplication::setOverrideCursor(Qt::WaitCursor);
                m_editorData->fullInit();
                QGuiApplication::restoreOverrideCursor();
            }
            return m_editorData->createEditor();
        });
    }
    ~ScxmlEditorFactory()
    {
        delete m_editorData;
    }

private:
    ScxmlEditorData *m_editorData = nullptr;
};

void setupScxmlEditor(QObject *guard)
{
    (void) new ScxmlEditorFactory(guard);
}

} // ScxmlEditor::Internal
