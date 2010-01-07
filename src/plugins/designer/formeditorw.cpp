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

#include "formeditorw.h"
#include "formwindoweditor.h"
#include "designerconstants.h"
#include "settingsmanager.h"
#include "settingspage.h"
#include "editorwidget.h"
#include "qtcreatorintegration.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtDesigner/QDesignerFormEditorPluginInterface>
#include "qt_private/pluginmanager_p.h"

#include "qt_private/iconloader_p.h"  // createIconSet
#include "qt_private/qdesigner_formwindowmanager_p.h"
#include "qt_private/formwindowbase_p.h"
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerComponents>

#include <QtDesigner/QDesignerWidgetBoxInterface>
#include <QtDesigner/abstractobjectinspector.h>
#include <QtDesigner/QDesignerComponents>
#include <QtDesigner/QDesignerPropertyEditorInterface>
#include <QtDesigner/QDesignerActionEditorInterface>

#include <QtCore/QPluginLoader>
#include <QtCore/QTemporaryFile>
#include <QtCore/QDir>
#include <QtCore/QTime>
#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QDockWidget>
#include <QtGui/QMenu>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QKeySequence>
#include <QtGui/QPrintDialog>
#include <QtGui/QPrinter>
#include <QtGui/QPainter>
#include <QtGui/QStatusBar>
#include <QtGui/QStyle>
#include <QtGui/QToolBar>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

static const char *settingsGroup = "Designer";

#ifdef Q_OS_MAC
    enum { osMac = 1 };
#else
    enum { osMac = 0 };
#endif

/* Actions of the designer plugin:
 * Designer provides a toolbar which is subject to a context change (to
 * "edit mode" context) when it is focussed.
 * In order to prevent its actions from being disabled/hidden by that context
 * change, the actions are registered on the global context. In currentEditorChanged(),
 * the ones that are present in the global edit menu are set visible/invisible manually.
 * The designer context is currently used for Cut/Copy/Paste, etc. */

static inline QIcon designerIcon(const QString &iconName)
{
    const QIcon icon = qdesigner_internal::createIconSet(iconName);
    if (icon.isNull())
        qWarning() << "Unable to locate " << iconName;
    return icon;
}

// Create an action to activate a designer tool
static inline QAction *createEditModeAction(QActionGroup *ag,
                                     const QList<int> &context,
                                     Core::ActionManager *am,
                                     Core::ActionContainer *medit,
                                     const QString &actionName,
                                     const QString &name,
                                     int toolNumber,
                                     const QString &iconName =  QString(),
                                     const QString &keySequence = QString())
{
    QAction *rc = new QAction(actionName, ag);
    rc->setCheckable(true);
    if (!iconName.isEmpty())
         rc->setIcon(designerIcon(iconName));
    Core::Command *command = am->registerAction(rc, name, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    medit->addAction(command, Core::Constants::G_EDIT_OTHER);
    rc->setData(toolNumber);
    ag->addAction(rc);
    return rc;
}


// Create a menu separato
static inline QAction * createSeparator(QObject *parent,
                                 Core::ActionManager *am,
                                 const QList<int> &context,
                                 Core::ActionContainer *container,
                                 const QString &name = QString(),
                                 const QString &group = QString())
{
    QAction *actSeparator = new QAction(parent);
    actSeparator->setSeparator(true);
    Core::Command *command = am->registerAction(actSeparator, name, context);
    container->addAction(command, group);
    return actSeparator;
}

// Create a tool action
static inline void addToolAction(QAction *a,
                   Core::ActionManager *am,
                   const QList<int> &context,
                   const QString &name,
                   Core::ActionContainer *c1,
                   const QString &keySequence = QString())
{
    Core::Command *command = am->registerAction(a, name, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    c1->addAction(command);
}

using namespace Designer;
using namespace Designer::Internal;
using namespace Designer::Constants;

// --------- Proxy Action

ProxyAction::ProxyAction(const QString &defaultText, QObject *parent)
    : QAction(defaultText, parent),
    m_defaultText(defaultText),
    m_action(0)
{
    setEnabled(false);
}

void ProxyAction::setAction(QAction *action)
{
    if (m_action) {
        disconnect(m_action, SIGNAL(changed()), this, SLOT(update()));
        disconnect(this, SIGNAL(triggered(bool)), m_action, SIGNAL(triggered(bool)));
        disconnect(this, SIGNAL(toggled(bool)), m_action, SLOT(setChecked(bool)));
    }
    m_action = action;
    if (!m_action) {
        setEnabled(false);
//        if (hasAttribute(CA_Hide))
//            m_action->setVisible(false);
//        if (hasAttribute(CA_UpdateText)) {
            setText(m_defaultText);
//        }
    } else {
        setCheckable(m_action->isCheckable());
        setSeparator(m_action->isSeparator());
        connect(m_action, SIGNAL(changed()), this, SLOT(update()));
        // we want to avoid the toggling semantic on slot trigger(), so we just connect the signals
        connect(this, SIGNAL(triggered(bool)), m_action, SIGNAL(triggered(bool)));
        // we need to update the checked state, so we connect to setChecked slot, which also fires a toggled signal
        connect(this, SIGNAL(toggled(bool)), m_action, SLOT(setChecked(bool)));
        update();
    }
}

void ProxyAction::update()
{
    QTC_ASSERT(m_action, return)
    bool block = blockSignals(true);
//    if (hasAttribute(CA_UpdateIcon)) {
        setIcon(m_action->icon());
        setIconText(m_action->iconText());
//    }
//    if (hasAttribute(CA_UpdateText)) {
        setText(m_action->text());
        setToolTip(m_action->toolTip());
        setStatusTip(m_action->statusTip());
        setWhatsThis(m_action->whatsThis());
//    }

    setChecked(m_action->isChecked());

    setEnabled(m_action->isEnabled());
    setVisible(m_action->isVisible());
    blockSignals(block);
    emit changed();
}

// --------- FormEditorW

FormEditorW *FormEditorW::m_self = 0;

FormEditorW::FormEditorW() :
    m_formeditor(QDesignerComponents::createFormEditor(0)),
    m_integration(0),
    m_fwm(0),
    m_core(Core::ICore::instance()),
    m_initStage(RegisterPlugins),
    m_actionGroupEditMode(0),
    m_actionPrint(0),
    m_actionPreview(0),
    m_actionGroupPreviewInStyle(0),
    m_actionAboutPlugins(0)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO;
    QTC_ASSERT(!m_self, return);
    m_self = this;
    QTC_ASSERT(m_core, return);

    qFill(m_designerSubWindows, m_designerSubWindows + Designer::Constants::DesignerSubWindowCount,
          static_cast<QWidget *>(0));
    qFill(m_designerSubWindowActions, m_designerSubWindowActions + Designer::Constants::DesignerSubWindowCount,
          static_cast<ProxyAction *>(0));

    m_formeditor->setTopLevel(qobject_cast<QWidget *>(m_core->editorManager()));
    m_formeditor->setSettingsManager(new SettingsManager());

    m_fwm = qobject_cast<qdesigner_internal::QDesignerFormWindowManager*>(m_formeditor->formWindowManager());
    QTC_ASSERT(m_fwm, return);

    const int uid = m_core->uniqueIDManager()->uniqueIdentifier(QLatin1String(C_FORMEDITOR_ID));
    m_context << uid;

    setupActions();

    foreach (QDesignerOptionsPageInterface *designerPage, m_formeditor->optionsPages()) {
        SettingsPage *settingsPage = new SettingsPage(designerPage);
        ExtensionSystem::PluginManager::instance()->addObject(settingsPage);
        m_settingsPages.append(settingsPage);
    }
    restoreSettings(m_core->settings());

    connect(m_core->editorManager(), SIGNAL(currentEditorChanged(Core::IEditor *)),
            this, SLOT(currentEditorChanged(Core::IEditor *)));
}

FormEditorW::~FormEditorW()
{
    saveSettings(m_core->settings());

    for (int i = 0; i < Designer::Constants::DesignerSubWindowCount; ++i)
        delete m_designerSubWindows[i];

    delete m_formeditor;
    foreach (SettingsPage *settingsPage, m_settingsPages) {
        ExtensionSystem::PluginManager::instance()->removeObject(settingsPage);
        delete settingsPage;
    }
    delete  m_integration;
    m_self = 0;
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

    /**
     * This will initialize our TabOrder, Signals and slots and Buddy editors.
     */
    QList<QObject*> plugins = QPluginLoader::staticInstances();
    plugins += m_formeditor->pluginManager()->instances();
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

    m_initStage = FullyInitialized;
}

void FormEditorW::initDesignerSubWindows()
{
    qFill(m_designerSubWindows, m_designerSubWindows + Designer::Constants::DesignerSubWindowCount, static_cast<QWidget*>(0));

    QDesignerWidgetBoxInterface *wb = QDesignerComponents::createWidgetBox(m_formeditor, 0);
    wb->setWindowTitle(tr("Widget Box"));
    m_formeditor->setWidgetBox(wb);
    m_designerSubWindows[WidgetBoxSubWindow] = wb;

    QDesignerObjectInspectorInterface *oi = QDesignerComponents::createObjectInspector(m_formeditor, 0);
    oi->setWindowTitle(tr("Object Inspector"));
    m_formeditor->setObjectInspector(oi);
    m_designerSubWindows[ObjectInspectorSubWindow] = oi;

    QDesignerPropertyEditorInterface *pe = QDesignerComponents::createPropertyEditor(m_formeditor, 0);
    pe->setWindowTitle(tr("Property Editor"));
    m_formeditor->setPropertyEditor(pe);
    m_designerSubWindows[PropertyEditorSubWindow] = pe;

    QWidget *se = QDesignerComponents::createSignalSlotEditor(m_formeditor, 0);
    se->setWindowTitle(tr("Signals & Slots Editor"));
    m_designerSubWindows[SignalSlotEditorSubWindow] = se;

    QDesignerActionEditorInterface *ae = QDesignerComponents::createActionEditor(m_formeditor, 0);
    ae->setWindowTitle(tr("Action Editor"));
    m_formeditor->setActionEditor(ae);
    m_designerSubWindows[ActionEditorSubWindow] = ae;
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
    Core::ActionManager *am = m_core->actionManager();
    Core::Command *command;

    //menus
    Core::ActionContainer *medit =
        am->actionContainer(Core::Constants::M_EDIT);
    Core::ActionContainer *mtools =
        am->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *mformtools =
        am->createMenu(M_FORMEDITOR);
    mformtools->menu()->setTitle(tr("For&m editor"));
    mtools->addMenu(mformtools);

    //overridden actions
    am->registerAction(m_fwm->actionUndo(), Core::Constants::UNDO, m_context);
    am->registerAction(m_fwm->actionRedo(), Core::Constants::REDO, m_context);
    am->registerAction(m_fwm->actionCut(), Core::Constants::CUT, m_context);
    am->registerAction(m_fwm->actionCopy(), Core::Constants::COPY, m_context);
    am->registerAction(m_fwm->actionPaste(), Core::Constants::PASTE, m_context);
    am->registerAction(m_fwm->actionSelectAll(), Core::Constants::SELECTALL, m_context);

    m_actionPrint = new QAction(this);
    am->registerAction(m_actionPrint, Core::Constants::PRINT, m_context);
    connect(m_actionPrint, SIGNAL(triggered()), this, SLOT(print()));

    //'delete' action
    command = am->registerAction(m_fwm->actionDelete(), QLatin1String("FormEditor.Edit.Delete"), m_context);
    command->setDefaultKeySequence(QKeySequence::Delete);
    command->setAttribute(Core::Command::CA_Hide);
    medit->addAction(command, Core::Constants::G_EDIT_COPYPASTE);

    QList<int> globalcontext;
    globalcontext << m_core->uniqueIDManager()->uniqueIdentifier(Core::Constants::C_GLOBAL);

    m_actionGroupEditMode = new QActionGroup(this);
    m_actionGroupEditMode->setExclusive(true);
    connect(m_actionGroupEditMode, SIGNAL(triggered(QAction*)), this, SLOT(activateEditMode(QAction*)));

    m_modeActionSeparator = new QAction(this);
    m_modeActionSeparator->setSeparator(true);
    command = am->registerAction(m_modeActionSeparator, QLatin1String("FormEditor.Sep.ModeActions"), globalcontext);
    medit->addAction(command, Core::Constants::G_EDIT_OTHER);

    m_toolActionIds.push_back(QLatin1String("FormEditor.WidgetEditor"));
    createEditModeAction(m_actionGroupEditMode, globalcontext, am, medit,
                         tr("Edit widgets"), m_toolActionIds.back(),
                         EditModeWidgetEditor, QLatin1String("widgettool.png"), tr("F3"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.SignalsSlotsEditor"));
    createEditModeAction(m_actionGroupEditMode, globalcontext, am, medit,
                         tr("Edit signals/slots"), m_toolActionIds.back(),
                         EditModeSignalsSlotEditor, QLatin1String("signalslottool.png"), tr("F4"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.BuddyEditor"));
    createEditModeAction(m_actionGroupEditMode, globalcontext, am, medit,
                         tr("Edit buddies"), m_toolActionIds.back(),
                         EditModeBuddyEditor, QLatin1String("buddytool.png"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.TabOrderEditor"));
    createEditModeAction(m_actionGroupEditMode, globalcontext, am, medit,
                         tr("Edit tab order"),  m_toolActionIds.back(),
                         EditModeTabOrderEditor, QLatin1String("tabordertool.png"));

    //tool actions
    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutHorizontally"));
    const QString horizLayoutShortcut = osMac ? tr("Meta+H") : tr("Ctrl+H");
    addToolAction(m_fwm->actionHorizontalLayout(), am, globalcontext,
                  m_toolActionIds.back(), mformtools, horizLayoutShortcut);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutVertically"));
    const QString vertLayoutShortcut = osMac ? tr("Meta+L") : tr("Ctrl+L");
    addToolAction(m_fwm->actionVerticalLayout(), am, globalcontext,
                  m_toolActionIds.back(),  mformtools, vertLayoutShortcut);

    m_toolActionIds.push_back(QLatin1String("FormEditor.SplitHorizontal"));
    addToolAction(m_fwm->actionSplitHorizontal(), am, globalcontext,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.SplitVertical"));
    addToolAction(m_fwm->actionSplitVertical(), am, globalcontext,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutForm"));
    addToolAction(m_fwm->actionFormLayout(), am, globalcontext,
                  m_toolActionIds.back(),  mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutGrid"));
    const QString gridShortcut = osMac ? tr("Meta+G") : tr("Ctrl+G");
    addToolAction(m_fwm->actionGridLayout(), am, globalcontext,
                  m_toolActionIds.back(),  mformtools, gridShortcut);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutBreak"));
    addToolAction(m_fwm->actionBreakLayout(), am, globalcontext,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutAdjustSize"));
    const QString adjustShortcut = osMac ? tr("Meta+J") : tr("Ctrl+J");
    addToolAction(m_fwm->actionAdjustSize(), am, globalcontext,
                  m_toolActionIds.back(),  mformtools, adjustShortcut);

    m_toolActionIds.push_back(QLatin1String("FormEditor.SimplifyLayout"));
    addToolAction(m_fwm->actionSimplifyLayout(), am, globalcontext,
                  m_toolActionIds.back(),  mformtools);

    createSeparator(this, am, m_context, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator1"));

    addToolAction(m_fwm->actionLower(), am, globalcontext,
                  QLatin1String("FormEditor.Lower"), mformtools);

    addToolAction(m_fwm->actionRaise(), am, globalcontext,
                  QLatin1String("FormEditor.Raise"), mformtools);

    // Views
    createSeparator(this, am, globalcontext, mformtools, QLatin1String("FormEditor.Menu.Tools.SeparatorViews"));

    Core::ActionContainer *mviews = am->createMenu(M_FORMEDITOR_VIEWS);
    mviews->menu()->setTitle(tr("Views"));
    mformtools->addMenu(mviews);

    m_designerSubWindowActions[WidgetBoxSubWindow] = new ProxyAction(tr("Widget Box"), this);
    addToolAction(m_designerSubWindowActions[WidgetBoxSubWindow], am, globalcontext,
                  QLatin1String("FormEditor.WidgetBox"), mviews, "");

    m_designerSubWindowActions[ObjectInspectorSubWindow] = new ProxyAction(tr("Object Inspector"), this);
    addToolAction(m_designerSubWindowActions[ObjectInspectorSubWindow], am, globalcontext,
                  QLatin1String("FormEditor.ObjectInspector"), mviews, "");

        m_designerSubWindowActions[PropertyEditorSubWindow] = new ProxyAction(tr("Property Editor"), this);
    addToolAction(m_designerSubWindowActions[PropertyEditorSubWindow], am, globalcontext,
                  QLatin1String("FormEditor.PropertyEditor"), mviews, "");

    m_designerSubWindowActions[SignalSlotEditorSubWindow] = new ProxyAction(tr("Signals && Slots Editor"), this);
    addToolAction(m_designerSubWindowActions[SignalSlotEditorSubWindow], am, globalcontext,
                  QLatin1String("FormEditor.SignalsAndSlotsEditor"), mviews, "");

    m_designerSubWindowActions[ActionEditorSubWindow] = new ProxyAction(tr("Action Editor"), this);
    addToolAction(m_designerSubWindowActions[ActionEditorSubWindow], am, globalcontext,
                  QLatin1String("FormEditor.ActionEditor"), mviews, "");

    createSeparator(this, am, globalcontext, mviews, QLatin1String("FormEditor.Menu.Tools.Views.SeparatorLock"));

    m_lockAction = new QAction(tr("Locked"), this);
    m_lockAction->setCheckable(true);
    addToolAction(m_lockAction, am, globalcontext, QLatin1String("FormEditor.Locked"), mviews, "");
    connect(m_lockAction, SIGNAL(toggled(bool)), this, SLOT(setFormWindowLayoutLocked(bool)));

    createSeparator(this, am, globalcontext, mviews, QLatin1String("FormEditor.Menu.Tools.Views.SeparatorReset"));

    m_resetLayoutAction = new QAction(tr("Reset to Default Layout"), this);
    addToolAction(m_resetLayoutAction, am, globalcontext, QLatin1String("FormEditor.ResetToDefaultLayout"), mviews, "");
    connect(m_resetLayoutAction, SIGNAL(triggered()), this, SLOT(resetToDefaultLayout()));

    // Commands that do not go into the editor toolbar
    createSeparator(this, am, globalcontext, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator2"));

    m_actionPreview = m_fwm->actionDefaultPreview();
    QTC_ASSERT(m_actionPreview, return);
    addToolAction(m_actionPreview,  am,  globalcontext,
                   QLatin1String("FormEditor.Preview"), mformtools, tr("Ctrl+Alt+R"));

    // Preview in style...
    m_actionGroupPreviewInStyle = m_fwm->actionGroupPreviewInStyle();
    mformtools->addMenu(createPreviewStyleMenu(am, m_actionGroupPreviewInStyle));

    // Form settings
    createSeparator(this, am, m_context,  medit, QLatin1String("FormEditor.Edit.Separator2"), Core::Constants::G_EDIT_OTHER);

    createSeparator(this, am, globalcontext, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator3"));
    QAction *actionFormSettings = m_fwm->actionShowFormWindowSettingsDialog();
    addToolAction(actionFormSettings, am, globalcontext, QLatin1String("FormEditor.FormSettings"), mformtools);

#if QT_VERSION > 0x040500
    createSeparator(this, am, globalcontext, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator4"));
    m_actionAboutPlugins = new QAction(tr("About Qt Designer plugins...."), this);
    addToolAction(m_actionAboutPlugins,  am,  globalcontext,
                   QLatin1String("FormEditor.AboutPlugins"), mformtools);
    connect(m_actionAboutPlugins,  SIGNAL(triggered()), m_fwm, SLOT(aboutPlugins()));
    m_actionAboutPlugins->setEnabled(false);
#endif

    // FWM
    connect(m_fwm, SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface *)), this, SLOT(activeFormWindowChanged(QDesignerFormWindowInterface *)));
}


QToolBar *FormEditorW::createEditorToolBar() const
{
    QToolBar *toolBar = new QToolBar;
    Core::ActionManager *am = m_core->actionManager();
    const QStringList::const_iterator cend = m_toolActionIds.constEnd();
    for (QStringList::const_iterator it = m_toolActionIds.constBegin(); it != cend; ++it) {
        Core::Command *cmd = am->command(*it);
        QTC_ASSERT(cmd, continue);
        QAction *action = cmd->action();
        if (!action->icon().isNull()) // Simplify grid has no action yet
            toolBar->addAction(action);
    }
    int size = toolBar->style()->pixelMetric(QStyle::PM_SmallIconSize);
    toolBar->setIconSize(QSize(size, size));
    toolBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    return toolBar;
}

Core::ActionContainer *FormEditorW::createPreviewStyleMenu(Core::ActionManager *am,
                                                            QActionGroup *actionGroup)
{
    const QString menuId = QLatin1String(M_FORMEDITOR_PREVIEW);
    Core::ActionContainer *menuPreviewStyle = am->createMenu(menuId);
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
        Core::Command *command = am->registerAction(a, name, m_context);
        if (isDeviceProfile) {
            command->setAttribute(Core::Command::CA_UpdateText);
            command->setAttribute(Core::Command::CA_NonConfigureable);
        }
        menuPreviewStyle->addAction(command);
    }
    return menuPreviewStyle;
}

void FormEditorW::saveSettings(QSettings *s)
{
    s->beginGroup(settingsGroup);
    EditorWidget::saveState(s);
    s->endGroup();
}

void FormEditorW::restoreSettings(QSettings *s)
{
    s->beginGroup(settingsGroup);
    EditorWidget::restoreState(s);
    s->endGroup();
}


void FormEditorW::critical(const QString &errorMessage)
{
    QMessageBox::critical(m_core->mainWindow(), tr("Designer"),  errorMessage);
}

FormWindowEditor *FormEditorW::createFormWindowEditor(QWidget* parentWidget)
{
    m_fwm->closeAllPreviews();
    QDesignerFormWindowInterface *form = m_fwm->createFormWindow(0);
    connect(form, SIGNAL(toolChanged(int)), this, SLOT(toolChanged(int)));
    qdesigner_internal::FormWindowBase::setupDefaultAction(form);
    FormWindowEditor *fww = new FormWindowEditor(m_context, form, parentWidget);
    // Store a pointer to all form windows so we can unselect
    // all other formwindows except the active one.
    m_formWindows.append(fww);
    connect(fww, SIGNAL(destroyed()), this, SLOT(editorDestroyed()));
    return fww;
}

void FormEditorW::editorDestroyed()
{
    QObject *source = sender();

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << source;

    for (EditorList::iterator it = m_formWindows.begin(); it != m_formWindows.end(); ) {
        if (*it == source) {
            it = m_formWindows.erase(it);
            break;
        } else {
            ++it;
        }
    }
}

void FormEditorW::currentEditorChanged(Core::IEditor *editor)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << editor << " of " << m_fwm->formWindowCount();

    // Deactivate Designer if a non-form is being edited
    if (editor && editor->id() == QLatin1String(Constants::C_FORMEDITOR_ID)) {
        FormWindowEditor *fw = qobject_cast<FormWindowEditor *>(editor);
        QTC_ASSERT(fw, return);
        fw->activate();
        m_fwm->setActiveFormWindow(fw->formWindow());
        m_actionGroupEditMode->setVisible(true);
        m_modeActionSeparator->setVisible(true);
        QDockWidget * const*dockWidgets = fw->dockWidgets();
        for (int i = 0; i < Designer::Constants::DesignerSubWindowCount; ++i) {
            if (m_designerSubWindowActions[i] != 0 && dockWidgets[i] != 0)
                m_designerSubWindowActions[i]->setAction(dockWidgets[i]->toggleViewAction());
        }
        m_lockAction->setEnabled(true);
        m_lockAction->setChecked(fw->isLocked());
        m_resetLayoutAction->setEnabled(true);
    } else {
        m_actionGroupEditMode->setVisible(false);
        m_modeActionSeparator->setVisible(false);
        m_fwm->setActiveFormWindow(0);
        for (int i = 0; i < Designer::Constants::DesignerSubWindowCount; ++i) {
            if (m_designerSubWindowActions[i] != 0)
                m_designerSubWindowActions[i]->setAction(0);
        }
        m_lockAction->setEnabled(false);
        m_resetLayoutAction->setEnabled(false);
    }
}

void FormEditorW::activeFormWindowChanged(QDesignerFormWindowInterface *afw)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << afw << " of " << m_fwm->formWindowCount() << m_formWindows;

    m_fwm->closeAllPreviews();

    bool foundFormWindow = false;
    // Display form selection handles only on active window
    EditorList::const_iterator cend = m_formWindows.constEnd();
    for (EditorList::const_iterator it = m_formWindows.constBegin(); it != cend ; ++it) {
        FormWindowEditor *fwe = *it;
        const bool active = fwe->formWindow() == afw;
        if (active)
            foundFormWindow = true;
        fwe->updateFormWindowSelectionHandles(active);
    }

    m_actionPreview->setEnabled(foundFormWindow);
    m_actionGroupPreviewInStyle->setEnabled(foundFormWindow);
}

FormWindowEditor *FormEditorW::activeFormWindow()
{
    QDesignerFormWindowInterface *afw = m_fwm->activeFormWindow();
    for (int i = 0; i < m_formWindows.count(); ++i) {
        if (FormWindowEditor *fw = m_formWindows[i]) {
            QDesignerFormWindowInterface *fwd = fw->formWindow();
            if (fwd == afw) {
                return fw;
            }
        }
    }
    return 0;
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

void FormEditorW::setFormWindowLayoutLocked(bool locked)
{
    FormWindowEditor *fwe = activeFormWindow();
    if (fwe)
        fwe->setLocked(locked);
}

void FormEditorW::resetToDefaultLayout()
{
    FormWindowEditor *fwe = activeFormWindow();
    if (fwe)
        fwe->resetToDefaultLayout();
}

void FormEditorW::print()
{
    // Printing code courtesy of designer_actions.cpp
    QDesignerFormWindowInterface *fw = m_fwm->activeFormWindow();
    if (!fw)
        return;

    const bool oldFullPage =  m_core->printer()->fullPage();
    const QPrinter::Orientation oldOrientation =  m_core->printer()->orientation ();
    m_core->printer()->setFullPage(false);
    do {

        // Grab the image to be able to a suggest suitable orientation
        QString errorMessage;
        const QPixmap pixmap = m_fwm->createPreviewPixmap(&errorMessage);
        if (pixmap.isNull()) {
            critical(tr("The image could not be created: %1").arg(errorMessage));
            break;
        }

        const QSizeF pixmapSize = pixmap.size();
        m_core->printer()->setOrientation( pixmapSize.width() > pixmapSize.height() ?  QPrinter::Landscape :  QPrinter::Portrait);

        // Printer parameters
        QPrintDialog dialog(m_core->printer(), fw);
        if (!dialog.exec())
           break;

        const QCursor oldCursor = m_core->mainWindow()->cursor();
        m_core->mainWindow()->setCursor(Qt::WaitCursor);
        // Estimate of required scaling to make form look the same on screen and printer.
        const double suggestedScaling = static_cast<double>(m_core->printer()->physicalDpiX()) /  static_cast<double>(fw->physicalDpiX());

        QPainter painter(m_core->printer());
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
        m_core->mainWindow()->setCursor(oldCursor);

    } while (false);
    m_core->printer()->setFullPage(oldFullPage);
    m_core->printer()->setOrientation(oldOrientation);
}
