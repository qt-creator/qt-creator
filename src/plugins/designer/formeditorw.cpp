/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "formeditorw.h"
#include "formwindoweditor.h"
#include "settingsmanager.h"
#include "settingspage.h"
#include "editorwidget.h"
#include "editordata.h"
#include "qtcreatorintegration.h"
#include "designerxmleditorwidget.h"
#include "designercontext.h"
#include "resourcehandler.h"
#include <widgethost.h>

#include <coreplugin/editortoolbar.h>
#include <coreplugin/designmode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/outputpane.h>
#include <texteditor/texteditorsettings.h>
#include <utils/qtcassert.h>

#include <QDesignerFormEditorPluginInterface>
#include <QDesignerFormEditorInterface>
#include <QDesignerComponents>

#if QT_VERSION >= 0x050000
#    include <QDesignerFormWindowManagerInterface>
#else
#    include "qt_private/pluginmanager_p.h"
#    include "qt_private/iconloader_p.h"  // createIconSet
#    include "qt_private/qdesigner_formwindowmanager_p.h"
#    include "qt_private/formwindowbase_p.h"
#endif

#include <QDesignerWidgetBoxInterface>
#include <abstractobjectinspector.h>
#include <QDesignerPropertyEditorInterface>
#include <QDesignerActionEditorInterface>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCursor>
#include <QDockWidget>
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
#include <QSignalMapper>
#include <QPluginLoader>
#include <QTime>

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
#if QT_VERSION >= 0x050000
    const QIcon icon = QDesignerFormEditorInterface::createIcon(iconName);
#else
    const QIcon icon = qdesigner_internal::createIconSet(iconName);
#endif
    if (icon.isNull())
        qWarning() << "Unable to locate " << iconName;
    return icon;
}

using namespace Core;
using namespace Designer::Constants;

namespace Designer {
namespace Internal {

// --------- FormEditorW

FormEditorW *FormEditorW::m_self = 0;

FormEditorW::FormEditorW() :
    m_formeditor(QDesignerComponents::createFormEditor(0)),
    m_integration(0),
    m_fwm(0),
    m_initStage(RegisterPlugins),
    m_actionGroupEditMode(0),
    m_actionPrint(0),
    m_actionPreview(0),
    m_actionGroupPreviewInStyle(0),
    m_previewInStyleMenu(0),
    m_actionAboutPlugins(0),
    m_shortcutMapper(new QSignalMapper(this)),
    m_context(0),
    m_modeWidget(0),
    m_editorWidget(0),
    m_designMode(0),
    m_editorToolBar(0),
    m_toolBar(0)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO;
    QTC_ASSERT(!m_self, return);
    m_self = this;

    qFill(m_designerSubWindows, m_designerSubWindows + Designer::Constants::DesignerSubWindowCount,
          static_cast<QWidget *>(0));

    m_formeditor->setTopLevel(ICore::mainWindow());
    m_formeditor->setSettingsManager(new SettingsManager());

#if QT_VERSION >= 0x050000
    m_fwm = m_formeditor->formWindowManager();
#else
    m_fwm = qobject_cast<qdesigner_internal::QDesignerFormWindowManager*>(m_formeditor->formWindowManager());
#endif
    QTC_ASSERT(m_fwm, return);

    m_contexts.add(Designer::Constants::C_FORMEDITOR);

    setupActions();

    foreach (QDesignerOptionsPageInterface *designerPage, m_formeditor->optionsPages()) {
        SettingsPage *settingsPage = new SettingsPage(designerPage);
        m_settingsPages.append(settingsPage);
    }

    connect(EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(currentEditorChanged(Core::IEditor*)));
    connect(m_shortcutMapper, SIGNAL(mapped(QObject*)),
            this, SLOT(updateShortcut(QObject*)));
}

FormEditorW::~FormEditorW()
{
    if (m_context)
        ICore::removeContextObject(m_context);
    if (m_initStage == FullyInitialized) {
        QSettings *s = ICore::settings();
        s->beginGroup(QLatin1String(settingsGroupC));
        m_editorWidget->saveSettings(s);
        s->endGroup();

        m_designMode->unregisterDesignWidget(m_modeWidget);
        delete m_modeWidget;
        m_modeWidget = 0;
    }

    delete m_formeditor;
    qDeleteAll(m_settingsPages);
    m_settingsPages.clear();
    delete m_integration;

    m_self = 0;
}

// Add an actioon to toggle the view state of a dock window
void FormEditorW::addDockViewAction(ActionContainer *viewMenu,
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

void FormEditorW::setupViewActions()
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

    cmd = addToolAction(m_editorWidget->toggleLockedAction(), m_contexts, "FormEditor.Locked", viewMenu);
    cmd->setAttribute(Command::CA_Hide);

    cmd = addToolAction(m_editorWidget->menuSeparator2(), m_contexts, "FormEditor.SeparatorReset", viewMenu);
    cmd->setAttribute(Command::CA_Hide);

    cmd = addToolAction(m_editorWidget->resetLayoutAction(), m_contexts, "FormEditor.ResetToDefaultLayout", viewMenu);
    connect(m_editorWidget, SIGNAL(resetLayout()), m_editorWidget, SLOT(resetToDefaultLayout()));
    cmd->setAttribute(Command::CA_Hide);
}

void FormEditorW::fullInit()
{
    QTC_ASSERT(m_initStage == RegisterPlugins, return);
    QTime *initTime = 0;
    if (Designer::Constants::Internal::debug) {
        initTime = new QTime;
        initTime->start();
    }

    QDesignerComponents::createTaskMenu(m_formeditor, parent());
    QDesignerComponents::initializePlugins(designerEditor());
    QDesignerComponents::initializeResources();
    initDesignerSubWindows();
    m_integration = new QtCreatorIntegration(m_formeditor, this);
    m_formeditor->setIntegration(m_integration);
    // Connect Qt Designer help request to HelpManager.
    connect(m_integration, SIGNAL(creatorHelpRequested(QUrl)),
        HelpManager::instance(), SLOT(handleHelpRequest(QUrl)));

    /**
     * This will initialize our TabOrder, Signals and slots and Buddy editors.
     */
    QList<QObject*> plugins = QPluginLoader::staticInstances();
#if QT_VERSION >= 0x050000
    plugins += m_formeditor->pluginInstances();
#else
    plugins += m_formeditor->pluginManager()->instances();
#endif
    foreach (QObject *plugin, plugins) {
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

    connect(EditorManager::instance(), SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            SLOT(closeFormEditorsForXmlEditors(QList<Core::IEditor*>)));
    // Nest toolbar and editor widget
    m_editorWidget = new EditorWidget(this);
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String(settingsGroupC));
    m_editorWidget->restoreSettings(settings);
    settings->endGroup();

    m_editorToolBar = createEditorToolBar();
    m_toolBar = EditorManager::createToolBar();
    m_toolBar->setToolbarCreationFlags(EditorToolBar::FlagsStandalone);
    m_toolBar->setNavigationVisible(false);
    m_toolBar->addCenterToolBar(m_editorToolBar);

    m_designMode = DesignMode::instance();
    m_modeWidget = new QWidget;
    m_modeWidget->setObjectName(QLatin1String("DesignerModeWidget"));
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_toolBar);
    // Avoid mode switch to 'Edit' mode when the application started by
    // 'Run' in 'Design' mode emits output.
    MiniSplitter *splitter = new MiniSplitter(Qt::Vertical);
    splitter->addWidget(m_editorWidget);
    QWidget *outputPane = new OutputPanePlaceHolder(m_designMode, splitter);
    outputPane->setObjectName(QLatin1String("DesignerOutputPanePlaceHolder"));
    splitter->addWidget(outputPane);
    layout->addWidget(splitter);
    m_modeWidget->setLayout(layout);

    Context designerContexts = m_contexts;
    designerContexts.add(Core::Constants::C_EDITORMANAGER);
    m_context = new DesignerContext(designerContexts, m_modeWidget, this);
    ICore::addContextObject(m_context);

    m_designMode->registerDesignWidget(m_modeWidget, QStringList(QLatin1String(FORM_MIMETYPE)), m_contexts);

    setupViewActions();

    m_initStage = FullyInitialized;
}

void FormEditorW::initDesignerSubWindows()
{
    qFill(m_designerSubWindows, m_designerSubWindows + Designer::Constants::DesignerSubWindowCount, static_cast<QWidget*>(0));

    QDesignerWidgetBoxInterface *wb = QDesignerComponents::createWidgetBox(m_formeditor, 0);
    wb->setWindowTitle(tr("Widget Box"));
    wb->setObjectName(QLatin1String("WidgetBox"));
    m_formeditor->setWidgetBox(wb);
    m_designerSubWindows[WidgetBoxSubWindow] = wb;

    QDesignerObjectInspectorInterface *oi = QDesignerComponents::createObjectInspector(m_formeditor, 0);
    oi->setWindowTitle(tr("Object Inspector"));
    oi->setObjectName(QLatin1String("ObjectInspector"));
    m_formeditor->setObjectInspector(oi);
    m_designerSubWindows[ObjectInspectorSubWindow] = oi;

    QDesignerPropertyEditorInterface *pe = QDesignerComponents::createPropertyEditor(m_formeditor, 0);
    pe->setWindowTitle(tr("Property Editor"));
    pe->setObjectName(QLatin1String("PropertyEditor"));
    m_formeditor->setPropertyEditor(pe);
    m_designerSubWindows[PropertyEditorSubWindow] = pe;

    QWidget *se = QDesignerComponents::createSignalSlotEditor(m_formeditor, 0);
    se->setWindowTitle(tr("Signals && Slots Editor"));
    se->setObjectName(QLatin1String("SignalsAndSlotsEditor"));
    m_designerSubWindows[SignalSlotEditorSubWindow] = se;

    QDesignerActionEditorInterface *ae = QDesignerComponents::createActionEditor(m_formeditor, 0);
    ae->setWindowTitle(tr("Action Editor"));
    ae->setObjectName(QLatin1String("ActionEditor"));
    m_formeditor->setActionEditor(ae);
    m_designerSubWindows[ActionEditorSubWindow] = ae;
}

QList<Core::IOptionsPage *> FormEditorW::optionsPages() const
{
    return m_settingsPages;
}

void FormEditorW::ensureInitStage(InitializationStage s)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << s;
    if (!m_self)
        m_self = new FormEditorW;
    if (m_self->m_initStage >= s)
        return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_self->fullInit();
    QApplication::restoreOverrideCursor();
}

FormEditorW *FormEditorW::instance()
{
    ensureInitStage(FullyInitialized);
    return m_self;
}

void FormEditorW::deleteInstance()
{
    delete m_self;
}

void FormEditorW::setupActions()
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

    m_actionPrint = new QAction(this);
    bindShortcut(ActionManager::registerAction(m_actionPrint, Core::Constants::PRINT, m_contexts), m_actionPrint);
    connect(m_actionPrint, SIGNAL(triggered()), this, SLOT(print()));

    //'delete' action. Do not set a shortcut as Designer handles
    // the 'Delete' key by event filter. Setting a shortcut triggers
    // buggy behaviour on Mac (Pressing Delete in QLineEdit removing the widget).
    Command *command;
    command = ActionManager::registerAction(m_fwm->actionDelete(), "FormEditor.Edit.Delete", m_contexts);
    bindShortcut(command, m_fwm->actionDelete());
    command->setAttribute(Command::CA_Hide);
    medit->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    m_actionGroupEditMode = new QActionGroup(this);
    m_actionGroupEditMode->setExclusive(true);
    connect(m_actionGroupEditMode, SIGNAL(triggered(QAction*)), this, SLOT(activateEditMode(QAction*)));

    medit->addSeparator(m_contexts, Core::Constants::G_EDIT_OTHER);

    m_toolActionIds.push_back("FormEditor.WidgetEditor");
    createEditModeAction(m_actionGroupEditMode, m_contexts, medit,
                         tr("Edit Widgets"), m_toolActionIds.back(),
                         EditModeWidgetEditor, QLatin1String("widgettool.png"), tr("F3"));

    m_toolActionIds.push_back("FormEditor.SignalsSlotsEditor");
    createEditModeAction(m_actionGroupEditMode, m_contexts, medit,
                         tr("Edit Signals/Slots"), m_toolActionIds.back(),
                         EditModeSignalsSlotEditor, QLatin1String("signalslottool.png"), tr("F4"));

    m_toolActionIds.push_back("FormEditor.BuddyEditor");
    createEditModeAction(m_actionGroupEditMode, m_contexts, medit,
                         tr("Edit Buddies"), m_toolActionIds.back(),
                         EditModeBuddyEditor, QLatin1String("buddytool.png"));

    m_toolActionIds.push_back("FormEditor.TabOrderEditor");
    createEditModeAction(m_actionGroupEditMode, m_contexts, medit,
                         tr("Edit Tab Order"),  m_toolActionIds.back(),
                         EditModeTabOrderEditor, QLatin1String("tabordertool.png"));

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

#if QT_VERSION >= 0x050000
    m_actionPreview = m_fwm->action(QDesignerFormWindowManagerInterface::DefaultPreviewAction);
#else
    m_actionPreview = m_fwm->actionDefaultPreview();
#endif
    QTC_ASSERT(m_actionPreview, return);
    addToolAction(m_actionPreview, m_contexts, "FormEditor.Preview", mformtools, tr("Alt+Shift+R"));

    // Preview in style...
#if QT_VERSION >= 0x050000
    m_actionGroupPreviewInStyle = m_fwm->actionGroup(QDesignerFormWindowManagerInterface::StyledPreviewActionGroup);
#else
    m_actionGroupPreviewInStyle = m_fwm->actionGroupPreviewInStyle();
#endif

    ActionContainer *previewAC = createPreviewStyleMenu(m_actionGroupPreviewInStyle);
    m_previewInStyleMenu = previewAC->menu();
    mformtools->addMenu(previewAC);
    setPreviewMenuEnabled(false);

    // Form settings
    medit->addSeparator(m_contexts, Core::Constants::G_EDIT_OTHER);

    mformtools->addSeparator(m_contexts);

    mformtools->addSeparator(m_contexts, Core::Constants::G_DEFAULT_THREE);
#if QT_VERSION >= 0x050000
    QAction *actionFormSettings = m_fwm->action(QDesignerFormWindowManagerInterface::FormWindowSettingsDialogAction);
#else
    QAction *actionFormSettings = m_fwm->actionShowFormWindowSettingsDialog();
#endif
    addToolAction(actionFormSettings, m_contexts, "FormEditor.FormSettings", mformtools,
                  QString(), Core::Constants::G_DEFAULT_THREE);

    mformtools->addSeparator(m_contexts, Core::Constants::G_DEFAULT_THREE);
    m_actionAboutPlugins = new QAction(tr("About Qt Designer Plugins..."), this);
    m_actionAboutPlugins->setMenuRole(QAction::NoRole);
    addToolAction(m_actionAboutPlugins, m_contexts, "FormEditor.AboutPlugins", mformtools,
                  QString(), Core::Constants::G_DEFAULT_THREE);
    connect(m_actionAboutPlugins,  SIGNAL(triggered()), m_fwm,
#if QT_VERSION >= 0x050000
            SLOT(showPluginDialog())
#else
            SLOT(aboutPlugins())
#endif
            );
    m_actionAboutPlugins->setEnabled(false);

    // FWM
    connect(m_fwm, SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface*)), this, SLOT(activeFormWindowChanged(QDesignerFormWindowInterface*)));
}

QToolBar *FormEditorW::createEditorToolBar() const
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

ActionContainer *FormEditorW::createPreviewStyleMenu(QActionGroup *actionGroup)
{
    const QString menuId = QLatin1String(M_FORMEDITOR_PREVIEW);
    ActionContainer *menuPreviewStyle = ActionManager::createMenu(M_FORMEDITOR_PREVIEW);
    menuPreviewStyle->menu()->setTitle(tr("Preview in"));

    // The preview menu is a list of invisible actions for the embedded design
    // device profiles (integer data) followed by a separator and the styles
    // (string data). Make device profiles update their text and hide them
    // in the configuration dialog.
    const QList<QAction*> actions = actionGroup->actions();

    const QString deviceProfilePrefix = QLatin1String("DeviceProfile");
    const QChar dot = QLatin1Char('.');

    foreach (QAction* a, actions) {
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

void FormEditorW::setPreviewMenuEnabled(bool e)
{
    m_actionPreview->setEnabled(e);
    m_previewInStyleMenu->setEnabled(e);
}

void FormEditorW::saveSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(settingsGroupC));
    m_editorWidget->saveSettings(s);
    s->endGroup();
}

void FormEditorW::critical(const QString &errorMessage)
{
    QMessageBox::critical(ICore::mainWindow(), tr("Designer"),  errorMessage);
}

// Apply the command shortcut to the action and connects to the command's keySequenceChanged signal
void FormEditorW::bindShortcut(Command *command, QAction *action)
{
    m_commandToDesignerAction.insert(command, action);
    connect(command, SIGNAL(keySequenceChanged()),
            m_shortcutMapper, SLOT(map()));
    m_shortcutMapper->setMapping(command, command);
    updateShortcut(command);
}

// Create an action to activate a designer tool
QAction *FormEditorW::createEditModeAction(QActionGroup *ag,
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
    command->setAttribute(Core::Command::CA_Hide);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    bindShortcut(command, rc);
    medit->addAction(command, Core::Constants::G_EDIT_OTHER);
    rc->setData(toolNumber);
    ag->addAction(rc);
    return rc;
}

// Create a tool action
Command *FormEditorW::addToolAction(QAction *a, const Context &context, Id id,
                                          ActionContainer *c1, const QString &keySequence,
                                    Core::Id groupId)
{
    Command *command = ActionManager::registerAction(a, id, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    if (!a->isSeparator())
        bindShortcut(command, a);
    c1->addAction(command, groupId);
    return command;
}

EditorData FormEditorW::createEditor(QWidget *parent)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormEditorW::createEditor";
    // Create and associate form and text editor.
    EditorData data;
    m_fwm->closeAllPreviews();
#if QT_VERSION >= 0x050000
    QDesignerFormWindowInterface *form = m_fwm->createFormWindow(0);
#else
    qdesigner_internal::FormWindowBase *form = qobject_cast<qdesigner_internal::FormWindowBase *>(m_fwm->createFormWindow(0));
#endif
    QTC_ASSERT(form, return data);
    connect(form, SIGNAL(toolChanged(int)), this, SLOT(toolChanged(int)));
    ResourceHandler *resourceHandler = new ResourceHandler(form);
#if QT_VERSION < 0x050000
    form->setDesignerGrid(qdesigner_internal::FormWindowBase::defaultDesignerGrid());
    qdesigner_internal::FormWindowBase::setupDefaultAction(form);
#endif
    data.widgetHost = new SharedTools::WidgetHost( /* parent */ 0, form);
    DesignerXmlEditorWidget *xmlEditor = new DesignerXmlEditorWidget(form, parent);
    TextEditor::TextEditorSettings::initializeEditor(xmlEditor);
    data.formWindowEditor = xmlEditor->designerEditor();
    connect(data.formWindowEditor->document(), SIGNAL(filePathChanged(QString,QString)),
            resourceHandler, SLOT(updateResources()));
    m_editorWidget->add(data);

    m_toolBar->addEditor(xmlEditor->editor());

    return data;
}

void FormEditorW::updateShortcut(QObject *command)
{
    Command *c = qobject_cast<Command *>(command);
    if (!c)
        return;
    QAction *a = m_commandToDesignerAction.value(c);
    if (!a)
        return;
    a->setShortcut(c->action()->shortcut());
}

void FormEditorW::currentEditorChanged(IEditor *editor)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << editor << " of " << m_fwm->formWindowCount();

    if (editor && editor->document()->id() == Constants::K_DESIGNER_XML_EDITOR_ID) {
        FormWindowEditor *xmlEditor = qobject_cast<FormWindowEditor *>(editor);
        QTC_ASSERT(xmlEditor, return);
        ensureInitStage(FullyInitialized);
        SharedTools::WidgetHost *fw = m_editorWidget->formWindowEditorForXmlEditor(xmlEditor);
        QTC_ASSERT(fw, return);
        m_editorWidget->setVisibleEditor(xmlEditor);
        m_fwm->setActiveFormWindow(fw->formWindow());
    }
}

void FormEditorW::activeFormWindowChanged(QDesignerFormWindowInterface *afw)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << afw << " of " << m_fwm->formWindowCount();

    m_fwm->closeAllPreviews();
    setPreviewMenuEnabled(afw != 0);
}

EditorData FormEditorW::activeEditor() const
{
    if (m_editorWidget)
        return m_editorWidget->activeEditor();
    return EditorData();
}

void FormEditorW::activateEditMode(int id)
{
    if (const int count = m_fwm->formWindowCount())
        for (int i = 0; i <  count; i++)
             m_fwm->formWindow(i)->setCurrentTool(id);
}

void FormEditorW::activateEditMode(QAction* a)
{
    activateEditMode(a->data().toInt());
}

void FormEditorW::toolChanged(int t)
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

void FormEditorW::closeFormEditorsForXmlEditors(QList<IEditor*> editors)
{
    foreach (IEditor *editor, editors)
        m_editorWidget->removeFormWindowEditor(editor);
}

void FormEditorW::print()
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
#if QT_VERSION >= 0x050000
        const QPixmap pixmap = m_fwm->createPreviewPixmap();
#else
        const QPixmap pixmap = m_fwm->createPreviewPixmap(&errorMessage);
#endif
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
