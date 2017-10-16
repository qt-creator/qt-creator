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

#include "formeditorw.h"
#include "formwindoweditor.h"
#include "formwindowfile.h"
#include "settingsmanager.h"
#include "settingspage.h"
#include "editorwidget.h"
#include "editordata.h"
#include "qtcreatorintegration.h"
#include "designercontext.h"
#include <widgethost.h>

#include <coreplugin/editortoolbar.h>
#include <coreplugin/designmode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/infobar.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/outputpane.h>
#include <utils/qtcassert.h>

#include <QDesignerFormEditorPluginInterface>
#include <QDesignerFormEditorInterface>
#include <QDesignerComponents>
#include <QDesignerFormWindowManagerInterface>
#include <QDesignerWidgetBoxInterface>
#include <abstractobjectinspector.h>
#include <QDesignerPropertyEditorInterface>
#include <QDesignerActionEditorInterface>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCursor>
#include <QDockWidget>
#include <QMenu>
#include <QMessageBox>
#include <QKeySequence>
#include <QPrintDialog>
#include <QPrinter>
#include <QPainter>
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>

#include <QDebug>
#include <QSettings>
#include <QPluginLoader>
#include <QTime>

#include <algorithm>

static const char settingsGroupC[] = "Designer";

/* Actions of the designer plugin:
 * Designer provides a toolbar which is subject to a context change (to
 * "edit mode" context) when it is focused.
 * In order to prevent its actions from being disabled/hidden by that context
 * change, the actions are registered on the global context. In currentEditorChanged(),
 * the ones that are present in the global edit menu are set visible/invisible manually.
 * The designer context is currently used for Cut/Copy/Paste, etc. */

static inline QIcon designerIcon(const QString &iconName)
{
    const QIcon icon = QDesignerFormEditorInterface::createIcon(iconName);
    if (icon.isNull())
        qWarning() << "Unable to locate " << iconName;
    return icon;
}

using namespace Core;
using namespace Designer::Constants;

namespace Designer {
namespace Internal {

/* A stub-like, read-only text editor which displays UI files as text. Could be used as a
  * read/write editor too, but due to lack of XML editor, highlighting and other such
  * functionality, editing is disabled.
  * Provides an informational title bar containing a button triggering a
  * switch to design mode.
  * Internally manages a FormWindowEditor and uses the plain text
  * editable embedded in it.  */
class DesignerXmlEditorWidget : public TextEditor::TextEditorWidget
{
public:
    DesignerXmlEditorWidget() {}

    void finalizeInitialization() override
    {
        setReadOnly(true);
    }
};

class FormWindowEditorFactory : public TextEditor::TextEditorFactory
{
public:
    FormWindowEditorFactory()
    {
        setId(K_DESIGNER_XML_EDITOR_ID);
        setEditorCreator([]() { return new FormWindowEditor; });
        setEditorWidgetCreator([]() { return new Internal::DesignerXmlEditorWidget; });
        setUseGenericHighlighter(true);
        setDuplicatedSupported(false);
    }

    FormWindowEditor *create(QDesignerFormWindowInterface *form)
    {
        setDocumentCreator([form]() { return new FormWindowFile(form); });
        return qobject_cast<FormWindowEditor *>(createEditor());
    }
};

// --------- FormEditorW

class FormEditorData
{
public:
    Q_DECLARE_TR_FUNCTIONS(FormEditorW)

public:
    FormEditorData();
    ~FormEditorData();

    void activateEditMode(int id);
    void toolChanged(int);
    void print();
    void setPreviewMenuEnabled(bool e);
    void updateShortcut(Command *command);

    void fullInit();

    void saveSettings(QSettings *s);

    void initDesignerSubWindows();

    void setupActions();
    void setupViewActions();
    void addDockViewAction(ActionContainer *viewMenu,
                           int index,
                           const Context &context,
                           const QString &title, Id id);

    ActionContainer *createPreviewStyleMenu(QActionGroup *actionGroup);

    void critical(const QString &errorMessage);
    void bindShortcut(Command *command, QAction *action);
    QAction *createEditModeAction(QActionGroup *ag,
                                  const Context &context,
                                  ActionContainer *medit,
                                  const QString &actionName,
                                  Id id,
                                  int toolNumber,
                                  const QString &iconName = QString(),
                                  const QString &keySequence = QString());
    Command *addToolAction(QAction *a,
                                 const Context &context, Id id,
                                 ActionContainer *c1, const QString &keySequence = QString(),
                                 Id groupId = Id());
    QToolBar *createEditorToolBar() const;
    IEditor *createEditor();

public:
    QDesignerFormEditorInterface *m_formeditor = nullptr;
    QtCreatorIntegration *m_integration = nullptr;
    QDesignerFormWindowManagerInterface *m_fwm = nullptr;
    FormEditorW::InitializationStage m_initStage;

    QWidget *m_designerSubWindows[DesignerSubWindowCount];

    QAction *m_lockAction = nullptr;
    QAction *m_resetLayoutAction = nullptr;

    QList<IOptionsPage *> m_settingsPages;
    QActionGroup *m_actionGroupEditMode = nullptr;
    QAction *m_actionPrint = nullptr;
    QAction *m_actionPreview = nullptr;
    QActionGroup *m_actionGroupPreviewInStyle = nullptr;
    QMenu *m_previewInStyleMenu = nullptr;
    QAction *m_actionAboutPlugins = nullptr;

    DesignerContext *m_context = nullptr;
    Context m_contexts;

    QList<Id> m_toolActionIds;
    QWidget *m_modeWidget = nullptr;
    EditorWidget *m_editorWidget = nullptr;
    DesignMode *m_designMode = nullptr;

    QWidget *m_editorToolBar = nullptr;
    EditorToolBar *m_toolBar = nullptr;

    QMap<Command *, QAction *> m_commandToDesignerAction;
    FormWindowEditorFactory *m_xmlEditorFactory = nullptr;
};

static FormEditorData *d = nullptr;
static FormEditorW *m_instance = nullptr;

FormEditorData::FormEditorData() :
    m_formeditor(QDesignerComponents::createFormEditor(0)),
    m_initStage(FormEditorW::RegisterPlugins)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO;
    QTC_ASSERT(!d, return);
    d = this;

    std::fill(m_designerSubWindows, m_designerSubWindows + DesignerSubWindowCount,
              static_cast<QWidget *>(0));

    m_formeditor->setTopLevel(ICore::mainWindow());
    m_formeditor->setSettingsManager(new SettingsManager());

    m_fwm = m_formeditor->formWindowManager();
    QTC_ASSERT(m_fwm, return);

    m_contexts.add(C_FORMEDITOR);

    setupActions();

    const auto optionPages = m_formeditor->optionsPages();
    for (QDesignerOptionsPageInterface *designerPage : optionPages) {
        SettingsPage *settingsPage = new SettingsPage(designerPage);
        m_settingsPages.append(settingsPage);
    }

    QObject::connect(EditorManager::instance(), &EditorManager::currentEditorChanged, [this](IEditor *editor) {
        if (Designer::Constants::Internal::debug)
            qDebug() << Q_FUNC_INFO << editor << " of " << m_fwm->formWindowCount();

        if (editor && editor->document()->id() == Constants::K_DESIGNER_XML_EDITOR_ID) {
            FormWindowEditor *xmlEditor = qobject_cast<FormWindowEditor *>(editor);
            QTC_ASSERT(xmlEditor, return);
            FormEditorW::ensureInitStage(FormEditorW::FullyInitialized);
            SharedTools::WidgetHost *fw = m_editorWidget->formWindowEditorForXmlEditor(xmlEditor);
            QTC_ASSERT(fw, return);
            m_editorWidget->setVisibleEditor(xmlEditor);
            m_fwm->setActiveFormWindow(fw->formWindow());
        }
    });

    m_xmlEditorFactory = new FormWindowEditorFactory;
}

FormEditorData::~FormEditorData()
{
    if (m_context)
        ICore::removeContextObject(m_context);
    if (m_initStage == FormEditorW::FullyInitialized) {
        QSettings *s = ICore::settings();
        s->beginGroup(settingsGroupC);
        m_editorWidget->saveSettings(s);
        s->endGroup();

        m_designMode->unregisterDesignWidget(m_modeWidget);
        delete m_modeWidget;
        m_modeWidget = nullptr;
    }

    delete m_formeditor;
    qDeleteAll(m_settingsPages);
    m_settingsPages.clear();
    delete m_integration;

    delete m_xmlEditorFactory ;
    d = nullptr;
}

// Add an actioon to toggle the view state of a dock window
void FormEditorData::addDockViewAction(ActionContainer *viewMenu,
                                    int index, const Context &context,
                                    const QString &title, Id id)
{
    if (const QDockWidget *dw = m_editorWidget->designerDockWidgets()[index]) {
        QAction *action = dw->toggleViewAction();
        action->setText(title);
        Command *cmd = addToolAction(action, context, id, viewMenu, QString());
        cmd->setAttribute(Command::CA_Hide);
    }
}

void FormEditorData::setupViewActions()
{
    // Populate "View" menu of form editor menu
    ActionContainer *viewMenu = ActionManager::actionContainer(Core::Constants::M_WINDOW_VIEWS);
    QTC_ASSERT(viewMenu, return);

    addDockViewAction(viewMenu, WidgetBoxSubWindow, m_contexts,
                      tr("Widget box"), "FormEditor.WidgetBox");

    addDockViewAction(viewMenu, ObjectInspectorSubWindow, m_contexts,
                      tr("Object Inspector"), "FormEditor.ObjectInspector");

    addDockViewAction(viewMenu, PropertyEditorSubWindow, m_contexts,
                      tr("Property Editor"), "FormEditor.PropertyEditor");

    addDockViewAction(viewMenu, SignalSlotEditorSubWindow, m_contexts,
                      tr("Signals && Slots Editor"), "FormEditor.SignalsAndSlotsEditor");

    addDockViewAction(viewMenu, ActionEditorSubWindow, m_contexts,
                      tr("Action Editor"), "FormEditor.ActionEditor");
    // Lock/Reset
    Command *cmd = addToolAction(m_editorWidget->menuSeparator1(), m_contexts, "FormEditor.SeparatorLock", viewMenu);
    cmd->setAttribute(Command::CA_Hide);

    cmd = addToolAction(m_editorWidget->autoHideTitleBarsAction(), m_contexts, "FormEditor.Locked", viewMenu);
    cmd->setAttribute(Command::CA_Hide);

    cmd = addToolAction(m_editorWidget->menuSeparator2(), m_contexts, "FormEditor.SeparatorReset", viewMenu);
    cmd->setAttribute(Command::CA_Hide);

    cmd = addToolAction(m_editorWidget->resetLayoutAction(), m_contexts, "FormEditor.ResetToDefaultLayout", viewMenu);

    QObject::connect(m_editorWidget, &EditorWidget::resetLayout,
                     m_editorWidget, &EditorWidget::resetToDefaultLayout);

    cmd->setAttribute(Command::CA_Hide);
}

void FormEditorData::fullInit()
{
    QTC_ASSERT(m_initStage == FormEditorW::RegisterPlugins, return);
    QTime *initTime = 0;
    if (Designer::Constants::Internal::debug) {
        initTime = new QTime;
        initTime->start();
    }

    QDesignerComponents::createTaskMenu(m_formeditor, m_instance);
    QDesignerComponents::initializePlugins(m_formeditor);
    QDesignerComponents::initializeResources();
    initDesignerSubWindows();
    m_integration = new QtCreatorIntegration(m_formeditor, m_instance);
    m_formeditor->setIntegration(m_integration);
    // Connect Qt Designer help request to HelpManager.
    QObject::connect(m_integration, &QtCreatorIntegration::creatorHelpRequested,
                     HelpManager::instance(),
                     [](const QUrl &url) { HelpManager::instance()->handleHelpRequest(url, HelpManager::HelpModeAlways); });

    /**
     * This will initialize our TabOrder, Signals and slots and Buddy editors.
     */
    const QList<QObject *> plugins = QPluginLoader::staticInstances() + m_formeditor->pluginInstances();
    for (QObject *plugin : plugins) {
        if (QDesignerFormEditorPluginInterface *formEditorPlugin = qobject_cast<QDesignerFormEditorPluginInterface*>(plugin)) {
            if (!formEditorPlugin->isInitialized())
                formEditorPlugin->initialize(m_formeditor);
        }
    }

    if (m_actionAboutPlugins)
        m_actionAboutPlugins->setEnabled(true);

    if (Designer::Constants::Internal::debug) {
        qDebug() << Q_FUNC_INFO << initTime->elapsed() << "ms";
        delete initTime;
    }

    QObject::connect(EditorManager::instance(), &EditorManager::editorsClosed,
                     [this] (const QList<IEditor *> editors) {
        for (IEditor *editor : editors)
            m_editorWidget->removeFormWindowEditor(editor);
    });

    // Nest toolbar and editor widget
    m_editorWidget = new EditorWidget;
    QSettings *settings = ICore::settings();
    settings->beginGroup(settingsGroupC);
    m_editorWidget->restoreSettings(settings);
    settings->endGroup();

    m_editorToolBar = createEditorToolBar();
    m_toolBar = new EditorToolBar;
    m_toolBar->setToolbarCreationFlags(EditorToolBar::FlagsStandalone);
    m_toolBar->setNavigationVisible(false);
    m_toolBar->addCenterToolBar(m_editorToolBar);

    m_designMode = DesignMode::instance();
    m_modeWidget = new QWidget;
    m_modeWidget->setObjectName("DesignerModeWidget");
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_toolBar);
    // Avoid mode switch to 'Edit' mode when the application started by
    // 'Run' in 'Design' mode emits output.
    MiniSplitter *splitter = new MiniSplitter(Qt::Vertical);
    splitter->addWidget(m_editorWidget);
    QWidget *outputPane = new OutputPanePlaceHolder(m_designMode->id(), splitter);
    outputPane->setObjectName("DesignerOutputPanePlaceHolder");
    splitter->addWidget(outputPane);
    layout->addWidget(splitter);
    m_modeWidget->setLayout(layout);

    Context designerContexts = m_contexts;
    designerContexts.add(Core::Constants::C_EDITORMANAGER);
    m_context = new DesignerContext(designerContexts, m_modeWidget, m_instance);
    ICore::addContextObject(m_context);

    m_designMode->registerDesignWidget(m_modeWidget, QStringList(FORM_MIMETYPE), m_contexts);

    setupViewActions();

    m_initStage = FormEditorW::FullyInitialized;
}

void FormEditorData::initDesignerSubWindows()
{
    std::fill(m_designerSubWindows, m_designerSubWindows + DesignerSubWindowCount, static_cast<QWidget*>(0));

    QDesignerWidgetBoxInterface *wb = QDesignerComponents::createWidgetBox(m_formeditor, 0);
    wb->setWindowTitle(tr("Widget Box"));
    wb->setObjectName("WidgetBox");
    m_formeditor->setWidgetBox(wb);
    m_designerSubWindows[WidgetBoxSubWindow] = wb;

    QDesignerObjectInspectorInterface *oi = QDesignerComponents::createObjectInspector(m_formeditor, 0);
    oi->setWindowTitle(tr("Object Inspector"));
    oi->setObjectName("ObjectInspector");
    m_formeditor->setObjectInspector(oi);
    m_designerSubWindows[ObjectInspectorSubWindow] = oi;

    QDesignerPropertyEditorInterface *pe = QDesignerComponents::createPropertyEditor(m_formeditor, 0);
    pe->setWindowTitle(tr("Property Editor"));
    pe->setObjectName("PropertyEditor");
    m_formeditor->setPropertyEditor(pe);
    m_designerSubWindows[PropertyEditorSubWindow] = pe;

    QWidget *se = QDesignerComponents::createSignalSlotEditor(m_formeditor, 0);
    se->setWindowTitle(tr("Signals && Slots Editor"));
    se->setObjectName("SignalsAndSlotsEditor");
    m_designerSubWindows[SignalSlotEditorSubWindow] = se;

    QDesignerActionEditorInterface *ae = QDesignerComponents::createActionEditor(m_formeditor, 0);
    ae->setWindowTitle(tr("Action Editor"));
    ae->setObjectName("ActionEditor");
    m_formeditor->setActionEditor(ae);
    m_designerSubWindows[ActionEditorSubWindow] = ae;
    m_initStage = FormEditorW::SubwindowsInitialized;
}

QList<IOptionsPage *> FormEditorW::optionsPages()
{
    return d->m_settingsPages;
}

void FormEditorW::ensureInitStage(InitializationStage s)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << s;
    if (!d) {
        m_instance = new FormEditorW;
        d = new FormEditorData;
    }
    if (d->m_initStage >= s)
        return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    d->fullInit();
    QApplication::restoreOverrideCursor();
}

void FormEditorW::deleteInstance()
{
    delete d;
    d = nullptr;
    delete m_instance;
    m_instance = nullptr;
}

IEditor *FormEditorW::createEditor()
{
    ensureInitStage(FullyInitialized);
    return d->createEditor();
}

void FormEditorData::setupActions()
{
    //menus
    ActionContainer *medit = ActionManager::actionContainer(Core::Constants::M_EDIT);
    ActionContainer *mformtools = ActionManager::actionContainer(M_FORMEDITOR);

    //overridden actions
    bindShortcut(ActionManager::registerAction(m_fwm->actionUndo(), Core::Constants::UNDO, m_contexts), m_fwm->actionUndo());
    bindShortcut(ActionManager::registerAction(m_fwm->actionRedo(), Core::Constants::REDO, m_contexts), m_fwm->actionRedo());
    bindShortcut(ActionManager::registerAction(m_fwm->actionCut(), Core::Constants::CUT, m_contexts), m_fwm->actionCut());
    bindShortcut(ActionManager::registerAction(m_fwm->actionCopy(), Core::Constants::COPY, m_contexts), m_fwm->actionCopy());
    bindShortcut(ActionManager::registerAction(m_fwm->actionPaste(), Core::Constants::PASTE, m_contexts), m_fwm->actionPaste());
    bindShortcut(ActionManager::registerAction(m_fwm->actionSelectAll(), Core::Constants::SELECTALL, m_contexts), m_fwm->actionSelectAll());

    m_actionPrint = new QAction(m_instance);
    bindShortcut(ActionManager::registerAction(m_actionPrint, Core::Constants::PRINT, m_contexts), m_actionPrint);
    QObject::connect(m_actionPrint, &QAction::triggered, [this]() { print(); });

    //'delete' action. Do not set a shortcut as Designer handles
    // the 'Delete' key by event filter. Setting a shortcut triggers
    // buggy behaviour on Mac (Pressing Delete in QLineEdit removing the widget).
    Command *command;
    command = ActionManager::registerAction(m_fwm->actionDelete(), "FormEditor.Edit.Delete", m_contexts);
    bindShortcut(command, m_fwm->actionDelete());
    command->setAttribute(Command::CA_Hide);
    medit->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    m_actionGroupEditMode = new QActionGroup(m_instance);
    m_actionGroupEditMode->setExclusive(true);
    QObject::connect(m_actionGroupEditMode, &QActionGroup::triggered,
            [this](QAction *a) { activateEditMode(a->data().toInt()); });

    medit->addSeparator(m_contexts, Core::Constants::G_EDIT_OTHER);

    m_toolActionIds.push_back("FormEditor.WidgetEditor");
    createEditModeAction(m_actionGroupEditMode, m_contexts, medit,
                         tr("Edit Widgets"), m_toolActionIds.back(),
                         EditModeWidgetEditor, "widgettool.png", tr("F3"));

    m_toolActionIds.push_back("FormEditor.SignalsSlotsEditor");
    createEditModeAction(m_actionGroupEditMode, m_contexts, medit,
                         tr("Edit Signals/Slots"), m_toolActionIds.back(),
                         EditModeSignalsSlotEditor, "signalslottool.png", tr("F4"));

    m_toolActionIds.push_back("FormEditor.BuddyEditor");
    createEditModeAction(m_actionGroupEditMode, m_contexts, medit,
                         tr("Edit Buddies"), m_toolActionIds.back(),
                         EditModeBuddyEditor, "buddytool.png");

    m_toolActionIds.push_back("FormEditor.TabOrderEditor");
    createEditModeAction(m_actionGroupEditMode, m_contexts, medit,
                         tr("Edit Tab Order"),  m_toolActionIds.back(),
                         EditModeTabOrderEditor, "tabordertool.png");

    //tool actions
    m_toolActionIds.push_back("FormEditor.LayoutHorizontally");
    const QString horizLayoutShortcut = UseMacShortcuts ? tr("Meta+Shift+H") : tr("Ctrl+H");
    addToolAction(m_fwm->actionHorizontalLayout(), m_contexts,
                  m_toolActionIds.back(), mformtools, horizLayoutShortcut);

    m_toolActionIds.push_back("FormEditor.LayoutVertically");
    const QString vertLayoutShortcut = UseMacShortcuts ? tr("Meta+L") : tr("Ctrl+L");
    addToolAction(m_fwm->actionVerticalLayout(), m_contexts,
                  m_toolActionIds.back(),  mformtools, vertLayoutShortcut);

    m_toolActionIds.push_back("FormEditor.SplitHorizontal");
    addToolAction(m_fwm->actionSplitHorizontal(), m_contexts,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back("FormEditor.SplitVertical");
    addToolAction(m_fwm->actionSplitVertical(), m_contexts,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back("FormEditor.LayoutForm");
    addToolAction(m_fwm->actionFormLayout(), m_contexts,
                  m_toolActionIds.back(),  mformtools);

    m_toolActionIds.push_back("FormEditor.LayoutGrid");
    const QString gridShortcut = UseMacShortcuts ? tr("Meta+Shift+G") : tr("Ctrl+G");
    addToolAction(m_fwm->actionGridLayout(), m_contexts,
                  m_toolActionIds.back(),  mformtools, gridShortcut);

    m_toolActionIds.push_back("FormEditor.LayoutBreak");
    addToolAction(m_fwm->actionBreakLayout(), m_contexts,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back("FormEditor.LayoutAdjustSize");
    const QString adjustShortcut = UseMacShortcuts ? tr("Meta+J") : tr("Ctrl+J");
    addToolAction(m_fwm->actionAdjustSize(), m_contexts,
                  m_toolActionIds.back(),  mformtools, adjustShortcut);

    m_toolActionIds.push_back("FormEditor.SimplifyLayout");
    addToolAction(m_fwm->actionSimplifyLayout(), m_contexts,
                  m_toolActionIds.back(),  mformtools);

    mformtools->addSeparator(m_contexts);

    addToolAction(m_fwm->actionLower(), m_contexts, "FormEditor.Lower", mformtools);
    addToolAction(m_fwm->actionRaise(), m_contexts, "FormEditor.Raise", mformtools);

    // Commands that do not go into the editor toolbar
    mformtools->addSeparator(m_contexts);

    m_actionPreview = m_fwm->action(QDesignerFormWindowManagerInterface::DefaultPreviewAction);
    QTC_ASSERT(m_actionPreview, return);
    addToolAction(m_actionPreview, m_contexts, "FormEditor.Preview", mformtools, tr("Alt+Shift+R"));

    // Preview in style...
    m_actionGroupPreviewInStyle = m_fwm->actionGroup(QDesignerFormWindowManagerInterface::StyledPreviewActionGroup);

    ActionContainer *previewAC = createPreviewStyleMenu(m_actionGroupPreviewInStyle);
    m_previewInStyleMenu = previewAC->menu();
    mformtools->addMenu(previewAC);
    setPreviewMenuEnabled(false);

    // Form settings
    medit->addSeparator(m_contexts, Core::Constants::G_EDIT_OTHER);

    mformtools->addSeparator(m_contexts);

    mformtools->addSeparator(m_contexts, Core::Constants::G_DEFAULT_THREE);
    QAction *actionFormSettings = m_fwm->action(QDesignerFormWindowManagerInterface::FormWindowSettingsDialogAction);
    addToolAction(actionFormSettings, m_contexts, "FormEditor.FormSettings", mformtools,
                  QString(), Core::Constants::G_DEFAULT_THREE);

    mformtools->addSeparator(m_contexts, Core::Constants::G_DEFAULT_THREE);
    m_actionAboutPlugins = new QAction(tr("About Qt Designer Plugins..."), m_instance);
    addToolAction(m_actionAboutPlugins, m_contexts, "FormEditor.AboutPlugins", mformtools,
                  QString(), Core::Constants::G_DEFAULT_THREE);
    QObject::connect(m_actionAboutPlugins, &QAction::triggered,
        m_fwm, &QDesignerFormWindowManagerInterface::showPluginDialog);
    m_actionAboutPlugins->setEnabled(false);

    // FWM
    QObject::connect(m_fwm, &QDesignerFormWindowManagerInterface::activeFormWindowChanged,
        [this] (QDesignerFormWindowInterface *afw) {
            m_fwm->closeAllPreviews();
            setPreviewMenuEnabled(afw != 0);
        });
}

QToolBar *FormEditorData::createEditorToolBar() const
{
    QToolBar *editorToolBar = new QToolBar;
    const QList<Id>::const_iterator cend = m_toolActionIds.constEnd();
    for (QList<Id>::const_iterator it = m_toolActionIds.constBegin(); it != cend; ++it) {
        Command *cmd = ActionManager::command(*it);
        QTC_ASSERT(cmd, continue);
        QAction *action = cmd->action();
        if (!action->icon().isNull()) // Simplify grid has no action yet
            editorToolBar->addAction(action);
    }
    const int size = editorToolBar->style()->pixelMetric(QStyle::PM_SmallIconSize);
    editorToolBar->setIconSize(QSize(size, size));
    editorToolBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    return editorToolBar;
}

ActionContainer *FormEditorData::createPreviewStyleMenu(QActionGroup *actionGroup)
{
    const QString menuId = M_FORMEDITOR_PREVIEW;
    ActionContainer *menuPreviewStyle = ActionManager::createMenu(M_FORMEDITOR_PREVIEW);
    menuPreviewStyle->menu()->setTitle(tr("Preview in"));

    // The preview menu is a list of invisible actions for the embedded design
    // device profiles (integer data) followed by a separator and the styles
    // (string data). Make device profiles update their text and hide them
    // in the configuration dialog.
    const QList<QAction*> actions = actionGroup->actions();

    const QString deviceProfilePrefix = "DeviceProfile";
    const QChar dot = '.';

    for (QAction *a : actions) {
        QString name = menuId;
        name += dot;
        const QVariant data = a->data();
        const bool isDeviceProfile = data.type() == QVariant::Int;
        if (isDeviceProfile) {
            name += deviceProfilePrefix;
            name += dot;
        }
        name += data.toString();
        Command *command = ActionManager::registerAction(a, Id::fromString(name), m_contexts);
        bindShortcut(command, a);
        if (isDeviceProfile) {
            command->setAttribute(Command::CA_UpdateText);
            command->setAttribute(Command::CA_NonConfigurable);
        }
        menuPreviewStyle->addAction(command);
    }
    return menuPreviewStyle;
}

void FormEditorData::setPreviewMenuEnabled(bool e)
{
    m_actionPreview->setEnabled(e);
    m_previewInStyleMenu->setEnabled(e);
}

void FormEditorData::saveSettings(QSettings *s)
{
    s->beginGroup(settingsGroupC);
    m_editorWidget->saveSettings(s);
    s->endGroup();
}

void FormEditorData::critical(const QString &errorMessage)
{
    QMessageBox::critical(ICore::mainWindow(), tr("Designer"),  errorMessage);
}

// Apply the command shortcut to the action and connects to the command's keySequenceChanged signal
void FormEditorData::bindShortcut(Command *command, QAction *action)
{
    m_commandToDesignerAction.insert(command, action);
    QObject::connect(command, &Command::keySequenceChanged,
                     command, [this, command] { updateShortcut(command); });
    updateShortcut(command);
}

// Create an action to activate a designer tool
QAction *FormEditorData::createEditModeAction(QActionGroup *ag,
                                     const Context &context,
                                     ActionContainer *medit,
                                     const QString &actionName,
                                     Id id,
                                     int toolNumber,
                                     const QString &iconName,
                                     const QString &keySequence)
{
    QAction *rc = new QAction(actionName, ag);
    rc->setCheckable(true);
    if (!iconName.isEmpty())
         rc->setIcon(designerIcon(iconName));
    Command *command = ActionManager::registerAction(rc, id, context);
    command->setAttribute(Command::CA_Hide);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    bindShortcut(command, rc);
    medit->addAction(command, Core::Constants::G_EDIT_OTHER);
    rc->setData(toolNumber);
    ag->addAction(rc);
    return rc;
}

// Create a tool action
Command *FormEditorData::addToolAction(QAction *a, const Context &context, Id id,
                                       ActionContainer *c1, const QString &keySequence,
                                       Id groupId)
{
    Command *command = ActionManager::registerAction(a, id, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    if (!a->isSeparator())
        bindShortcut(command, a);
    c1->addAction(command, groupId);
    return command;
}

IEditor *FormEditorData::createEditor()
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorW::createEditor";
    // Create and associate form and text editor.
    m_fwm->closeAllPreviews();
    QDesignerFormWindowInterface *form = m_fwm->createFormWindow(0);
    QTC_ASSERT(form, return 0);
    QObject::connect(form, &QDesignerFormWindowInterface::toolChanged, [this] (int i) { toolChanged(i); });

    SharedTools::WidgetHost *widgetHost = new SharedTools::WidgetHost( /* parent */ 0, form);
    FormWindowEditor *formWindowEditor = m_xmlEditorFactory->create(form);

    m_editorWidget->add(widgetHost, formWindowEditor);
    m_toolBar->addEditor(formWindowEditor);

    if (formWindowEditor) {
        InfoBarEntry info(Id(Constants::INFO_READ_ONLY),
                                tr("This file can only be edited in <b>Design</b> mode."));
        info.setCustomButtonInfo(tr("Switch Mode"), []() { ModeManager::activateMode(Core::Constants::MODE_DESIGN); });
        formWindowEditor->document()->infoBar()->addInfo(info);
    }
    return formWindowEditor;
}

QDesignerFormEditorInterface *FormEditorW::designerEditor()
{
    ensureInitStage(FullyInitialized);
    return d->m_formeditor;
}

QWidget * const *FormEditorW::designerSubWindows()
{
    ensureInitStage(SubwindowsInitialized);
    return d->m_designerSubWindows;
}

SharedTools::WidgetHost *FormEditorW::activeWidgetHost()
{
    ensureInitStage(FullyInitialized);
    if (d->m_editorWidget)
        return d->m_editorWidget->activeEditor().widgetHost;
    return 0;
}

FormWindowEditor *FormEditorW::activeEditor()
{
    ensureInitStage(FullyInitialized);
    if (d->m_editorWidget)
        return d->m_editorWidget->activeEditor().formWindowEditor;
    return 0;
}

void FormEditorData::updateShortcut(Command *command)
{
    if (!command)
        return;
    if (QAction *a = m_commandToDesignerAction.value(command))
        a->setShortcut(command->action()->shortcut());
}

void FormEditorData::activateEditMode(int id)
{
    if (const int count = m_fwm->formWindowCount())
        for (int i = 0; i <  count; i++)
             m_fwm->formWindow(i)->setCurrentTool(id);
}

void FormEditorData::toolChanged(int t)
{
    typedef QList<QAction *> ActionList;
    if (const QAction *currentAction = m_actionGroupEditMode->checkedAction())
        if (currentAction->data().toInt() == t)
            return;
    const ActionList actions = m_actionGroupEditMode->actions();
    const ActionList::const_iterator cend = actions.constEnd();
    for (ActionList::const_iterator it = actions.constBegin(); it != cend; ++it)
        if ( (*it)->data().toInt() == t) {
            (*it)->setChecked(true);
            break;
        }
}

void FormEditorData::print()
{
    // Printing code courtesy of designer_actions.cpp
    QDesignerFormWindowInterface *fw = m_fwm->activeFormWindow();
    if (!fw)
        return;

    QPrinter *printer = ICore::printer();
    const bool oldFullPage =  printer->fullPage();
    const QPrinter::Orientation oldOrientation =  printer->orientation ();
    printer->setFullPage(false);
    do {
        // Grab the image to be able to a suggest suitable orientation
        QString errorMessage;
        const QPixmap pixmap = m_fwm->createPreviewPixmap();
        if (pixmap.isNull()) {
            critical(tr("The image could not be created: %1").arg(errorMessage));
            break;
        }

        const QSizeF pixmapSize = pixmap.size();
        printer->setOrientation( pixmapSize.width() > pixmapSize.height() ?  QPrinter::Landscape :  QPrinter::Portrait);

        // Printer parameters
        QPrintDialog dialog(printer, fw);
        if (!dialog.exec())
           break;

        QWidget *mainWindow = ICore::mainWindow();
        const QCursor oldCursor = mainWindow->cursor();
        mainWindow->setCursor(Qt::WaitCursor);
        // Estimate of required scaling to make form look the same on screen and printer.
        const double suggestedScaling = static_cast<double>(printer->physicalDpiX()) /  static_cast<double>(fw->physicalDpiX());

        QPainter painter(printer);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // Clamp to page
        const QRectF page =  painter.viewport();
        const double maxScaling = qMin(page.size().width() / pixmapSize.width(), page.size().height() / pixmapSize.height());
        const double scaling = qMin(suggestedScaling, maxScaling);

        const double xOffset = page.left() + qMax(0.0, (page.size().width()  - scaling * pixmapSize.width())  / 2.0);
        const double yOffset = page.top()  + qMax(0.0, (page.size().height() - scaling * pixmapSize.height()) / 2.0);

        // Draw.
        painter.translate(xOffset, yOffset);
        painter.scale(scaling, scaling);
        painter.drawPixmap(0, 0, pixmap);
        mainWindow->setCursor(oldCursor);

    } while (false);
    printer->setFullPage(oldFullPage);
    printer->setOrientation(oldOrientation);
}

} // namespace Internal
} // namespace Designer
