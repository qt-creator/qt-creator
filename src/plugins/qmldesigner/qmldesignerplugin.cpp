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

#include "qmldesignerplugin.h"
#include "exception.h"
#include "qmldesignerconstants.h"
#include "designmodewidget.h"
#include "settingspage.h"
#include "designmodecontext.h"
#include "openuiqmlfiledialog.h"
#include "generateresource.h"
#include "nodeinstanceview.h"

#include <metainfo.h>
#include <connectionview.h>
#include <sourcetool/sourcetool.h>
#include <colortool/colortool.h>
#include <texttool/texttool.h>
#include <timelineeditor/timelineview.h>
#include <pathtool/pathtool.h>

#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljseditor/qmljseditordocument.h>

#include <qmljstools/qmljstoolsconstants.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginspec.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QTimer>
#include <QCoreApplication>
#include <qplugin.h>
#include <QDebug>
#include <QProcessEnvironment>
#include <QScreen>
#include <QWindow>

static Q_LOGGING_CATEGORY(qmldesignerLog, "qtc.qmldesigner", QtWarningMsg)

using namespace QmlDesigner::Internal;

namespace QmlDesigner {

namespace Internal {

class QtQuickDesignerFactory : public QmlJSEditor::QmlJSEditorFactory
{
public:
    QtQuickDesignerFactory();
};

QtQuickDesignerFactory::QtQuickDesignerFactory()
    : QmlJSEditorFactory(QmlJSEditor::Constants::C_QTQUICKDESIGNEREDITOR_ID)
{
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", "Qt Quick Designer"));

    addMimeType(QmlJSTools::Constants::QMLUI_MIMETYPE);
    setDocumentCreator([this]() {
        auto document = new QmlJSEditor::QmlJSEditorDocument(id());
        document->setIsDesignModePreferred(
                    QmlDesigner::DesignerSettings::getValue(
                        QmlDesigner::DesignerSettingsKey::ALWAYS_DESIGN_MODE).toBool());
        return document;
    });
}

} // namespace Internal

class QmlDesignerPluginPrivate
{
public:
    ViewManager viewManager;
    DocumentManager documentManager;
    ShortCutManager shortCutManager;
    SettingsPage settingsPage;
    DesignModeWidget mainWidget;
    DesignerSettings settings;
    DesignModeContext *context = nullptr;
    QtQuickDesignerFactory m_qtQuickDesignerFactory;
    bool blockEditorChange = false;
};

QmlDesignerPlugin *QmlDesignerPlugin::m_instance = nullptr;

static bool isInDesignerMode()
{
    return Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN;
}

static bool checkIfEditorIsQtQuick(Core::IEditor *editor)
{
    if (editor
        && (editor->document()->id() == QmlJSEditor::Constants::C_QMLJSEDITOR_ID
            || editor->document()->id() == QmlJSEditor::Constants::C_QTQUICKDESIGNEREDITOR_ID)) {
        QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
        QmlJS::Document::Ptr document = modelManager->ensuredGetDocumentForPath(editor->document()->filePath().toString());
        if (!document.isNull())
            return document->language() == QmlJS::Dialect::QmlQtQuick2
                    || document->language() == QmlJS::Dialect::QmlQtQuick2Ui
                    || document->language() == QmlJS::Dialect::Qml;

        if (Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN) {
            Core::AsynchronousMessageBox::warning(QmlDesignerPlugin::tr("Cannot Open Design Mode"),
                                                  QmlDesignerPlugin::tr("The QML file is not currently opened in a QML Editor."));
            Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
        }
    }

    return false;
}

static bool isDesignerMode(Core::Id mode)
{
    return mode == Core::Constants::MODE_DESIGN;
}

static bool documentIsAlreadyOpen(DesignDocument *designDocument, Core::IEditor *editor, Core::Id newMode)
{
    return designDocument
            && editor == designDocument->editor()
            && isDesignerMode(newMode);
}

static bool shouldAssertInException()
{
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    return !processEnvironment.value("QMLDESIGNER_ASSERT_ON_EXCEPTION").isEmpty();
}

static bool warningsForQmlFilesInsteadOfUiQmlEnabled()
{
    return DesignerSettings::getValue(DesignerSettingsKey::WARNING_FOR_QML_FILES_INSTEAD_OF_UIQML_FILES).toBool();
}

QmlDesignerPlugin::QmlDesignerPlugin()
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
    if (d) {
        Core::DesignMode::unregisterDesignWidget(&d->mainWidget);
        Core::ICore::removeContextObject(d->context);
        d->context = nullptr;
    }
    delete d;
    d = nullptr;
    m_instance = nullptr;
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
bool QmlDesignerPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage/* = 0*/)
{
    if (!Utils::HostOsInfo::canCreateOpenGLContext(errorMessage))
        return false;
    d = new QmlDesignerPluginPrivate;
    if (DesignerSettings::getValue(DesignerSettingsKey::STANDALONE_MODE).toBool())
        GenerateResource::generateMenuEntry();

    return true;
}

bool QmlDesignerPlugin::delayedInitialize()
{
    // adding default path to item library plugins
    const QString pluginPath = Utils::HostOsInfo::isMacHost()
            ? QString(QCoreApplication::applicationDirPath() + "/../PlugIns/QmlDesigner")
            : QString(QCoreApplication::applicationDirPath() + "/../"
                      + QLatin1String(IDE_LIBRARY_BASENAME) + "/qtcreator/plugins/qmldesigner");
    MetaInfo::setPluginPaths(QStringList(pluginPath));

    d->settings.fromSettings(Core::ICore::settings());

    d->viewManager.registerViewTakingOwnership(new QmlDesigner::Internal::ConnectionView);
    if (DesignerSettings::getValue(DesignerSettingsKey::ENABLE_TIMELINEVIEW).toBool()) {
        auto timelineView = new QmlDesigner::TimelineView;
        d->viewManager.registerViewTakingOwnership(timelineView);
        timelineView->registerActions();
    }

    d->viewManager.registerFormEditorToolTakingOwnership(new QmlDesigner::SourceTool);
    d->viewManager.registerFormEditorToolTakingOwnership(new QmlDesigner::ColorTool);
    d->viewManager.registerFormEditorToolTakingOwnership(new QmlDesigner::TextTool);
    d->viewManager.registerFormEditorToolTakingOwnership(new QmlDesigner::PathTool);

    return true;
}

void QmlDesignerPlugin::extensionsInitialized()
{
    Core::DesignMode::setDesignModeIsRequired();
    // delay after Core plugin's extensionsInitialized, so the DesignMode is availabe
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToOpen, this, [this] {
        integrateIntoQtCreator(&d->mainWidget);
    });

    connect(Core::ICore::instance(), &Core::ICore::windowStateChanged, this,
            [this] (Qt::WindowStates previousStates, Qt::WindowStates currentStates) {
        d->viewManager.nodeInstanceView()->mainWindowStateChanged(previousStates, currentStates);
    });

    connect(Core::ICore::instance(), &Core::ICore::windowActivationChanged, this,
            [this] (bool isActive, bool hasPopup) {
        d->viewManager.nodeInstanceView()->mainWindowActiveChanged(isActive, hasPopup);
    });
}

static QStringList allUiQmlFilesforCurrentProject(const Utils::FilePath &fileName)
{
    QStringList list;
    ProjectExplorer::Project *currentProject = ProjectExplorer::SessionManager::projectForFile(fileName);

    if (currentProject) {
        foreach (const Utils::FilePath &fileName, currentProject->files(ProjectExplorer::Project::SourceFiles)) {
            if (fileName.endsWith(".ui.qml"))
                list.append(fileName.toString());
        }
    }

    return list;
}

static QString projectPath(const Utils::FilePath &fileName)
{
    QString path;
    ProjectExplorer::Project *currentProject = ProjectExplorer::SessionManager::projectForFile(fileName);

    if (currentProject)
        path = currentProject->projectDirectory().toString();

    return path;
}

void QmlDesignerPlugin::integrateIntoQtCreator(QWidget *modeWidget)
{
    d->context = new Internal::DesignModeContext(modeWidget);
    Core::ICore::addContextObject(d->context);
    Core::Context qmlDesignerMainContext(Constants::C_QMLDESIGNER);
    Core::Context qmlDesignerFormEditorContext(Constants::C_QMLFORMEDITOR);
    Core::Context qmlDesignerNavigatorContext(Constants::C_QMLNAVIGATOR);

    d->context->context().add(qmlDesignerMainContext);
    d->context->context().add(qmlDesignerFormEditorContext);
    d->context->context().add(qmlDesignerNavigatorContext);
    d->context->context().add(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID);

    d->shortCutManager.registerActions(qmlDesignerMainContext, qmlDesignerFormEditorContext, qmlDesignerNavigatorContext);

    const QStringList mimeTypes = { QmlJSTools::Constants::QML_MIMETYPE,
                                    QmlJSTools::Constants::QMLUI_MIMETYPE };

    Core::DesignMode::registerDesignWidget(modeWidget, mimeTypes, d->context->context());

    connect(Core::DesignMode::instance(), &Core::DesignMode::actionsUpdated,
        &d->shortCutManager, &ShortCutManager::updateActions);

    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged, [this] (Core::IEditor *editor) {
        if (d && checkIfEditorIsQtQuick(editor) && isInDesignerMode())
            changeEditor();
    });

    connect(Core::EditorManager::instance(), &Core::EditorManager::editorsClosed, [this] (QList<Core::IEditor*> editors) {
        if (d) {
            if (d->documentManager.hasCurrentDesignDocument()
                    && editors.contains(currentDesignDocument()->textEditor()))
                hideDesigner();

            d->documentManager.removeEditors(editors);
        }
    });

    connect(Core::ModeManager::instance(), &Core::ModeManager::currentModeChanged,
        [this] (Core::Id newMode, Core::Id oldMode) {

        Core::IEditor *currentEditor = Core::EditorManager::currentEditor();
        if (d && currentEditor && checkIfEditorIsQtQuick(currentEditor) &&
                !documentIsAlreadyOpen(currentDesignDocument(), currentEditor, newMode)) {

            if (isDesignerMode(newMode)) {
                showDesigner();
            } else if (currentDesignDocument() ||
                     (!isDesignerMode(newMode) && isDesignerMode(oldMode))) {
                    hideDesigner();
            }
        }
    });
}

void QmlDesignerPlugin::showDesigner()
{
    QTC_ASSERT(!d->documentManager.hasCurrentDesignDocument(), return);

    d->mainWidget.initialize();

    const Utils::FilePath fileName = Core::EditorManager::currentEditor()->document()->filePath();
    const QStringList allUiQmlFiles = allUiQmlFilesforCurrentProject(fileName);
    if (warningsForQmlFilesInsteadOfUiQmlEnabled() && !fileName.endsWith(".ui.qml") && !allUiQmlFiles.isEmpty()) {
        OpenUiQmlFileDialog dialog(&d->mainWidget);
        dialog.setUiQmlFiles(projectPath(fileName), allUiQmlFiles);
        dialog.exec();
        if (dialog.uiFileOpened()) {
            Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
            Core::EditorManager::openEditorAt(dialog.uiQmlFile(), 0, 0);
            return;
        }
    }

    d->shortCutManager.disconnectUndoActions(currentDesignDocument());
    d->documentManager.setCurrentDesignDocument(Core::EditorManager::currentEditor());
    d->shortCutManager.connectUndoActions(currentDesignDocument());

    if (d->documentManager.hasCurrentDesignDocument()) {
        activateAutoSynchronization();
        d->shortCutManager.updateActions(currentDesignDocument()->textEditor());
        d->viewManager.pushFileOnCrumbleBar(currentDesignDocument()->fileName());
    }

    d->shortCutManager.updateUndoActions(currentDesignDocument());
}

void QmlDesignerPlugin::hideDesigner()
{
    if (d->documentManager.hasCurrentDesignDocument()) {
        deactivateAutoSynchronization();
        d->mainWidget.saveSettings();
    }

    d->shortCutManager.disconnectUndoActions(currentDesignDocument());
    d->documentManager.setCurrentDesignDocument(nullptr);
    d->shortCutManager.updateUndoActions(nullptr);
}

void QmlDesignerPlugin::changeEditor()
{
    if (d->blockEditorChange)
         return;

    if (d->documentManager.hasCurrentDesignDocument()) {
        deactivateAutoSynchronization();
        d->mainWidget.saveSettings();
    }

    d->shortCutManager.disconnectUndoActions(currentDesignDocument());
    d->documentManager.setCurrentDesignDocument(Core::EditorManager::currentEditor());
    d->mainWidget.initialize();
    d->shortCutManager.connectUndoActions(currentDesignDocument());

    if (d->documentManager.hasCurrentDesignDocument()) {
        activateAutoSynchronization();
        d->viewManager.pushFileOnCrumbleBar(currentDesignDocument()->fileName());
        d->viewManager.setComponentViewToMaster();
    }

    d->shortCutManager.updateUndoActions(currentDesignDocument());
}

void QmlDesignerPlugin::jumpTextCursorToSelectedModelNode()
{
    // visual editor -> text editor
    ModelNode selectedNode;
    if (!rewriterView()->selectedModelNodes().isEmpty())
        selectedNode = rewriterView()->selectedModelNodes().constFirst();

    if (selectedNode.isValid()) {
        const int nodeOffset = rewriterView()->nodeOffset(selectedNode);
        if (nodeOffset > 0) {
            const ModelNode currentSelectedNode = rewriterView()->
                nodeAtTextCursorPosition(currentDesignDocument()->plainTextEdit()->textCursor().position());
            if (currentSelectedNode != selectedNode) {
                int line, column;
                currentDesignDocument()->textEditor()->convertPosition(nodeOffset, &line, &column);
                // line has to be 1 based, column 0 based!
                currentDesignDocument()->textEditor()->gotoLine(line, column - 1);
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

    currentDesignDocument()->updateActiveTarget();
    currentDesignDocument()->updateActiveTarget();
    d->mainWidget.enableWidgets();
    currentDesignDocument()->attachRewriterToModel();

    resetModelSelection();

    viewManager().attachComponentView();
    viewManager().attachViewsExceptRewriterAndComponetView();

    selectModelNodeUnderTextCursor();

    d->mainWidget.setupNavigatorHistory(currentDesignDocument()->textEditor());

    currentDesignDocument()->updateSubcomponentManager();
}

void QmlDesignerPlugin::deactivateAutoSynchronization()
{
    viewManager().detachViewsExceptRewriterAndComponetView();
    viewManager().detachComponentView();
    viewManager().detachRewriterView();
    documentManager().currentDesignDocument()->resetToDocumentModel();
}

void QmlDesignerPlugin::resetModelSelection()
{
    if (!rewriterView()) {
        qCWarning(qmldesignerLog) << "No rewriter existing while calling resetModelSelection";
        return;
    }
    if (!currentModel()) {
        qCWarning(qmldesignerLog) << "No current QmlDesigner document model while calling resetModelSelection";
        return;
    }
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

DesignDocument *QmlDesignerPlugin::currentDesignDocument() const
{
    if (d)
        return d->documentManager.currentDesignDocument();

    return nullptr;
}

Internal::DesignModeWidget *QmlDesignerPlugin::mainWidget() const
{
    if (d)
        return &d->mainWidget;

    return nullptr;
}

QWidget *QmlDesignerPlugin::createProjectExplorerWidget(QWidget *parent) const
{
    return Internal::DesignModeWidget::createProjectExplorerWidget(parent);
}

void QmlDesignerPlugin::switchToTextModeDeferred()
{
    QTimer::singleShot(0, this, [] () {
        Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
    });
}

void QmlDesignerPlugin::emitCurrentTextEditorChanged(Core::IEditor *editor)
{
    d->blockEditorChange = true;
    emit Core::EditorManager::instance()->currentEditorChanged(editor);
    d->blockEditorChange = false;
}

double QmlDesignerPlugin::formEditorDevicePixelRatio()
{
    if (DesignerSettings::getValue(DesignerSettingsKey::IGNORE_DEVICE_PIXEL_RATIO).toBool())
        return 1;

    const QList<QWindow *> topLevelWindows = QApplication::topLevelWindows();
    if (topLevelWindows.isEmpty())
        return 1;
    return topLevelWindows.constFirst()->screen()->devicePixelRatio();
}

QmlDesignerPlugin *QmlDesignerPlugin::instance()
{
    return m_instance;
}

DocumentManager &QmlDesignerPlugin::documentManager()
{
    return d->documentManager;
}

const DocumentManager &QmlDesignerPlugin::documentManager() const
{
    return d->documentManager;
}

ViewManager &QmlDesignerPlugin::viewManager()
{
    return d->viewManager;
}

const ViewManager &QmlDesignerPlugin::viewManager() const
{
    return d->viewManager;
}

DesignerActionManager &QmlDesignerPlugin::designerActionManager()
{
    return d->viewManager.designerActionManager();
}

const DesignerActionManager &QmlDesignerPlugin::designerActionManager() const
{
    return d->viewManager.designerActionManager();
}

DesignerSettings QmlDesignerPlugin::settings()
{
    d->settings.fromSettings(Core::ICore::settings());
    return d->settings;
}

void QmlDesignerPlugin::setSettings(const DesignerSettings &s)
{
    if (s != d->settings) {
        d->settings = s;
        d->settings.toSettings(Core::ICore::settings());
    }
}

} // namespace QmlDesigner
