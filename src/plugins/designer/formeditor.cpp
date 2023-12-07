// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designercontext.h"
#include "designertr.h"
#include "editordata.h"
#include "editorwidget.h"
#include "formeditor.h"
#include "formwindoweditor.h"
#include "formwindowfile.h"
#include "qtcreatorintegration.h"
#include "settingsmanager.h"
#include "settingspage.h"
#include <widgethost.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/designmode.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/outputpane.h>

#include <utils/infobar.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <abstractobjectinspector.h>
#include <QDesignerActionEditorInterface>
#include <QDesignerComponents>
#include <QDesignerFormEditorInterface>
#include <QDesignerFormEditorPluginInterface>
#include <QDesignerFormWindowManagerInterface>
#include <QDesignerFormWindowToolInterface>
#include <QDesignerPropertyEditorInterface>
#include <QDesignerWidgetBoxInterface>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPluginLoader>
#include <QPrintDialog>
#include <QPrinter>
#include <QStyle>
#include <QTime>
#include <QToolBar>
#include <QVBoxLayout>

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

Q_GLOBAL_STATIC(QString, sQtPluginPath);
Q_GLOBAL_STATIC(QStringList, sAdditionalPluginPaths);

using namespace Core;
using namespace Designer::Constants;
using namespace Utils;

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
    using TextEditorWidget::TextEditorWidget;

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
        setMarksVisible(false);
    }

    FormWindowEditor *create(QDesignerFormWindowInterface *form)
    {
        setDocumentCreator([form]() { return new FormWindowFile(form); });
        return qobject_cast<FormWindowEditor *>(createEditor());
    }
};

class ToolData
{
public:
    int index;
    QByteArray className;
};

// FormEditorData

class FormEditorData : public QObject
{
public:
    FormEditorData();
    ~FormEditorData();

    void activateEditMode(const ToolData &toolData);
    void toolChanged(QDesignerFormWindowInterface *form, int);
    void print();
    void setPreviewMenuEnabled(bool e);
    void updateShortcut(Command *command);

    void fullInit();

    void saveSettings(QtcSettings *s);

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
                                  const QByteArray &toolClassName,
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
    InitializationStage m_initStage = RegisterPlugins;

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

    Context m_contexts;

    QList<Id> m_toolActionIds;
    QWidget *m_modeWidget = nullptr;
    EditorWidget *m_editorWidget = nullptr;

    QWidget *m_editorToolBar = nullptr;
    EditorToolBar *m_toolBar = nullptr;

    QMap<Command *, QAction *> m_commandToDesignerAction;
    FormWindowEditorFactory *m_xmlEditorFactory = nullptr;
};

static FormEditorData *d = nullptr;

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
static QStringList designerPluginPaths()
{
    const QStringList qtPluginPath = sQtPluginPath->isEmpty()
                                         ? QDesignerComponents::defaultPluginPaths()
                                         : QStringList(*sQtPluginPath);
    return qtPluginPath + *sAdditionalPluginPaths;
}
#endif

FormEditorData::FormEditorData()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    m_formeditor = QDesignerComponents::createFormEditorWithPluginPaths(designerPluginPaths(),
                                                                        nullptr);
#else
    // Qt < 6.7.0 doesn't have API for changing the plugin path yet.
    // Work around it by temporarily changing the application's library paths,
    // which are used for Designer's plugin paths.
    // This must be done before creating the FormEditor, and with it QDesignerPluginManager.
    const QStringList restoreLibraryPaths = sQtPluginPath->isEmpty()
                                                ? QStringList()
                                                : QCoreApplication::libraryPaths();
    if (!sQtPluginPath->isEmpty())
        QCoreApplication::setLibraryPaths(QStringList(*sQtPluginPath));
    m_formeditor = QDesignerComponents::createFormEditor(nullptr);
    if (!sQtPluginPath->isEmpty())
        QCoreApplication::setLibraryPaths(restoreLibraryPaths);
#endif
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO;
    QTC_ASSERT(!d, return);
    d = this;

    std::fill(m_designerSubWindows, m_designerSubWindows + DesignerSubWindowCount,
              static_cast<QWidget *>(nullptr));

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

    QObject::connect(EditorManager::instance(), &EditorManager::currentEditorChanged, this, [this](IEditor *editor) {
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
    });

    m_xmlEditorFactory = new FormWindowEditorFactory;
}

FormEditorData::~FormEditorData()
{
    if (m_initStage == FullyInitialized) {
        QtcSettings *s = ICore::settings();
        s->beginGroup(settingsGroupC);
        m_editorWidget->saveSettings(s);
        s->endGroup();

        DesignMode::unregisterDesignWidget(m_modeWidget);
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
    ActionContainer *viewMenu = ActionManager::actionContainer(Core::Constants::M_VIEW_VIEWS);
    QTC_ASSERT(viewMenu, return);

    addDockViewAction(viewMenu, WidgetBoxSubWindow, m_contexts,
                      Tr::tr("Widget box"), "FormEditor.WidgetBox");

    addDockViewAction(viewMenu, ObjectInspectorSubWindow, m_contexts,
                      Tr::tr("Object Inspector"), "FormEditor.ObjectInspector");

    addDockViewAction(viewMenu, PropertyEditorSubWindow, m_contexts,
                      Tr::tr("Property Editor"), "FormEditor.PropertyEditor");

    addDockViewAction(viewMenu, SignalSlotEditorSubWindow, m_contexts,
                      Tr::tr("Signals && Slots Editor"), "FormEditor.SignalsAndSlotsEditor");

    addDockViewAction(viewMenu, ActionEditorSubWindow, m_contexts,
                      Tr::tr("Action Editor"), "FormEditor.ActionEditor");
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
    QTC_ASSERT(m_initStage == RegisterPlugins, return);
    QElapsedTimer *initTime = nullptr;
    if (Designer::Constants::Internal::debug) {
        initTime = new QElapsedTimer;
        initTime->start();
    }

    QDesignerComponents::createTaskMenu(m_formeditor, this);
    QDesignerComponents::initializePlugins(m_formeditor);
    QDesignerComponents::initializeResources();
    initDesignerSubWindows();
    m_integration = new QtCreatorIntegration(m_formeditor, this);
    m_formeditor->setIntegration(m_integration);
    // Connect Qt Designer help request to HelpManager.
    QObject::connect(m_integration, &QtCreatorIntegration::creatorHelpRequested,
                     HelpManager::Signals::instance(),
                     [](const QUrl &url) { HelpManager::showHelpUrl(url, HelpManager::HelpModeAlways); });

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

    QObject::connect(EditorManager::instance(), &EditorManager::editorsClosed, this,
                     [this] (const QList<IEditor *> editors) {
        for (IEditor *editor : editors)
            m_editorWidget->removeFormWindowEditor(editor);
    });

    // Nest toolbar and editor widget
    m_editorWidget = new EditorWidget;
    QtcSettings *settings = ICore::settings();
    settings->beginGroup(settingsGroupC);
    m_editorWidget->restoreSettings(settings);
    settings->endGroup();

    m_editorToolBar = createEditorToolBar();
    m_toolBar = new EditorToolBar;
    m_toolBar->setToolbarCreationFlags(EditorToolBar::FlagsStandalone);
    m_toolBar->setNavigationVisible(false);
    m_toolBar->addCenterToolBar(m_editorToolBar);

    m_modeWidget = new QWidget;
    m_modeWidget->setObjectName("DesignerModeWidget");
    auto layout = new QVBoxLayout(m_modeWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_toolBar);
    // Avoid mode switch to 'Edit' mode when the application started by
    // 'Run' in 'Design' mode emits output.
    auto splitter = new MiniSplitter(Qt::Vertical);
    splitter->addWidget(m_editorWidget);
    QWidget *outputPane = new OutputPanePlaceHolder(Core::Constants::MODE_DESIGN, splitter);
    outputPane->setObjectName("DesignerOutputPanePlaceHolder");
    splitter->addWidget(outputPane);
    layout->addWidget(splitter);

    Context designerContexts = m_contexts;
    designerContexts.add(Core::Constants::C_EDITORMANAGER);
    ICore::addContextObject(new DesignerContext(designerContexts, m_modeWidget, this));

    DesignMode::registerDesignWidget(m_modeWidget, QStringList(FORM_MIMETYPE), m_contexts);

    setupViewActions();

    m_initStage = FullyInitialized;
}

void FormEditorData::initDesignerSubWindows()
{
    std::fill(m_designerSubWindows, m_designerSubWindows + DesignerSubWindowCount, static_cast<QWidget*>(nullptr));

    QDesignerWidgetBoxInterface *wb = QDesignerComponents::createWidgetBox(m_formeditor, nullptr);
    wb->setWindowTitle(Tr::tr("Widget Box"));
    wb->setObjectName("WidgetBox");
    m_formeditor->setWidgetBox(wb);
    m_designerSubWindows[WidgetBoxSubWindow] = wb;

    QDesignerObjectInspectorInterface *oi = QDesignerComponents::createObjectInspector(m_formeditor, nullptr);
    oi->setWindowTitle(Tr::tr("Object Inspector"));
    oi->setObjectName("ObjectInspector");
    m_formeditor->setObjectInspector(oi);
    m_designerSubWindows[ObjectInspectorSubWindow] = oi;

    QDesignerPropertyEditorInterface *pe = QDesignerComponents::createPropertyEditor(m_formeditor, nullptr);
    pe->setWindowTitle(Tr::tr("Property Editor"));
    pe->setObjectName("PropertyEditor");
    m_formeditor->setPropertyEditor(pe);
    m_designerSubWindows[PropertyEditorSubWindow] = pe;

    QWidget *se = QDesignerComponents::createSignalSlotEditor(m_formeditor, nullptr);
    se->setWindowTitle(Tr::tr("Signals and Slots Editor"));
    se->setObjectName("SignalsAndSlotsEditor");
    m_designerSubWindows[SignalSlotEditorSubWindow] = se;

    QDesignerActionEditorInterface *ae = QDesignerComponents::createActionEditor(m_formeditor, nullptr);
    ae->setWindowTitle(Tr::tr("Action Editor"));
    ae->setObjectName("ActionEditor");
    m_formeditor->setActionEditor(ae);
    m_designerSubWindows[ActionEditorSubWindow] = ae;
    m_initStage = SubwindowsInitialized;
}

QList<IOptionsPage *> optionsPages()
{
    return d->m_settingsPages;
}

void ensureInitStage(InitializationStage s)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << s;
    if (!d)
        d = new FormEditorData;

    if (d->m_initStage >= s)
        return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    d->fullInit();
    QApplication::restoreOverrideCursor();
}

void deleteInstance()
{
    delete d;
    d = nullptr;
}

IEditor *createEditor()
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

    m_actionPrint = new QAction(this);
    bindShortcut(ActionManager::registerAction(m_actionPrint, Core::Constants::PRINT, m_contexts), m_actionPrint);
    connect(m_actionPrint, &QAction::triggered, this, &FormEditorData::print);

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
    QObject::connect(m_actionGroupEditMode, &QActionGroup::triggered, this, [this](QAction *a) {
        activateEditMode(a->data().value<ToolData>());
    });

    medit->addSeparator(m_contexts, Core::Constants::G_EDIT_OTHER);

    m_toolActionIds.push_back("FormEditor.WidgetEditor");
    createEditModeAction(m_actionGroupEditMode,
                         m_contexts,
                         medit,
                         Tr::tr("Edit Widgets"),
                         m_toolActionIds.back(),
                         EditModeWidgetEditor,
                         "qdesigner_internal::WidgetEditorTool",
                         "widgettool.png",
                         Tr::tr("F3"));

    m_toolActionIds.push_back("FormEditor.SignalsSlotsEditor");
    createEditModeAction(m_actionGroupEditMode,
                         m_contexts,
                         medit,
                         Tr::tr("Edit Signals/Slots"),
                         m_toolActionIds.back(),
                         EditModeSignalsSlotEditor,
                         "qdesigner_internal::SignalSlotEditorTool",
                         "signalslottool.png",
                         Tr::tr("F4"));

    m_toolActionIds.push_back("FormEditor.BuddyEditor");
    createEditModeAction(m_actionGroupEditMode,
                         m_contexts,
                         medit,
                         Tr::tr("Edit Buddies"),
                         m_toolActionIds.back(),
                         EditModeBuddyEditor,
                         "qdesigner_internal::BuddyEditorTool",
                         "buddytool.png");

    m_toolActionIds.push_back("FormEditor.TabOrderEditor");
    createEditModeAction(m_actionGroupEditMode,
                         m_contexts,
                         medit,
                         Tr::tr("Edit Tab Order"),
                         m_toolActionIds.back(),
                         EditModeTabOrderEditor,
                         "qdesigner_internal::TabOrderEditorTool",
                         "tabordertool.png");

    //tool actions
    m_toolActionIds.push_back("FormEditor.LayoutHorizontally");
    const QString horizLayoutShortcut = useMacShortcuts ? Tr::tr("Meta+Shift+H") : Tr::tr("Ctrl+H");
    addToolAction(m_fwm->actionHorizontalLayout(), m_contexts,
                  m_toolActionIds.back(), mformtools, horizLayoutShortcut);

    m_toolActionIds.push_back("FormEditor.LayoutVertically");
    const QString vertLayoutShortcut = useMacShortcuts ? Tr::tr("Meta+L") : Tr::tr("Ctrl+L");
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
    const QString gridShortcut = useMacShortcuts ? Tr::tr("Meta+Shift+G") : Tr::tr("Ctrl+G");
    addToolAction(m_fwm->actionGridLayout(), m_contexts,
                  m_toolActionIds.back(),  mformtools, gridShortcut);

    m_toolActionIds.push_back("FormEditor.LayoutBreak");
    addToolAction(m_fwm->actionBreakLayout(), m_contexts,
                  m_toolActionIds.back(), mformtools);

    m_toolActionIds.push_back("FormEditor.LayoutAdjustSize");
    const QString adjustShortcut = useMacShortcuts ? Tr::tr("Meta+J") : Tr::tr("Ctrl+J");
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
    addToolAction(m_actionPreview, m_contexts, "FormEditor.Preview", mformtools, Tr::tr("Alt+Shift+R"));

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
    m_actionAboutPlugins = new QAction(Tr::tr("About Qt Designer Plugins..."), d);
    addToolAction(m_actionAboutPlugins, m_contexts, "FormEditor.AboutPlugins", mformtools,
                  QString(), Core::Constants::G_DEFAULT_THREE);
    QObject::connect(m_actionAboutPlugins, &QAction::triggered,
        m_fwm, &QDesignerFormWindowManagerInterface::showPluginDialog);
    m_actionAboutPlugins->setEnabled(false);

    // FWM
    QObject::connect(m_fwm, &QDesignerFormWindowManagerInterface::activeFormWindowChanged, this,
        [this] (QDesignerFormWindowInterface *afw) {
            m_fwm->closeAllPreviews();
            setPreviewMenuEnabled(afw != nullptr);
        });
}

QToolBar *FormEditorData::createEditorToolBar() const
{
    QToolBar *editorToolBar = new QToolBar;
    for (const auto &id : m_toolActionIds) {
        Command *cmd = ActionManager::command(id);
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
    menuPreviewStyle->menu()->setTitle(Tr::tr("Preview in"));

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
        const bool isDeviceProfile = data.typeId() == QVariant::Int;
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

void FormEditorData::saveSettings(QtcSettings *s)
{
    s->beginGroup(settingsGroupC);
    m_editorWidget->saveSettings(s);
    s->endGroup();
}

void FormEditorData::critical(const QString &errorMessage)
{
    QMessageBox::critical(ICore::dialogParent(), Tr::tr("Designer"), errorMessage);
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
                                              const QByteArray &toolClassName,
                                              const QString &iconName,
                                              const QString &keySequence)
{
    auto rc = new QAction(actionName, ag);
    rc->setCheckable(true);
    if (!iconName.isEmpty())
         rc->setIcon(designerIcon(iconName));
    Command *command = ActionManager::registerAction(rc, id, context);
    command->setAttribute(Command::CA_Hide);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    bindShortcut(command, rc);
    medit->addAction(command, Core::Constants::G_EDIT_OTHER);
    rc->setData(QVariant::fromValue(ToolData{toolNumber, toolClassName}));
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
    QDesignerFormWindowInterface *form = m_fwm->createFormWindow(nullptr);
    QTC_ASSERT(form, return nullptr);
    form->setPalette(Theme::initialPalette());
    connect(form, &QDesignerFormWindowInterface::toolChanged, this, [this, form](int index) {
        toolChanged(form, index);
    });

    auto widgetHost = new SharedTools::WidgetHost( /* parent */ nullptr, form);
    FormWindowEditor *formWindowEditor = m_xmlEditorFactory->create(form);

    m_editorWidget->add(widgetHost, formWindowEditor);
    m_toolBar->addEditor(formWindowEditor);

    if (formWindowEditor) {
        Utils::InfoBarEntry info(Id(Constants::INFO_READ_ONLY),
                                 Tr::tr("This file can only be edited in <b>Design</b> mode."));
        info.addCustomButton(Tr::tr("Switch Mode"), []() { ModeManager::activateMode(Core::Constants::MODE_DESIGN); });
        formWindowEditor->document()->infoBar()->addInfo(info);
    }
    return formWindowEditor;
}

QDesignerFormEditorInterface *designerEditor()
{
    ensureInitStage(FullyInitialized);
    return d->m_formeditor;
}

QWidget * const *designerSubWindows()
{
    ensureInitStage(SubwindowsInitialized);
    return d->m_designerSubWindows;
}

SharedTools::WidgetHost *activeWidgetHost()
{
    ensureInitStage(FullyInitialized);
    if (d->m_editorWidget)
        return d->m_editorWidget->activeEditor().widgetHost;
    return nullptr;
}

FormWindowEditor *activeEditor()
{
    ensureInitStage(FullyInitialized);
    if (d->m_editorWidget)
        return d->m_editorWidget->activeEditor().formWindowEditor;
    return nullptr;
}

void FormEditorData::updateShortcut(Command *command)
{
    if (!command)
        return;
    if (QAction *a = m_commandToDesignerAction.value(command))
        a->setShortcut(command->action()->shortcut());
}

static void setTool(QDesignerFormWindowInterface *formWindow, const ToolData &toolData)
{
    // check for tool action with correct object name,
    // otherwise fall back to index
    if (!toolData.className.isEmpty()) {
        const int toolCount = formWindow->toolCount();
        for (int tool = 0; tool < toolCount; ++tool) {
            if (formWindow->tool(tool)->metaObject()->className() == toolData.className) {
                formWindow->setCurrentTool(tool);
                return;
            }
        }
    }
    formWindow->setCurrentTool(toolData.index);
}

void FormEditorData::activateEditMode(const ToolData &toolData)
{
    if (const int count = m_fwm->formWindowCount()) {
        for (int i = 0; i < count; i++)
            setTool(m_fwm->formWindow(i), toolData);
    }
}

void FormEditorData::toolChanged(QDesignerFormWindowInterface *form, int t)
{
    // check for action with correct object name,
    // otherwise fall back to index
    QDesignerFormWindowToolInterface *tool = form->tool(t);
    const QList<QAction *> actions = m_actionGroupEditMode->actions();
    QAction *candidateByIndex = nullptr;
    for (QAction *action : actions) {
        const auto toolData = action->data().value<ToolData>();
        if (!toolData.className.isEmpty() && toolData.className == tool->metaObject()->className()) {
            action->setChecked(true);
            return;
        }
        if (toolData.index == t)
            candidateByIndex = action;
    }
    if (candidateByIndex)
        candidateByIndex->setChecked(true);
}

void FormEditorData::print()
{
    // Printing code courtesy of designer_actions.cpp
    QDesignerFormWindowInterface *fw = m_fwm->activeFormWindow();
    if (!fw)
        return;

    QPrinter *printer = ICore::printer();
    const bool oldFullPage =  printer->fullPage();
    const QPageLayout::Orientation oldOrientation = printer->pageLayout().orientation();
    printer->setFullPage(false);
    do {
        // Grab the image to be able to a suggest suitable orientation
        QString errorMessage;
        const QPixmap pixmap = m_fwm->createPreviewPixmap();
        if (pixmap.isNull()) {
            critical(Tr::tr("The image could not be created: %1").arg(errorMessage));
            break;
        }

        const QSizeF pixmapSize = pixmap.size();
        printer->setPageOrientation(pixmapSize.width() > pixmapSize.height() ? QPageLayout::Landscape
                                                                             : QPageLayout::Portrait);

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
    printer->setPageOrientation(oldOrientation);
}

void setQtPluginPath(const QString &qtPluginPath)
{
    QTC_CHECK(!d);
    *sQtPluginPath = QDir::fromNativeSeparators(qtPluginPath);
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    // Cut a "/designer" postfix off, if present.
    // For Qt < 6.7.0 we hack the plugin path by temporarily setting the application library paths
    // and Designer adds "/designer" to these.
    static const QString postfix = "/designer";
    *sQtPluginPath = Utils::trimBack(*sQtPluginPath, '/');
    if (sQtPluginPath->endsWith(postfix))
        sQtPluginPath->chop(postfix.size());
    if (!QFileInfo::exists(*sQtPluginPath + postfix)) {
        qWarning() << qPrintable(
            QLatin1String(
                "Warning: The path \"%1\" passed to -designer-qt-pluginpath does not exist. "
                "Note that \"%2\" at the end is enforced.")
                .arg(*sQtPluginPath + postfix, postfix));
    }
#endif
}

void addPluginPath(const QString &pluginPath)
{
    QTC_CHECK(!d);
    sAdditionalPluginPaths->append(pluginPath);
}

} // namespace Internal
} // namespace Designer

Q_DECLARE_METATYPE(Designer::Internal::ToolData)
