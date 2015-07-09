/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmldesignerplugin.h"
#include "exception.h"
#include "qmldesignerconstants.h"
#include "designmodewidget.h"
#include "settingspage.h"
#include "designmodecontext.h"

#include <qmljseditor/qmljseditorconstants.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginspec.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QTimer>
#include <QCoreApplication>
#include <qplugin.h>
#include <QDebug>
#include <QProcessEnvironment>

namespace QmlDesigner {

class QmlDesignerPluginData {
public:
    ViewManager viewManager;
    DocumentManager documentManager;
    ShortCutManager shortCutManager;

    Internal::DesignModeWidget *mainWidget;

    QmlDesigner::PluginManager pluginManager;
    DesignerSettings settings;
    Internal::DesignModeContext *context;
};

QmlDesignerPlugin *QmlDesignerPlugin::m_instance = 0;

static bool isInDesignerMode()
{
    return Core::ModeManager::currentMode() == Core::DesignMode::instance();
}

bool shouldAssertInException()
{
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    return !processEnvironment.value("QMLDESIGNER_ASSERT_ON_EXCEPTION").isEmpty();
}

QmlDesignerPlugin::QmlDesignerPlugin()
    : data(0)
{
    m_instance = this;
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
    if (data) {
        Core::DesignMode::instance()->unregisterDesignWidget(data->mainWidget);
        Core::ICore::removeContextObject(data->context);
        data->context = 0;
    }
    delete data;
    m_instance = 0;
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
bool QmlDesignerPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage/* = 0*/) // =0;
{
    if (errorMessage)
        errorMessage->clear();

    data = new QmlDesignerPluginData;

    data->settings.fromSettings(Core::ICore::settings());

    const Core::Context switchContext(QmlDesigner::Constants::C_QMLDESIGNER,
        QmlJSEditor::Constants::C_QMLJSEDITOR_ID);

    QAction *switchAction = new QAction(tr("Switch Text/Design"), this);
    Core::Command *command = Core::ActionManager::registerAction(
                switchAction, QmlDesigner::Constants::SWITCH_TEXT_DESIGN, switchContext);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F4));



    const QString pluginPath = Utils::HostOsInfo::isMacHost()
            ? QString(QCoreApplication::applicationDirPath() + "/../PlugIns/QmlDesigner")
            : QString(QCoreApplication::applicationDirPath() + "/../"
                      + QLatin1String(IDE_LIBRARY_BASENAME) + "/qtcreator/plugins/qmldesigner");
    data->pluginManager.setPluginPaths(QStringList() << pluginPath);

    createDesignModeWidget();
    connect(switchAction, SIGNAL(triggered()), this, SLOT(switchTextDesign()));

    addAutoReleasedObject(new Internal::SettingsPage);

    return true;
}

void QmlDesignerPlugin::extensionsInitialized()
{
    QStringList mimeTypes;
    mimeTypes.append(QmlJSTools::Constants::QML_MIMETYPE);
    mimeTypes.append(QmlJSTools::Constants::QMLUI_MIMETYPE);

    Core::DesignMode::instance()->registerDesignWidget(data->mainWidget, mimeTypes, data->context->context());
    connect(Core::DesignMode::instance(),
            SIGNAL(actionsUpdated(Core::IEditor*)),
            &data->shortCutManager,
            SLOT(updateActions(Core::IEditor*)));
}

void QmlDesignerPlugin::createDesignModeWidget()
{
    data->mainWidget = new Internal::DesignModeWidget;

    data->context = new Internal::DesignModeContext(data->mainWidget);
    Core::ICore::addContextObject(data->context);
    Core::Context qmlDesignerMainContext(Constants::C_QMLDESIGNER);
    Core::Context qmlDesignerFormEditorContext(Constants::C_QMLFORMEDITOR);
    Core::Context qmlDesignerNavigatorContext(Constants::C_QMLNAVIGATOR);

    data->context->context().add(qmlDesignerMainContext);
    data->context->context().add(qmlDesignerFormEditorContext);
    data->context->context().add(qmlDesignerNavigatorContext);
    data->context->context().add(ProjectExplorer::Constants::LANG_QMLJS);

    data->shortCutManager.registerActions(qmlDesignerMainContext, qmlDesignerFormEditorContext, qmlDesignerNavigatorContext);

    connect(Core::EditorManager::instance(),
            SIGNAL(currentEditorChanged(Core::IEditor*)),
            this,
            SLOT(onCurrentEditorChanged(Core::IEditor*)));

    connect(Core::EditorManager::instance(),
            SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this,
            SLOT(onTextEditorsClosed(QList<Core::IEditor*>)));

//    connect(Core::ICore::editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)),
//            &data->documentManager, SLOT(currentTextEditorChanged(Core::IEditor*)));

//    connect(Core::ICore::instance(), SIGNAL(contextChanged(Core::IContext*,Core::Context)),
//            this, SLOT(contextChanged(Core::IContext*,Core::Context)));

    connect(Core::ModeManager::instance(),
            SIGNAL(currentModeChanged(Core::IMode*,Core::IMode*)),
            SLOT(onCurrentModeChanged(Core::IMode*,Core::IMode*)));

}

void QmlDesignerPlugin::showDesigner()
{
    QTC_ASSERT(!data->documentManager.hasCurrentDesignDocument(), return);

    data->shortCutManager.disconnectUndoActions(currentDesignDocument());

    data->documentManager.setCurrentDesignDocument(Core::EditorManager::currentEditor());

    data->shortCutManager.connectUndoActions(currentDesignDocument());

    data->mainWidget->initialize();

    if (data->documentManager.hasCurrentDesignDocument()) {
        activateAutoSynchronization();
        data->shortCutManager.updateActions(currentDesignDocument()->textEditor());
        data->viewManager.pushFileOnCrumbleBar(data->documentManager.currentDesignDocument()->fileName().fileName());
    }

    data->shortCutManager.updateUndoActions(currentDesignDocument());
}

void QmlDesignerPlugin::hideDesigner()
{
    if (currentDesignDocument()
            && currentModel()
            && !currentDesignDocument()->hasQmlSyntaxErrors())
        jumpTextCursorToSelectedModelNode();


    if (data->documentManager.hasCurrentDesignDocument()) {
        deactivateAutoSynchronization();
        data->mainWidget->saveSettings();
    }

    data->shortCutManager.disconnectUndoActions(currentDesignDocument());

    data->documentManager.setCurrentDesignDocument(0);

    data->shortCutManager.updateUndoActions(0);
}

void QmlDesignerPlugin::changeEditor()
{
    if (data->documentManager.hasCurrentDesignDocument()) {
        deactivateAutoSynchronization();
        data->mainWidget->saveSettings();
    }

    data->shortCutManager.disconnectUndoActions(currentDesignDocument());

    data->documentManager.setCurrentDesignDocument(Core::EditorManager::currentEditor());

    data->mainWidget->initialize();

    data->shortCutManager.connectUndoActions(currentDesignDocument());

    if (data->documentManager.hasCurrentDesignDocument()) {
        activateAutoSynchronization();
        data->viewManager.pushFileOnCrumbleBar(data->documentManager.currentDesignDocument()->fileName().fileName());
        data->viewManager.setComponentViewToMaster();
    }

    data->shortCutManager.updateUndoActions(currentDesignDocument());
}

void QmlDesignerPlugin::jumpTextCursorToSelectedModelNode()
{
    // visual editor -> text editor
    ModelNode selectedNode;
    if (!rewriterView()->selectedModelNodes().isEmpty())
        selectedNode = rewriterView()->selectedModelNodes().first();

    if (selectedNode.isValid()) {
        const int nodeOffset = rewriterView()->nodeOffset(selectedNode);
        if (nodeOffset > 0) {
            const ModelNode currentSelectedNode
                    = rewriterView()->nodeAtTextCursorPosition(currentDesignDocument()->plainTextEdit()->textCursor().position());
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
    const int cursorPosition = currentDesignDocument()->plainTextEdit()->textCursor().position();
    ModelNode modelNode = rewriterView()->nodeAtTextCursorPosition(cursorPosition);
    if (modelNode.isValid())
        rewriterView()->setSelectedModelNode(modelNode);
}

void QmlDesignerPlugin::activateAutoSynchronization()
{
    // text editor -> visual editor
    if (!currentDesignDocument()->isDocumentLoaded())
        currentDesignDocument()->loadDocument(currentDesignDocument()->plainTextEdit());

    currentDesignDocument()->updateActiveQtVersion();
    currentDesignDocument()->attachRewriterToModel();

    resetModelSelection();

    viewManager().attachComponentView();
    viewManager().attachViewsExceptRewriterAndComponetView();

    QList<RewriterError> errors = currentDesignDocument()->qmlSyntaxErrors();
    if (errors.isEmpty()) {
        selectModelNodeUnderTextCursor();
        data->mainWidget->enableWidgets();
        data->mainWidget->setupNavigatorHistory(currentDesignDocument()->textEditor());
    } else {
        data->mainWidget->disableWidgets();
        data->mainWidget->showErrorMessage(errors);
    }

    currentDesignDocument()->updateSubcomponentManager();

    connect(rewriterView(),
            SIGNAL(errorsChanged(QList<RewriterError>)),
            data->mainWidget,
            SLOT(updateErrorStatus(QList<RewriterError>)));
}

void QmlDesignerPlugin::deactivateAutoSynchronization()
{
    viewManager().detachViewsExceptRewriterAndComponetView();
    viewManager().detachComponentView();
    viewManager().detachRewriterView();
    documentManager().currentDesignDocument()->resetToDocumentModel();

    disconnect(rewriterView(),
               SIGNAL(errorsChanged(QList<RewriterError>)),
               data->mainWidget,
               SLOT(updateErrorStatus(QList<RewriterError>)));

}

void QmlDesignerPlugin::resetModelSelection()
{
    if (rewriterView() && currentModel())
        rewriterView()->setSelectedModelNodes(QList<ModelNode>());
}

RewriterView *QmlDesignerPlugin::rewriterView() const
{
    return currentDesignDocument()->rewriterView();
}

Model *QmlDesignerPlugin::currentModel() const
{
    return currentDesignDocument()->currentModel();
}

static bool checkIfEditorIsQtQuick(Core::IEditor *editor)
{
    if (editor)
    if (editor && editor->document()->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID) {
        QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
        QmlJS::Document::Ptr document = modelManager->ensuredGetDocumentForPath(editor->document()->filePath().toString());
        if (!document.isNull())
            return document->language() == QmlJS::Dialect::QmlQtQuick1
                    || document->language() == QmlJS::Dialect::QmlQtQuick2
                    || document->language() == QmlJS::Dialect::QmlQtQuick2Ui
                    || document->language() == QmlJS::Dialect::Qml;
    }

    return false;
}

void QmlDesignerPlugin::onCurrentEditorChanged(Core::IEditor *editor)
{
    if (data
            && checkIfEditorIsQtQuick(editor)
            && isInDesignerMode())
    {
        data->shortCutManager.updateActions(editor);
        changeEditor();
    }
}

static bool isDesignerMode(Core::IMode *mode)
{
    return mode == Core::DesignMode::instance();
}



static bool documentIsAlreadyOpen(DesignDocument *designDocument, Core::IEditor *editor, Core::IMode *newMode)
{
    return designDocument
            && editor == designDocument->editor()
            && isDesignerMode(newMode);
}

void QmlDesignerPlugin::onCurrentModeChanged(Core::IMode *newMode, Core::IMode *oldMode)
{
    if (data
            && Core::EditorManager::currentEditor()
            && checkIfEditorIsQtQuick(Core::EditorManager::currentEditor())
            && !documentIsAlreadyOpen(currentDesignDocument(), Core::EditorManager::currentEditor(), newMode)) {

        if (!isDesignerMode(newMode) && isDesignerMode(oldMode))
            hideDesigner();
        else  if (Core::EditorManager::currentEditor()
                  && isDesignerMode(newMode))
            showDesigner();
        else if (currentDesignDocument())
            hideDesigner();
    }
}

DesignDocument *QmlDesignerPlugin::currentDesignDocument() const
{
    if (data)
        return data->documentManager.currentDesignDocument();

    return 0;
}

Internal::DesignModeWidget *QmlDesignerPlugin::mainWidget() const
{
    if (data)
        return data->mainWidget;

    return 0;
}

void QmlDesignerPlugin::switchToTextModeDeferred()
{
    QTimer::singleShot(0, this, SLOT(switschToTextMode()));
}

void QmlDesignerPlugin::onTextEditorsClosed(QList<Core::IEditor*> editors)
{
    if (data) {
        if (data->documentManager.hasCurrentDesignDocument()
                && editors.contains(data->documentManager.currentDesignDocument()->textEditor()))
            hideDesigner();

        data->documentManager.removeEditors(editors);
    }
}

QmlDesignerPlugin *QmlDesignerPlugin::instance()
{
    return m_instance;
}

DocumentManager &QmlDesignerPlugin::documentManager()
{
    return data->documentManager;
}

const DocumentManager &QmlDesignerPlugin::documentManager() const
{
    return data->documentManager;
}

ViewManager &QmlDesignerPlugin::viewManager()
{
    return data->viewManager;
}

const ViewManager &QmlDesignerPlugin::viewManager() const
{
    return data->viewManager;
}

DesignerActionManager &QmlDesignerPlugin::designerActionManager()
{
    return data->viewManager.designerActionManager();
}

const DesignerActionManager &QmlDesignerPlugin::designerActionManager() const
{
    return data->viewManager.designerActionManager();
}

void QmlDesignerPlugin::switchTextDesign()
{
    if (Core::ModeManager::currentMode()->id() == Core::Constants::MODE_EDIT) {
        Core::IEditor *editor = Core::EditorManager::currentEditor();
        if (checkIfEditorIsQtQuick(editor))
            Core::ModeManager::activateMode(Core::Constants::MODE_DESIGN);
    } else if (Core::ModeManager::currentMode()->id() == Core::Constants::MODE_DESIGN) {
        Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
    }
}

void QmlDesignerPlugin::switschToTextMode()
{
    Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
}

DesignerSettings QmlDesignerPlugin::settings()
{
    data->settings.fromSettings(Core::ICore::settings());
    return data->settings;
}

void QmlDesignerPlugin::setSettings(const DesignerSettings &s)
{
    if (s != data->settings) {
        data->settings = s;
        data->settings.toSettings(Core::ICore::settings());
    }
}

}
