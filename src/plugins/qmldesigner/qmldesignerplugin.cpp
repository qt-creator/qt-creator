/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "exception.h"
#include "qmldesignerplugin.h"
#include "qmldesignerconstants.h"
#include "pluginmanager.h"
#include "designmodewidget.h"
#include "settingspage.h"
#include "designmodecontext.h"

#include <qmljseditor/qmljseditorconstants.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/openeditorsmodel.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QAction>

#include <QFileInfo>
#include <QCoreApplication>
#include <qplugin.h>
#include <QDebug>
#include <QProcessEnvironment>

namespace QmlDesigner {

QmlDesignerPlugin *QmlDesignerPlugin::m_instance = 0;

static bool isQmlFile(Core::IEditor *editor)
{
    return editor && editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID;
}

static bool isInDesignerMode()
{
    return Core::ModeManager::currentMode() == Core::DesignMode::instance();
}

bool shouldAssertInException()
{
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    return !processEnvironment.value("QMLDESIGNER_ASSERT_ON_EXCEPTION").isEmpty();
}

QmlDesignerPlugin::QmlDesignerPlugin() :
    m_isActive(false)
{

    // Exceptions should never ever assert: they are handled in a number of
    // places where it is actually VALID AND EXPECTED BEHAVIOUR to get an
    // exception.
    // If you still want to see exactly where the exception originally
    // occurred, then you have various ways to do this:
    //  1. set a breakpoint on the constructor of the exception
    //  2. in gdb: "catch throw" or "catch throw Exception"
    //  3. set a breakpoint on __raise_exception()
    // And with gdb, you can even do this from your ~/.gdbinit file.
    // DnD is not working with gdb so this is still needed to get a good stacktrace

    Exception::setShouldAssert(shouldAssertInException());
}

QmlDesignerPlugin::~QmlDesignerPlugin()
{
    Core::ICore::removeContextObject(m_context);
    m_context = 0;
    m_instance = 0;
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
bool QmlDesignerPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage/* = 0*/) // =0;
{
    const Core::Context switchContext(QmlDesigner::Constants::C_QMLDESIGNER,
        QmlJSEditor::Constants::C_QMLJSEDITOR_ID);

    QAction *switchAction = new QAction(tr("Switch Text/Design"), this);
    Core::Command *command = Core::ActionManager::registerAction(
                switchAction, QmlDesigner::Constants::SWITCH_TEXT_DESIGN, switchContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F4));

    m_instance = this;

    const QString pluginPath = Utils::HostOsInfo::isMacHost()
            ? QString(QCoreApplication::applicationDirPath() + "/../PlugIns/QmlDesigner")
            : QString(QCoreApplication::applicationDirPath() + "/../"
                      + QLatin1String(IDE_LIBRARY_BASENAME) + "/qtcreator/qmldesigner");
    m_pluginManager.setPluginPaths(QStringList() << pluginPath);

    createDesignModeWidget();
    connect(switchAction, SIGNAL(triggered()), this, SLOT(switchTextDesign()));

    addAutoReleasedObject(new Internal::SettingsPage);


    m_settings.fromSettings(Core::ICore::settings());

    errorMessage->clear();

    return true;
}

void QmlDesignerPlugin::createDesignModeWidget()
{
    m_mainWidget = new Internal::DesignModeWidget;

    m_context = new Internal::DesignModeContext(m_mainWidget);
    Core::ICore::addContextObject(m_context);
    Core::Context qmlDesignerMainContext(Constants::C_QMLDESIGNER);
    Core::Context qmlDesignerFormEditorContext(Constants::C_QMLFORMEDITOR);
    Core::Context qmlDesignerNavigatorContext(Constants::C_QMLNAVIGATOR);

    m_context->context().add(qmlDesignerMainContext);
    m_context->context().add(qmlDesignerFormEditorContext);
    m_context->context().add(qmlDesignerNavigatorContext);
    m_context->context().add(ProjectExplorer::Constants::LANG_QMLJS);

    m_shortCutManager.registerActions();

    connect(Core::ICore::editorManager(),
            SIGNAL(currentEditorChanged(Core::IEditor*)),
            this,
            SLOT(onCurrentEditorChanged(Core::IEditor*)));

    connect(Core::ICore::editorManager(),
            SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this,
            SLOT(onTextEditorsClosed(QList<Core::IEditor*>)));

//    connect(Core::ICore::editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)),
//            &m_documentManager, SLOT(currentTextEditorChanged(Core::IEditor*)));

//    connect(Core::ICore::instance(), SIGNAL(contextChanged(Core::IContext*,Core::Context)),
//            this, SLOT(contextChanged(Core::IContext*,Core::Context)));

    connect(Core::ModeManager::instance(),
            SIGNAL(currentModeChanged(Core::IMode*,Core::IMode*)),
            SLOT(onCurrentModeChanged(Core::IMode*,Core::IMode*)));

}

void QmlDesignerPlugin::showDesigner()
{
    Q_ASSERT(!m_documentManager.hasCurrentDesignDocument());

    m_shortCutManager.disconnectUndoActions(currentDesignDocument());

    m_documentManager.setCurrentDesignDocument(Core::EditorManager::currentEditor());

    m_shortCutManager.connectUndoActions(currentDesignDocument());

    m_mainWidget->initialize();

    if (m_documentManager.hasCurrentDesignDocument()) {
        activateAutoSynchronization();
        m_viewManager.pushFileOnCrambleBar(m_documentManager.currentDesignDocument()->fileName());
    }

    m_shortCutManager.updateUndoActions(currentDesignDocument());
}

void QmlDesignerPlugin::hideDesigner()
{
    if (currentDesignDocument()->currentModel()
            && !currentDesignDocument()->hasQmlSyntaxErrors())
        jumpTextCursorToSelectedModelNode();


    if (m_documentManager.hasCurrentDesignDocument()) {
        deactivateAutoSynchronization();
        m_mainWidget->saveSettings();
    }

    m_shortCutManager.disconnectUndoActions(currentDesignDocument());

    m_documentManager.setCurrentDesignDocument(0);

    m_shortCutManager.updateUndoActions(0);
}

void QmlDesignerPlugin::changeEditor()
{
    if (m_documentManager.hasCurrentDesignDocument()) {
        deactivateAutoSynchronization();
        m_mainWidget->saveSettings();
    }

    m_shortCutManager.disconnectUndoActions(currentDesignDocument());

    m_documentManager.setCurrentDesignDocument(Core::EditorManager::currentEditor());

    m_mainWidget->initialize();

    m_shortCutManager.connectUndoActions(currentDesignDocument());

    if (m_documentManager.hasCurrentDesignDocument()) {
        m_viewManager.pushFileOnCrambleBar(m_documentManager.currentDesignDocument()->fileName());
        activateAutoSynchronization();
    }

    m_shortCutManager.updateUndoActions(currentDesignDocument());
}

void QmlDesignerPlugin::jumpTextCursorToSelectedModelNode()
{
    // visual editor -> text editor
    ModelNode selectedNode;
    if (!currentDesignDocument()->rewriterView()->selectedModelNodes().isEmpty())
        selectedNode = currentDesignDocument()->rewriterView()->selectedModelNodes().first();

    if (selectedNode.isValid()) {
        const int nodeOffset = currentDesignDocument()->rewriterView()->nodeOffset(selectedNode);
        if (nodeOffset > 0) {
            const ModelNode currentSelectedNode
                    = currentDesignDocument()->rewriterView()->nodeAtTextCursorPosition(currentDesignDocument()->plainTextEdit()->textCursor().position());
            if (currentSelectedNode != selectedNode) {
                int line, column;
                currentDesignDocument()->textEditor()->convertPosition(nodeOffset, &line, &column);
                currentDesignDocument()->textEditor()->gotoLine(line, column);
            }
        }
    }
}

void QmlDesignerPlugin::selectModelNodeUnderTextCursor()
{
    const int cursorPos = currentDesignDocument()->plainTextEdit()->textCursor().position();
    ModelNode node = currentDesignDocument()->rewriterView()->nodeAtTextCursorPosition(cursorPos);
    if (currentDesignDocument()->rewriterView() && node.isValid()) {
        currentDesignDocument()->rewriterView()->setSelectedModelNodes(QList<ModelNode>() << node);
    }
}

void QmlDesignerPlugin::activateAutoSynchronization()
{
    // text editor -> visual editor
    if (!currentDesignDocument()->isDocumentLoaded()) {
        currentDesignDocument()->loadDocument(currentDesignDocument()->plainTextEdit());
    } else {
        currentDesignDocument()->activateCurrentModel();
    }

    resetModelSelection();


    QList<RewriterView::Error> errors = currentDesignDocument()->qmlSyntaxErrors();
    if (errors.isEmpty()) {
        viewManager().attachComponentView();
        viewManager().attachViewsExceptRewriterAndComponetView();
        selectModelNodeUnderTextCursor();
        m_mainWidget->enableWidgets();
    } else {
        m_mainWidget->disableWidgets();
        m_mainWidget->showErrorMessage(errors);
    }

    currentDesignDocument()->updateSubcomponentManager();

    connect(currentDesignDocument()->rewriterView(),
            SIGNAL(errorsChanged(QList<RewriterView::Error>)),
            m_mainWidget,
            SLOT(updateErrorStatus(QList<RewriterView::Error>)));
}

void QmlDesignerPlugin::deactivateAutoSynchronization()
{
    viewManager().detachViewsExceptRewriterAndComponetView();
    viewManager().detachComponentView();

    disconnect(currentDesignDocument()->rewriterView(),
               SIGNAL(errorsChanged(QList<RewriterView::Error>)),
               m_mainWidget,
               SLOT(updateErrorStatus(QList<RewriterView::Error>)));

}

void QmlDesignerPlugin::resetModelSelection()
{
    if (currentDesignDocument()->rewriterView() && currentDesignDocument()->currentModel())
        currentDesignDocument()->rewriterView()->setSelectedModelNodes(QList<ModelNode>());
}



void QmlDesignerPlugin::onCurrentEditorChanged(Core::IEditor *editor)
{
    if (editor
            && editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
            && isInDesignerMode())
    {
        m_shortCutManager.updateActions(editor);
        changeEditor();
    }
}

static bool isDesignerMode(Core::IMode *mode)
{
    return mode == Core::DesignMode::instance();
}

void QmlDesignerPlugin::onCurrentModeChanged(Core::IMode *newMode, Core::IMode *oldMode)
{
    if (!Core::EditorManager::currentEditor())
        return;

    if (Core::EditorManager::currentEditor()
            && Core::EditorManager::currentEditor()->id() != QmlJSEditor::Constants::C_QMLJSEDITOR_ID)
        return;

    if ((currentDesignDocument()
         && Core::EditorManager::currentEditor() == currentDesignDocument()->editor())
            && isDesignerMode(newMode))
        return;

    if (!isDesignerMode(newMode) && isDesignerMode(oldMode))
        hideDesigner();
    else  if (Core::EditorManager::currentEditor()
              && isDesignerMode(newMode)
              && isQmlFile(Core::EditorManager::currentEditor()))
        showDesigner();
    else if (currentDesignDocument())
        hideDesigner();

}

DesignDocument *QmlDesignerPlugin::currentDesignDocument() const
{
    return m_documentManager.currentDesignDocument();
}

Internal::DesignModeWidget *QmlDesignerPlugin::mainWidget() const
{
    return m_mainWidget;
}

void QmlDesignerPlugin::onTextEditorsClosed(QList<Core::IEditor*> editors)
{
    if (m_documentManager.hasCurrentDesignDocument()
            && editors.contains(m_documentManager.currentDesignDocument()->textEditor()))
        hideDesigner();

    m_documentManager.removeEditors(editors);
}

void QmlDesignerPlugin::extensionsInitialized()
{
    QStringList mimeTypes;
    mimeTypes.append("application/x-qml");

    Core::DesignMode::instance()->registerDesignWidget(m_mainWidget, mimeTypes, m_context->context());
    connect(Core::DesignMode::instance(),
            SIGNAL(actionsUpdated(Core::IEditor*)),
            &m_shortCutManager,
            SLOT(updateActions(Core::IEditor*)));
}

QmlDesignerPlugin *QmlDesignerPlugin::instance()
{
    return m_instance;
}

DocumentManager &QmlDesignerPlugin::documentManager()
{
    return m_documentManager;
}

const DocumentManager &QmlDesignerPlugin::documentManager() const
{
    return m_documentManager;
}

ViewManager &QmlDesignerPlugin::viewManager()
{
    return m_viewManager;
}

const ViewManager &QmlDesignerPlugin::viewManager() const
{
    return m_viewManager;
}

void QmlDesignerPlugin::switchTextDesign()
{
    if (Core::ModeManager::currentMode()->id() == Core::Constants::MODE_EDIT) {
        Core::IEditor *editor = Core::EditorManager::currentEditor();
        if (editor->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID)
            Core::ModeManager::activateMode(Core::Constants::MODE_DESIGN);
    } else if (Core::ModeManager::currentMode()->id() == Core::Constants::MODE_DESIGN) {
        Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
    }
}

DesignerSettings QmlDesignerPlugin::settings()
{
    m_settings.fromSettings(Core::ICore::settings());
    return m_settings;
}

void QmlDesignerPlugin::setSettings(const DesignerSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        m_settings.toSettings(Core::ICore::settings());
    }
}

}

Q_EXPORT_PLUGIN(QmlDesigner::QmlDesignerPlugin)
