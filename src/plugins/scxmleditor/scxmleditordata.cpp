/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "scxmleditordata.h"
#include "mainwidget.h"
#include "scxmlcontext.h"
#include "scxmleditorconstants.h"
#include "scxmleditordocument.h"
#include "scxmleditorstack.h"
#include "scxmltexteditor.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/infobar.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/outputpane.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>
#include <utils/icon.h>
#include <utils/utilsicons.h>

#include <QVBoxLayout>

using namespace ScxmlEditor::Common;
using namespace ScxmlEditor::PluginInterface;

namespace ScxmlEditor {

namespace Internal {

class ScxmlTextEditorWidget : public TextEditor::TextEditorWidget
{
public:
    ScxmlTextEditorWidget()
    {
    }

    void finalizeInitialization()
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
        setEditorCreator([]() { return new ScxmlTextEditor; });
        setEditorWidgetCreator([]() { return new ScxmlTextEditorWidget; });
        setUseGenericHighlighter(true);
        setDuplicatedSupported(false);
    }

    ScxmlTextEditor *create(ScxmlEditor::Common::MainWidget *designWidget)
    {
        setDocumentCreator([designWidget]() { return new ScxmlEditorDocument(designWidget); });
        return qobject_cast<ScxmlTextEditor*>(createEditor());
    }
};

ScxmlEditorData::ScxmlEditorData(QObject *parent)
    : QObject(parent)
{
    m_contexts.add(ScxmlEditor::Constants::C_SCXMLEDITOR);

    QObject::connect(EditorManager::instance(), &EditorManager::currentEditorChanged, [this](IEditor *editor) {
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
    if (m_context)
        ICore::removeContextObject(m_context);

    if (m_modeWidget) {
        m_designMode->unregisterDesignWidget(m_modeWidget);
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
    m_designMode = DesignMode::instance();
    m_modeWidget = createModeWidget();

    // Create undo/redo group/actions
    m_undoGroup = new QUndoGroup(m_widgetToolBar);
    m_undoAction = m_undoGroup->createUndoAction(m_widgetToolBar);
    m_undoAction->setIcon(Utils::Icons::UNDO_TOOLBAR.icon());
    m_undoAction->setToolTip(tr("Undo (Ctrl + Z)"));

    m_redoAction = m_undoGroup->createRedoAction(m_widgetToolBar);
    m_redoAction->setIcon(Utils::Icons::REDO_TOOLBAR.icon());
    m_redoAction->setToolTip(tr("Redo (Ctrl + Y)"));

    ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, m_contexts);
    ActionManager::registerAction(m_redoAction, Core::Constants::REDO, m_contexts);

    Context scxmlContexts = m_contexts;
    scxmlContexts.add(Core::Constants::C_EDITORMANAGER);
    m_context = new ScxmlContext(scxmlContexts, m_modeWidget, this);
    ICore::addContextObject(m_context);

    m_designMode->registerDesignWidget(m_modeWidget, QStringList(QLatin1String(ProjectExplorer::Constants::SCXML_MIMETYPE)), m_contexts);
}

IEditor *ScxmlEditorData::createEditor()
{
    auto designWidget = new MainWidget;
    ScxmlTextEditor *xmlEditor = m_xmlEditorFactory->create(designWidget);

    m_undoGroup->addStack(designWidget->undoStack());
    m_widgetStack->add(xmlEditor, designWidget);
    m_mainToolBar->addEditor(xmlEditor);

    if (xmlEditor) {
        InfoBarEntry info(Id(Constants::INFO_READ_ONLY),
            tr("This file can only be edited in <b>Design</b> mode."));
        info.setCustomButtonInfo(tr("Switch Mode"), []() { ModeManager::activateMode(Core::Constants::MODE_DESIGN); });
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
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_mainToolBar);
    // Avoid mode switch to 'Edit' mode when the application started by
    // 'Run' in 'Design' mode emits output.
    auto splitter = new MiniSplitter(Qt::Vertical);
    splitter->addWidget(m_widgetStack);
    auto outputPane = new OutputPanePlaceHolder(m_designMode->id(), splitter);
    outputPane->setObjectName("DesignerOutputPanePlaceHolder");
    splitter->addWidget(outputPane);
    layout->addWidget(splitter);
    widget->setLayout(layout);

    return widget;
}

} // namespace Internal
} // namespace ScxmlEditor
