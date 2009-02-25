/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "formeditorw.h"
#include "formwindoweditor.h"
#include "designerconstants.h"
#include "settingsmanager.h"
#include "settingspage.h"
#include "editorwidget.h"
#include "workbenchintegration.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtDesigner/QDesignerFormEditorPluginInterface>
#include <qt_private/pluginmanager_p.h>

#include <qt_private/iconloader_p.h>  // createIconSet
#include <qt_private/qdesigner_formwindowmanager_p.h>
#include <qt_private/formwindowbase_p.h>
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

static const char *editorWidgetStateKeyC = "editorWidgetState";
static const char *settingsGroup = "Designer";

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
    command->setAttribute(Core::Command::CA_Hide);
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

// --------- FormEditorW

using namespace Designer::Internal;
using namespace Designer::Constants;

FormEditorW *FormEditorW::m_self = 0;

FormEditorW::FormEditorW() :
    m_formeditor(QDesignerComponents::createFormEditor(0)),
    m_integration(0),
    m_fwm(0),
    m_core(Core::ICore::instance()),
    m_initStage(RegisterPlugins),
    m_actionGroupEditMode(0),
    m_actionPrint(0)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO;
    QTC_ASSERT(!m_self, return);
    m_self = this;
    QTC_ASSERT(m_core, return);

    qFill(m_designerSubWindows, m_designerSubWindows + Designer::Constants::DesignerSubWindowCount,
          static_cast<QWidget *>(0));

    m_formeditor->setTopLevel(qobject_cast<QWidget *>(m_core->editorManager()));
    m_formeditor->setSettingsManager(new SettingsManager());

    m_fwm = qobject_cast<qdesigner_internal::QDesignerFormWindowManager*>(m_formeditor->formWindowManager());
    QTC_ASSERT(m_fwm, return);

    const int uid = m_core->uniqueIDManager()->uniqueIdentifier(QLatin1String(C_FORMEDITOR));
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
    m_integration = new WorkbenchIntegration(m_formeditor, this);
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
    wb->setWindowTitle(tr("Designer widgetbox"));
    m_formeditor->setWidgetBox(wb);
    m_designerSubWindows[WidgetBoxSubWindow] = wb;

    QDesignerObjectInspectorInterface *oi = QDesignerComponents::createObjectInspector(m_formeditor, 0);
    oi->setWindowTitle(tr("Object inspector"));
    m_formeditor->setObjectInspector(oi);
    m_designerSubWindows[ObjectInspectorSubWindow] = oi;

    QDesignerPropertyEditorInterface *pe = QDesignerComponents::createPropertyEditor(m_formeditor, 0);
    pe->setWindowTitle(tr("Property editor"));
    m_formeditor->setPropertyEditor(pe);
    m_designerSubWindows[PropertyEditorSubWindow] = pe;

    QWidget *se = QDesignerComponents::createSignalSlotEditor(m_formeditor, 0);
    se->setWindowTitle(tr("Signals and slots editor"));
    m_designerSubWindows[SignalSlotEditorSubWindow] = se;

    QDesignerActionEditorInterface *ae = QDesignerComponents::createActionEditor(m_formeditor, 0);
    ae->setWindowTitle(tr("Action editor"));
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

    //editor Modes. Store ids for editor tool bars
    m_actionGroupEditMode = new QActionGroup(this);
    m_actionGroupEditMode->setExclusive(true);
    connect(m_actionGroupEditMode, SIGNAL(triggered(QAction*)), this, SLOT(activateEditMode(QAction*)));

    m_toolActionIds.push_back(QLatin1String("FormEditor.WidgetEditor"));
    createEditModeAction(m_actionGroupEditMode, m_context, am, medit,
                         QLatin1String("Edit widgets"), m_toolActionIds.back(),
                         EditModeWidgetEditor, QLatin1String("widgettool.png"), tr("F3"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.SignalsSlotsEditor"));
    createEditModeAction(m_actionGroupEditMode,  m_context, am, medit,
                         QLatin1String("Edit signals/slots"), m_toolActionIds.back(),
                         EditModeSignalsSlotEditor, QLatin1String("signalslottool.png"), tr("F4"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.BuddyEditor"));
    createEditModeAction(m_actionGroupEditMode,  m_context, am, medit,
                         QLatin1String("Edit buddies"), m_toolActionIds.back(),
                         EditModeBuddyEditor, QLatin1String("buddytool.png"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.TabOrderEditor"));
    createEditModeAction(m_actionGroupEditMode,  m_context, am, medit,
                         QLatin1String("Edit tab order"),  m_toolActionIds.back(),
                         EditModeTabOrderEditor, QLatin1String("tabordertool.png"));

    //tool actions
    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutHorizontally"));
    addToolAction(m_fwm->actionHorizontalLayout(), am, m_context,
                  m_toolActionIds.back(), mformtools, tr("Ctrl+H"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutVertically"));
    addToolAction(m_fwm->actionVerticalLayout(), am, m_context,
                  m_toolActionIds.back(),  mformtools, tr("Ctrl+L"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.SplitHorizontal"));
    addToolAction(m_fwm->actionSplitHorizontal(), am, m_context,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.SplitVertical"));
    addToolAction(m_fwm->actionSplitVertical(), am, m_context,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutForm"));
    addToolAction(m_fwm->actionFormLayout(), am, m_context,
                  m_toolActionIds.back(),  mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutGrid"));
    addToolAction(m_fwm->actionGridLayout(), am, m_context,
                  m_toolActionIds.back(),  mformtools, tr("Ctrl+G"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutBreak"));
    addToolAction(m_fwm->actionBreakLayout(), am, m_context,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back(QLatin1String("FormEditor.LayoutAdjustSize"));
    addToolAction(m_fwm->actionAdjustSize(), am, m_context,
                  m_toolActionIds.back(),  mformtools, tr("Ctrl+J"));

    m_toolActionIds.push_back(QLatin1String("FormEditor.SimplifyLayout"));
    addToolAction(m_fwm->actionSimplifyLayout(), am, m_context,
                  m_toolActionIds.back(),  mformtools);

    createSeparator(this, am, m_context, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator1"));

    addToolAction(m_fwm->actionLower(), am, m_context,
                  QLatin1String("FormEditor.Lower"), mformtools);

    addToolAction(m_fwm->actionRaise(), am, m_context,
                  QLatin1String("FormEditor.Raise"), mformtools);

    // Commands that do not go into the editor toolbar
    createSeparator(this, am, m_context, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator2"));

    m_actionPreview = m_fwm->actionDefaultPreview();
    QTC_ASSERT(m_actionPreview, return);
    addToolAction(m_actionPreview,  am,  m_context,
                   QLatin1String("FormEditor.Preview"), mformtools, tr("Ctrl+Alt+R"));

    // Preview in style...
    m_actionGroupPreviewInStyle = m_fwm->actionGroupPreviewInStyle();
    mformtools->addMenu(createPreviewStyleMenu(am, m_actionGroupPreviewInStyle));

    // Form settings
    createSeparator(this, am, m_context,  medit, QLatin1String("FormEditor.Edit.Separator2"), Core::Constants::G_EDIT_OTHER);

#if QT_VERSION >= 0x040500
    createSeparator(this, am, m_context, mformtools, QLatin1String("FormEditor.Menu.Tools.Separator3"));
    QAction *actionFormSettings = m_fwm->actionShowFormWindowSettingsDialog();
    addToolAction(actionFormSettings, am, m_context, QLatin1String("FormEditor.FormSettings"), mformtools);
#endif
    // FWM
    connect(m_fwm, SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface *)), this, SLOT(activeFormWindowChanged(QDesignerFormWindowInterface *)));
}


QToolBar *FormEditorW::createEditorToolBar() const
{
    QToolBar *rc = new QToolBar;
    rc->addSeparator();
    Core::ActionManager *am = m_core->actionManager();
    const QStringList::const_iterator cend = m_toolActionIds.constEnd();
    for (QStringList::const_iterator it = m_toolActionIds.constBegin(); it != cend; ++it) {
        Core::Command *cmd = am->command(*it);
        QTC_ASSERT(cmd, continue);
        QAction *action = cmd->action();
        if (!action->icon().isNull()) // Simplify grid has no action yet
            rc->addAction(action);
    }
    int size = rc->style()->pixelMetric(QStyle::PM_SmallIconSize);
    rc->setIconSize(QSize(size, size));
    return rc;
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
    s->setValue(QLatin1String(editorWidgetStateKeyC), EditorWidget::state().toVariant());
    s->endGroup();
}

void FormEditorW::restoreSettings(const QSettings *s)
{
    QString key = QLatin1String(settingsGroup) + QLatin1Char('/')
                                + QLatin1String(editorWidgetStateKeyC);
    const QVariant ev = s->value(key);
    if (ev.type() != QVariant::Invalid) {
        EditorWidgetState st;
        if (st.fromVariant(ev))
            EditorWidget::setState(st);
    }
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
    if (editor && !qstrcmp(editor->kind(), Constants::C_FORMEDITOR)) {
        FormWindowEditor *fw = qobject_cast<FormWindowEditor *>(editor);
        QTC_ASSERT(fw, return);
        fw->activate();
        m_fwm->setActiveFormWindow(fw->formWindow());
    } else {
        m_fwm->setActiveFormWindow(0);
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
            critical(tr("The image could not be create: %1").arg(errorMessage));
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
