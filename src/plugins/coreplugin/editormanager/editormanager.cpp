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

#include "editormanager.h"
#include "editormanager_p.h"
#include "editorwindow.h"

#include "editorview.h"
#include "openeditorswindow.h"
#include "openeditorsview.h"
#include "documentmodel.h"
#include "documentmodel_p.h"
#include "ieditor.h"

#include <app/app_version.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/dialogs/openwithdialog.h>
#include <coreplugin/dialogs/readonlyfilesdialog.h>
#include <coreplugin/diffservice.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/ieditorfactory_p.h>
#include <coreplugin/editormanager/iexternaleditor.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/find/searchresultitem.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/outputpane.h>
#include <coreplugin/outputpanemanager.h>
#include <coreplugin/rightpane.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/vcsmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/executeondestruction.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/infobar.h>
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/overridecursor.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSet>
#include <QSettings>
#include <QTextCodec>
#include <QTimer>

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>

#include <algorithm>

#if defined(WITH_TESTS)
#include <coreplugin/coreplugin.h>
#include <QTest>
#endif

using namespace Utils;

enum { debugEditorManager=0 };

static const char kCurrentDocumentPrefix[] = "CurrentDocument";
static const char kCurrentDocumentXPos[] = "CurrentDocument:XPos";
static const char kCurrentDocumentYPos[] = "CurrentDocument:YPos";
static const char kMakeWritableWarning[] = "Core.EditorManager.MakeWritable";

static const char documentStatesKey[] = "EditorManager/DocumentStates";
static const char reloadBehaviorKey[] = "EditorManager/ReloadBehavior";
static const char autoSaveEnabledKey[] = "EditorManager/AutoSaveEnabled";
static const char autoSaveIntervalKey[] = "EditorManager/AutoSaveInterval";
static const char autoSuspendEnabledKey[] = "EditorManager/AutoSuspendEnabled";
static const char autoSuspendMinDocumentCountKey[] = "EditorManager/AutoSuspendMinDocuments";
static const char warnBeforeOpeningBigTextFilesKey[] = "EditorManager/WarnBeforeOpeningBigTextFiles";
static const char bigTextFileSizeLimitKey[] = "EditorManager/BigTextFileSizeLimitInMB";
static const char maxRecentFilesKey[] = "EditorManager/MaxRecentFiles";
static const char fileSystemCaseSensitivityKey[] = "Core/FileSystemCaseSensitivity";
static const char preferredEditorFactoriesKey[] = "EditorManager/PreferredEditorFactories";

static const char scratchBufferKey[] = "_q_emScratchBuffer";

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

//===================EditorManager=====================

/*!
    \class Core::EditorManagerPlaceHolder
    \inmodule QtCreator
    \internal
*/

EditorManagerPlaceHolder::EditorManagerPlaceHolder(QWidget *parent)
    : QWidget(parent)
{
    setLayout(new QVBoxLayout);
    layout()->setContentsMargins(0, 0, 0, 0);
    setFocusProxy(EditorManagerPrivate::mainEditorArea());
}

EditorManagerPlaceHolder::~EditorManagerPlaceHolder()
{
    // EditorManager will be deleted in ~MainWindow()
    QWidget *em = EditorManagerPrivate::mainEditorArea();
    if (em && em->parent() == this) {
        em->hide();
        em->setParent(nullptr);
    }
}

void EditorManagerPlaceHolder::showEvent(QShowEvent *)
{
    QWidget *previousFocus = nullptr;
    QWidget *em = EditorManagerPrivate::mainEditorArea();
    if (em->focusWidget() && em->focusWidget()->hasFocus())
        previousFocus = em->focusWidget();
    layout()->addWidget(em);
    em->show();
    if (previousFocus)
        previousFocus->setFocus();
}

// ---------------- EditorManager

/*!
    \class Core::EditorManager
    \inheaderfile coreplugin/editormanager/editormanager.h
    \inmodule QtCreator

    \brief The EditorManager class manages the editors created for files
    according to their MIME type.

    Whenever a user wants to edit or create a file, the EditorManager scans all
    IEditorFactory interfaces for suitable editors. The selected IEditorFactory
    is then asked to create an editor, as determined by the MIME type of the
    file.

    Users can split the editor view or open the editor in a new window when
    to work on and view multiple files on the same screen or on multiple
    screens. For more information, see
    \l{https://doc.qt.io/qtcreator/creator-coding-navigating.html#splitting-the-editor-view}
    {Splitting the Editor View}.
*/

/*!
    \enum Core::MakeWritableResult
    \internal

    This enum holds whether the document has successfully been made writable.

    \value OpenedWithVersionControl
           The document was opened under version control.
    \value MadeWritable
           The document was made writable.
    \value SavedAs
           The document was saved under another name.
    \value Failed
           The document cannot be made writable.
*/

/*!
    \enum EditorManager::OpenEditorFlag

    This enum holds settings for opening a file in an editor.

    \value NoFlags
           Does not use any settings.
    \value DoNotChangeCurrentEditor
           Does not switch focus to the newly opened editor.
    \value IgnoreNavigationHistory
           Does not add an entry to the navigation history for the
           opened editor.
    \value DoNotMakeVisible
           Does not force the editor to become visible.
    \value CanContainLineAndColumnNumber
           If the file path contains line and column numbers, opens
           the file in an editor and jumps to the line and column.
    \value OpenInOtherSplit
           Opens the document in another split of the window.
    \value DoNotSwitchToDesignMode
           Opens the document in the current mode.
    \value DoNotSwitchToEditMode
           Opens the document in the current mode.
    \value SwitchSplitIfAlreadyVisible
           Switches to another split if the document is already
           visible there.
*/

static EditorManager *m_instance = nullptr;
static EditorManagerPrivate *d;

static QString autoSaveName(const QString &fileName)
{
    return fileName + ".autosave";
}

static void setFocusToEditorViewAndUnmaximizePanes(EditorView *view)
{
    IEditor *editor = view->currentEditor();
    QWidget *target = editor ? editor->widget() : view;
    QWidget *focus = target->focusWidget();
    QWidget *w = focus ? focus : target;

    w->setFocus();
    ICore::raiseWindow(w);

    OutputPanePlaceHolder *holder = OutputPanePlaceHolder::getCurrent();
    if (holder && holder->window() == view->window()) {
        // unmaximize output pane if necessary
        if (holder->isVisible() && holder->isMaximized())
            holder->setMaximized(false);
    }
}

EditorManagerPrivate::EditorManagerPrivate(QObject *parent) :
    QObject(parent),
    m_revertToSavedAction(new QAction(EditorManager::tr("Revert to Saved"), this)),
    m_saveAction(new QAction(this)),
    m_saveAsAction(new QAction(this)),
    m_closeCurrentEditorAction(new QAction(EditorManager::tr("Close"), this)),
    m_closeAllEditorsAction(new QAction(EditorManager::tr("Close All"), this)),
    m_closeOtherDocumentsAction(new QAction(EditorManager::tr("Close Others"), this)),
    m_closeAllEditorsExceptVisibleAction(new QAction(EditorManager::tr("Close All Except Visible"), this)),
    m_gotoNextDocHistoryAction(new QAction(EditorManager::tr("Next Open Document in History"), this)),
    m_gotoPreviousDocHistoryAction(new QAction(EditorManager::tr("Previous Open Document in History"), this)),
    m_goBackAction(new QAction(Utils::Icons::PREV.icon(), EditorManager::tr("Go Back"), this)),
    m_goForwardAction(new QAction(Utils::Icons::NEXT.icon(), EditorManager::tr("Go Forward"), this)),
    m_gotoLastEditAction(new QAction(EditorManager::tr("Go to Last Edit"), this)),
    m_copyFilePathContextAction(new QAction(EditorManager::tr("Copy Full Path"), this)),
    m_copyLocationContextAction(new QAction(EditorManager::tr("Copy Path and Line Number"), this)),
    m_copyFileNameContextAction(new QAction(EditorManager::tr("Copy File Name"), this)),
    m_saveCurrentEditorContextAction(new QAction(EditorManager::tr("&Save"), this)),
    m_saveAsCurrentEditorContextAction(new QAction(EditorManager::tr("Save &As..."), this)),
    m_revertToSavedCurrentEditorContextAction(new QAction(EditorManager::tr("Revert to Saved"), this)),
    m_closeCurrentEditorContextAction(new QAction(EditorManager::tr("Close"), this)),
    m_closeAllEditorsContextAction(new QAction(EditorManager::tr("Close All"), this)),
    m_closeOtherDocumentsContextAction(new QAction(EditorManager::tr("Close Others"), this)),
    m_closeAllEditorsExceptVisibleContextAction(new QAction(EditorManager::tr("Close All Except Visible"), this)),
    m_openGraphicalShellAction(new QAction(FileUtils::msgGraphicalShellAction(), this)),
    m_openTerminalAction(new QAction(FileUtils::msgTerminalHereAction(), this)),
    m_findInDirectoryAction(new QAction(FileUtils::msgFindInDirectory(), this)),
    m_filePropertiesAction(new QAction(tr("Properties..."), this)),
    m_pinAction(new QAction(tr("Pin"), this))
{
    d = this;
}

EditorManagerPrivate::~EditorManagerPrivate()
{
    if (ICore::instance())
        delete m_openEditorsFactory;

    // close all extra windows
    for (int i = 0; i < m_editorAreas.size(); ++i) {
        EditorArea *area = m_editorAreas.at(i);
        disconnect(area, &QObject::destroyed, this, &EditorManagerPrivate::editorAreaDestroyed);
        delete area;
    }
    m_editorAreas.clear();

    DocumentModel::destroy();
    d = nullptr;
}

void EditorManagerPrivate::init()
{
    DocumentModel::init();
    connect(ICore::instance(), &ICore::contextAboutToChange,
            this, &EditorManagerPrivate::handleContextChange);
    connect(qApp, &QApplication::applicationStateChanged,
            this, [](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive)
                    EditorManager::updateWindowTitles();
            });

    const Context editManagerContext(Constants::C_EDITORMANAGER);
    // combined context for edit & design modes
    const Context editDesignContext(Constants::C_EDITORMANAGER, Constants::C_DESIGN_MODE);

    ActionContainer *mfile = ActionManager::actionContainer(Constants::M_FILE);

    // Revert to saved
    m_revertToSavedAction->setIcon(QIcon::fromTheme("document-revert"));
    Command *cmd = ActionManager::registerAction(m_revertToSavedAction,
                                       Constants::REVERTTOSAVED, editManagerContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Revert File to Saved"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_revertToSavedAction, &QAction::triggered, m_instance, &EditorManager::revertToSaved);

    // Save Action
    ActionManager::registerAction(m_saveAction, Constants::SAVE, editManagerContext);
    connect(m_saveAction, &QAction::triggered, m_instance, []() { EditorManager::saveDocument(); });

    // Save As Action
    ActionManager::registerAction(m_saveAsAction, Constants::SAVEAS, editManagerContext);
    connect(m_saveAsAction, &QAction::triggered, m_instance, &EditorManager::saveDocumentAs);

    // Window Menu
    ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);

    // Window menu separators
    mwindow->addSeparator(editManagerContext, Constants::G_WINDOW_SPLIT);
    mwindow->addSeparator(editManagerContext, Constants::G_WINDOW_NAVIGATE);

    // Close Action
    cmd = ActionManager::registerAction(m_closeCurrentEditorAction, Constants::CLOSE, editManagerContext, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+W")));
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(m_closeCurrentEditorAction->text());
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(m_closeCurrentEditorAction, &QAction::triggered,
            m_instance, &EditorManager::slotCloseCurrentEditorOrDocument);

    if (HostOsInfo::isWindowsHost()) {
        // workaround for QTCREATORBUG-72
        QAction *action = new QAction(tr("Alternative Close"), this);
        cmd = ActionManager::registerAction(action, Constants::CLOSE_ALTERNATIVE, editManagerContext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+F4")));
        cmd->setDescription(EditorManager::tr("Close"));
        connect(action, &QAction::triggered,
                m_instance, &EditorManager::slotCloseCurrentEditorOrDocument);
    }

    // Close All Action
    cmd = ActionManager::registerAction(m_closeAllEditorsAction, Constants::CLOSEALL, editManagerContext, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+W")));
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(m_closeAllEditorsAction, &QAction::triggered, m_instance, &EditorManager::closeAllDocuments);

    // Close All Others Action
    cmd = ActionManager::registerAction(m_closeOtherDocumentsAction, Constants::CLOSEOTHERS, editManagerContext, true);
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    cmd->setAttribute(Command::CA_UpdateText);
    connect(m_closeOtherDocumentsAction, &QAction::triggered,
            m_instance, []() { EditorManager::closeOtherDocuments(); });

    // Close All Others Except Visible Action
    cmd = ActionManager::registerAction(m_closeAllEditorsExceptVisibleAction, Constants::CLOSEALLEXCEPTVISIBLE, editManagerContext, true);
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(m_closeAllEditorsExceptVisibleAction, &QAction::triggered,
            this, &EditorManagerPrivate::closeAllEditorsExceptVisible);

    //Save XXX Context Actions
    connect(m_copyFilePathContextAction, &QAction::triggered,
            this, &EditorManagerPrivate::copyFilePathFromContextMenu);
    connect(m_copyLocationContextAction, &QAction::triggered,
            this, &EditorManagerPrivate::copyLocationFromContextMenu);
    connect(m_copyFileNameContextAction, &QAction::triggered,
            this, &EditorManagerPrivate::copyFileNameFromContextMenu);
    connect(m_saveCurrentEditorContextAction, &QAction::triggered,
            this, &EditorManagerPrivate::saveDocumentFromContextMenu);
    connect(m_saveAsCurrentEditorContextAction, &QAction::triggered,
            this, &EditorManagerPrivate::saveDocumentAsFromContextMenu);
    connect(m_revertToSavedCurrentEditorContextAction, &QAction::triggered,
            this, &EditorManagerPrivate::revertToSavedFromContextMenu);

    // Close XXX Context Actions
    connect(m_closeAllEditorsContextAction, &QAction::triggered,
            m_instance, &EditorManager::closeAllDocuments);
    connect(m_closeCurrentEditorContextAction, &QAction::triggered,
            this, &EditorManagerPrivate::closeEditorFromContextMenu);
    connect(m_closeOtherDocumentsContextAction, &QAction::triggered,
            this, &EditorManagerPrivate::closeOtherDocumentsFromContextMenu);
    connect(m_closeAllEditorsExceptVisibleContextAction, &QAction::triggered,
            this, &EditorManagerPrivate::closeAllEditorsExceptVisible);

    connect(m_openGraphicalShellAction, &QAction::triggered,
            this, &EditorManagerPrivate::showInGraphicalShell);
    connect(m_openTerminalAction, &QAction::triggered, this, &EditorManagerPrivate::openTerminal);
    connect(m_findInDirectoryAction, &QAction::triggered,
            this, &EditorManagerPrivate::findInDirectory);
    connect(m_filePropertiesAction, &QAction::triggered, this, []() {
        if (!d->m_contextMenuEntry || d->m_contextMenuEntry->fileName().isEmpty())
            return;
        DocumentManager::showFilePropertiesDialog(d->m_contextMenuEntry->fileName());
    });
    connect(m_pinAction, &QAction::triggered, this, &EditorManagerPrivate::togglePinned);

    // Goto Previous In History Action
    cmd = ActionManager::registerAction(m_gotoPreviousDocHistoryAction, Constants::GOTOPREVINHISTORY, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Alt+Tab") : tr("Ctrl+Tab")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_gotoPreviousDocHistoryAction, &QAction::triggered,
            this, &EditorManagerPrivate::gotoPreviousDocHistory);

    // Goto Next In History Action
    cmd = ActionManager::registerAction(m_gotoNextDocHistoryAction, Constants::GOTONEXTINHISTORY, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Alt+Shift+Tab") : tr("Ctrl+Shift+Tab")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_gotoNextDocHistoryAction, &QAction::triggered,
            this, &EditorManagerPrivate::gotoNextDocHistory);

    // Go back in navigation history
    cmd = ActionManager::registerAction(m_goBackAction, Constants::GO_BACK, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+Alt+Left") : tr("Alt+Left")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_goBackAction, &QAction::triggered,
            m_instance, &EditorManager::goBackInNavigationHistory);

    // Go forward in navigation history
    cmd = ActionManager::registerAction(m_goForwardAction, Constants::GO_FORWARD, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Ctrl+Alt+Right") : tr("Alt+Right")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_goForwardAction, &QAction::triggered,
            m_instance, &EditorManager::goForwardInNavigationHistory);

    // Go to last edit
    cmd = ActionManager::registerAction(m_gotoLastEditAction, Constants::GOTOLASTEDIT, editDesignContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_gotoLastEditAction, &QAction::triggered,
            this, &EditorManagerPrivate::gotoLastEditLocation);

    m_splitAction = new QAction(Utils::Icons::SPLIT_HORIZONTAL.icon(), tr("Split"), this);
    cmd = ActionManager::registerAction(m_splitAction, Constants::SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+E,2") : tr("Ctrl+E,2")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_splitAction, &QAction::triggered, this, []() { split(Qt::Vertical); });

    m_splitSideBySideAction = new QAction(Utils::Icons::SPLIT_VERTICAL.icon(),
                                          tr("Split Side by Side"), this);
    cmd = ActionManager::registerAction(m_splitSideBySideAction, Constants::SPLIT_SIDE_BY_SIDE, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+E,3") : tr("Ctrl+E,3")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_splitSideBySideAction, &QAction::triggered, m_instance, &EditorManager::splitSideBySide);

    m_splitNewWindowAction = new QAction(tr("Open in New Window"), this);
    cmd = ActionManager::registerAction(m_splitNewWindowAction, Constants::SPLIT_NEW_WINDOW, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+E,4") : tr("Ctrl+E,4")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_splitNewWindowAction, &QAction::triggered,
            this, []() { splitNewWindow(currentEditorView()); });

    m_removeCurrentSplitAction = new QAction(tr("Remove Current Split"), this);
    cmd = ActionManager::registerAction(m_removeCurrentSplitAction, Constants::REMOVE_CURRENT_SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+E,0") : tr("Ctrl+E,0")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_removeCurrentSplitAction, &QAction::triggered,
            this, &EditorManagerPrivate::removeCurrentSplit);

    m_removeAllSplitsAction = new QAction(tr("Remove All Splits"), this);
    cmd = ActionManager::registerAction(m_removeAllSplitsAction, Constants::REMOVE_ALL_SPLITS, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+E,1") : tr("Ctrl+E,1")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_removeAllSplitsAction, &QAction::triggered,
            this, &EditorManagerPrivate::removeAllSplits);

    m_gotoPreviousSplitAction = new QAction(tr("Go to Previous Split or Window"), this);
    cmd = ActionManager::registerAction(m_gotoPreviousSplitAction, Constants::GOTO_PREV_SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+E,i") : tr("Ctrl+E,i")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_gotoPreviousSplitAction, &QAction::triggered,
            this, &EditorManagerPrivate::gotoPreviousSplit);

    m_gotoNextSplitAction = new QAction(tr("Go to Next Split or Window"), this);
    cmd = ActionManager::registerAction(m_gotoNextSplitAction, Constants::GOTO_NEXT_SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+E,o") : tr("Ctrl+E,o")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_gotoNextSplitAction, &QAction::triggered, this, &EditorManagerPrivate::gotoNextSplit);

    ActionContainer *medit = ActionManager::actionContainer(Constants::M_EDIT);
    ActionContainer *advancedMenu = ActionManager::createMenu(Constants::M_EDIT_ADVANCED);
    medit->addMenu(advancedMenu, Constants::G_EDIT_ADVANCED);
    advancedMenu->menu()->setTitle(tr("Ad&vanced"));
    advancedMenu->appendGroup(Constants::G_EDIT_FORMAT);
    advancedMenu->appendGroup(Constants::G_EDIT_TEXT);
    advancedMenu->appendGroup(Constants::G_EDIT_COLLAPSING);
    advancedMenu->appendGroup(Constants::G_EDIT_BLOCKS);
    advancedMenu->appendGroup(Constants::G_EDIT_FONT);
    advancedMenu->appendGroup(Constants::G_EDIT_EDITOR);

    // Advanced menu separators
    advancedMenu->addSeparator(editManagerContext, Constants::G_EDIT_TEXT);
    advancedMenu->addSeparator(editManagerContext, Constants::G_EDIT_COLLAPSING);
    advancedMenu->addSeparator(editManagerContext, Constants::G_EDIT_BLOCKS);
    advancedMenu->addSeparator(editManagerContext, Constants::G_EDIT_FONT);
    advancedMenu->addSeparator(editManagerContext, Constants::G_EDIT_EDITOR);

    // other setup
    auto mainEditorArea = new EditorArea();
    // assign parent to avoid failing updates (e.g. windowTitle) before it is displayed first time
    mainEditorArea->setParent(ICore::mainWindow());
    mainEditorArea->hide();
    connect(mainEditorArea, &EditorArea::windowTitleNeedsUpdate,
            this, &EditorManagerPrivate::updateWindowTitle);
    connect(mainEditorArea, &QObject::destroyed, this, &EditorManagerPrivate::editorAreaDestroyed);
    d->m_editorAreas.append(mainEditorArea);
    d->m_currentView = mainEditorArea->view();

    updateActions();

    // The popup needs a parent to get keyboard focus.
    m_windowPopup = new OpenEditorsWindow(mainEditorArea);
    m_windowPopup->hide();

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setObjectName("EditorManager::m_autoSaveTimer");
    connect(m_autoSaveTimer, &QTimer::timeout, this, &EditorManagerPrivate::autoSave);
    updateAutoSave();

    d->m_openEditorsFactory = new OpenEditorsViewFactory();

    globalMacroExpander()->registerFileVariables(kCurrentDocumentPrefix, tr("Current document"),
        []() -> QString {
            IDocument *document = EditorManager::currentDocument();
            return document ? document->filePath().toString() : QString();
        });

    globalMacroExpander()->registerIntVariable(kCurrentDocumentXPos,
        tr("X-coordinate of the current editor's upper left corner, relative to screen."),
        []() -> int {
            IEditor *editor = EditorManager::currentEditor();
            return editor ? editor->widget()->mapToGlobal(QPoint(0, 0)).x() : 0;
        });

    globalMacroExpander()->registerIntVariable(kCurrentDocumentYPos,
        tr("Y-coordinate of the current editor's upper left corner, relative to screen."),
        []() -> int {
            IEditor *editor = EditorManager::currentEditor();
            return editor ? editor->widget()->mapToGlobal(QPoint(0, 0)).y() : 0;
                                               });
}

void EditorManagerPrivate::extensionsInitialized()
{
    // Do not ask for files to save.
    // MainWindow::closeEvent has already done that.
    ICore::addPreCloseListener([]() -> bool { return EditorManager::closeAllEditors(false); });
}

EditorManagerPrivate *EditorManagerPrivate::instance()
{
    return d;
}

EditorArea *EditorManagerPrivate::mainEditorArea()
{
    return d->m_editorAreas.at(0);
}

bool EditorManagerPrivate::skipOpeningBigTextFile(const QString &filePath)
{
    if (!d->m_warnBeforeOpeningBigFilesEnabled)
        return false;

    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists(filePath))
        return false;

    Utils::MimeType mimeType = Utils::mimeTypeForFile(filePath);
    if (!mimeType.inherits("text/plain"))
        return false;

    const double fileSizeInMB = fileInfo.size() / 1000.0 / 1000.0;
    if (fileSizeInMB > d->m_bigFileSizeLimitInMB) {
        const QString title = EditorManager::tr("Continue Opening Huge Text File?");
        const QString text = EditorManager::tr(
            "The text file \"%1\" has the size %2MB and might take more memory to open"
            " and process than available.\n"
            "\n"
            "Continue?")
                .arg(fileInfo.fileName())
                .arg(fileSizeInMB, 0, 'f', 2);

        CheckableMessageBox messageBox(ICore::dialogParent());
        messageBox.setWindowTitle(title);
        messageBox.setText(text);
        messageBox.setStandardButtons(QDialogButtonBox::Yes|QDialogButtonBox::No);
        messageBox.setDefaultButton(QDialogButtonBox::No);
        messageBox.setIcon(QMessageBox::Question);
        messageBox.setCheckBoxVisible(true);
        messageBox.setCheckBoxText(CheckableMessageBox::msgDoNotAskAgain());
        messageBox.exec();
        d->setWarnBeforeOpeningBigFilesEnabled(!messageBox.isChecked());
        return messageBox.clickedStandardButton() != QDialogButtonBox::Yes;
    }

    return false;
}

IEditor *EditorManagerPrivate::openEditor(EditorView *view, const QString &fileName, Id editorId,
                                          EditorManager::OpenEditorFlags flags, bool *newEditor)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << fileName << editorId.name();

    QString fn = fileName;
    QFileInfo fi(fn);
    int lineNumber = -1;
    int columnNumber = -1;
    if ((flags & EditorManager::CanContainLineAndColumnNumber) && !fi.exists()) {
        const EditorManager::FilePathInfo fp = EditorManager::splitLineAndColumnNumber(fn);
        fn = Utils::FileUtils::normalizePathName(fp.filePath);
        lineNumber = fp.lineNumber;
        columnNumber = fp.columnNumber;
    } else {
        fn = Utils::FileUtils::normalizePathName(fn);
    }

    if (fn.isEmpty())
        return nullptr;
    if (fn != fileName)
        fi.setFile(fn);

    if (newEditor)
        *newEditor = false;

    const QList<IEditor *> editors = DocumentModel::editorsForFilePath(fn);
    if (!editors.isEmpty()) {
        IEditor *editor = editors.first();
        if (flags & EditorManager::SwitchSplitIfAlreadyVisible) {
            for (IEditor *ed : editors) {
                EditorView *v = viewForEditor(ed);
                // Don't switch to a view where editor is not its current editor
                if (v && v->currentEditor() == ed) {
                    editor = ed;
                    view = v;
                    break;
                }
            }
        }
        editor = activateEditor(view, editor, flags);
        if (editor && flags & EditorManager::CanContainLineAndColumnNumber)
            editor->gotoLine(lineNumber, columnNumber);
        return editor;
    }

    QString realFn = autoSaveName(fn);
    QFileInfo rfi(realFn);
    if (!fi.exists() || !rfi.exists() || fi.lastModified() >= rfi.lastModified()) {
        QFile::remove(realFn);
        realFn = fn;
    }

    EditorFactoryList factories = EditorManagerPrivate::findFactories(Id(), fn);
    if (factories.isEmpty()) {
        Utils::MimeType mimeType = Utils::mimeTypeForFile(fn);
        QMessageBox msgbox(QMessageBox::Critical, EditorManager::tr("File Error"),
                           tr("Could not open \"%1\": Cannot open files of type \"%2\".")
                           .arg(FilePath::fromString(realFn).toUserOutput(), mimeType.name()),
                           QMessageBox::Ok, ICore::dialogParent());
        msgbox.exec();
        return nullptr;
    }
    if (editorId.isValid()) {
        IEditorFactory *factory = Utils::findOrDefault(IEditorFactory::allEditorFactories(),
                                                       Utils::equal(&IEditorFactory::id, editorId));
        if (factory) {
            factories.removeOne(factory);
            factories.push_front(factory);
        }
    }

    if (skipOpeningBigTextFile(fileName))
        return nullptr;

    IEditor *editor = nullptr;
    auto overrideCursor = Utils::OverrideCursor(QCursor(Qt::WaitCursor));

    IEditorFactory *factory = factories.takeFirst();
    while (factory) {
        editor = createEditor(factory, fn);
        if (!editor) {
            factory = factories.takeFirst();
            continue;
        }

        QString errorString;
        IDocument::OpenResult openResult = editor->document()->open(&errorString, fn, realFn);
        if (openResult == IDocument::OpenResult::Success)
            break;

        overrideCursor.reset();
        delete editor;
        editor = nullptr;

        if (openResult == IDocument::OpenResult::ReadError) {
            QMessageBox msgbox(QMessageBox::Critical, EditorManager::tr("File Error"),
                               tr("Could not open \"%1\" for reading. "
                                  "Either the file does not exist or you do not have "
                                  "the permissions to open it.")
                               .arg(FilePath::fromString(realFn).toUserOutput()),
                               QMessageBox::Ok, ICore::dialogParent());
            msgbox.exec();
            return nullptr;
        }
        QTC_CHECK(openResult == IDocument::OpenResult::CannotHandle);

        if (errorString.isEmpty()) {
            errorString = tr("Could not open \"%1\": Unknown error.")
                    .arg(FilePath::fromString(realFn).toUserOutput());
        }

        QMessageBox msgbox(QMessageBox::Critical,
                           EditorManager::tr("File Error"),
                           errorString,
                           QMessageBox::Open | QMessageBox::Cancel,
                           ICore::dialogParent());

        IEditorFactory *selectedFactory = nullptr;
        if (!factories.isEmpty()) {
            auto button = qobject_cast<QPushButton *>(msgbox.button(QMessageBox::Open));
            QTC_ASSERT(button, return nullptr);
            auto menu = new QMenu(button);
            foreach (IEditorFactory *factory, factories) {
                QAction *action = menu->addAction(factory->displayName());
                connect(action, &QAction::triggered, &msgbox, [&selectedFactory, factory, &msgbox]() {
                    selectedFactory = factory;
                    msgbox.done(QMessageBox::Open);
                });
            }

            button->setMenu(menu);
        } else {
            msgbox.setStandardButtons(QMessageBox::Ok);
        }

        int ret = msgbox.exec();
        if (ret == QMessageBox::Cancel || ret == QMessageBox::Ok)
            return nullptr;

        overrideCursor.set();

        factories.removeOne(selectedFactory);
        factory = selectedFactory;
    }

    if (!editor)
        return nullptr;

    if (realFn != fn)
        editor->document()->setRestoredFrom(realFn);
    addEditor(editor);

    if (newEditor)
        *newEditor = true;

    IEditor *result = activateEditor(view, editor, flags);
    if (editor == result)
        restoreEditorState(editor);

    if (flags & EditorManager::CanContainLineAndColumnNumber)
        editor->gotoLine(lineNumber, columnNumber);

    return result;
}

IEditor *EditorManagerPrivate::openEditorAt(EditorView *view, const QString &fileName, int line,
                                            int column, Id editorId,
                                            EditorManager::OpenEditorFlags flags, bool *newEditor)
{
    EditorManager::cutForwardNavigationHistory();
    EditorManager::addCurrentPositionToNavigationHistory();
    EditorManager::OpenEditorFlags tempFlags = flags | EditorManager::IgnoreNavigationHistory;
    IEditor *editor = openEditor(view, fileName, editorId, tempFlags, newEditor);
    if (editor && line != -1)
        editor->gotoLine(line, column);
    return editor;
}

IEditor *EditorManagerPrivate::openEditorWith(const QString &fileName, Utils::Id editorId)
{
    // close any open editors that have this file open
    // remember the views to open new editors in there
    QList<EditorView *> views;
    QList<IEditor *> editorsOpenForFile
            = DocumentModel::editorsForFilePath(fileName);
    foreach (IEditor *openEditor, editorsOpenForFile) {
        EditorView *view = EditorManagerPrivate::viewForEditor(openEditor);
        if (view && view->currentEditor() == openEditor) // visible
            views.append(view);
    }
    if (!EditorManager::closeEditors(editorsOpenForFile)) // don't open if cancel was pressed
        return nullptr;

    IEditor *openedEditor = nullptr;
    if (views.isEmpty()) {
        openedEditor = EditorManager::openEditor(fileName, editorId);
    } else {
        if (EditorView *currentView = EditorManagerPrivate::currentEditorView()) {
            if (views.removeOne(currentView))
                views.prepend(currentView); // open editor in current view first
        }
        EditorManager::OpenEditorFlags flags;
        foreach (EditorView *view, views) {
            IEditor *editor = EditorManagerPrivate::openEditor(view, fileName, editorId, flags);
            if (!openedEditor && editor)
                openedEditor = editor;
            // Do not change the current editor after opening the first one. That
            // * prevents multiple updates of focus etc which are not necessary
            // * lets us control which editor is made current by putting the current editor view
            //   to the front (if that was in the list in the first place)
            flags |= EditorManager::DoNotChangeCurrentEditor;
            // do not try to open more editors if this one failed, or editor type does not
            // support duplication anyhow
            if (!editor || !editor->duplicateSupported())
                break;
        }
    }
    return openedEditor;
}

IEditor *EditorManagerPrivate::activateEditorForDocument(EditorView *view, IDocument *document,
                                                         EditorManager::OpenEditorFlags flags)
{
    Q_ASSERT(view);
    IEditor *editor = view->editorForDocument(document);
    if (!editor) {
        const QList<IEditor*> editors = DocumentModel::editorsForDocument(document);
        if (editors.isEmpty())
            return nullptr;
        editor = editors.first();
    }
    return activateEditor(view, editor, flags);
}

EditorView *EditorManagerPrivate::viewForEditor(IEditor *editor)
{
    QWidget *w = editor->widget();
    while (w) {
        w = w->parentWidget();
        if (auto view = qobject_cast<EditorView *>(w))
            return view;
    }
    return nullptr;
}

MakeWritableResult EditorManagerPrivate::makeFileWritable(IDocument *document)
{
    if (!document)
        return Failed;
    ReadOnlyFilesDialog roDialog(document, ICore::dialogParent(), document->isSaveAsAllowed());
    switch (roDialog.exec()) {
    case ReadOnlyFilesDialog::RO_MakeWritable:
    case ReadOnlyFilesDialog::RO_OpenVCS:
        return MadeWritable;
    case ReadOnlyFilesDialog::RO_SaveAs:
        return SavedAs;
    default:
        return Failed;
    }
}

/*!
    Implements the logic of the escape key shortcut (ReturnToEditor).
    Should only be called by the shortcut handler.
    \internal
*/
void EditorManagerPrivate::doEscapeKeyFocusMoveMagic()
{
    // use cases to cover:
    // 1. if app focus is in mode or external window without editor view (e.g. Design, Projects, ext. Help)
    //      if there are extra views (e.g. output)
    //        hide them
    //      otherwise
    //        activate & raise the current editor view (can be external)
    //        if that is in edit mode
    //          activate edit mode and unmaximize output pane
    // 2. if app focus is in external window with editor view
    //      hide find if necessary
    // 2. if app focus is in mode with editor view
    //      if current editor view is in external window
    //        raise and activate current editor view
    //      otherwise if the current editor view is not app focus
    //        move focus to editor view in mode and unmaximize output pane
    //      otherwise if the current view is app focus
    //        if mode is not edit mode
    //          if there are extra views (find, help, output)
    //            hide them
    //          otherwise
    //            activate edit mode and unmaximize output pane
    //        otherwise (i.e. mode is edit mode)
    //          hide extra views (find, help, output)

    QWidget *activeWindow = QApplication::activeWindow();
    if (!activeWindow)
        return;
    QWidget *focus = QApplication::focusWidget();
    EditorView *editorView = currentEditorView();
    bool editorViewActive = (focus && focus == editorView->focusWidget());
    bool editorViewVisible = editorView->isVisible();

    bool stuffHidden = false;
    FindToolBarPlaceHolder *findPane = FindToolBarPlaceHolder::getCurrent();
    if (findPane && findPane->isVisible() && findPane->isUsedByWidget(focus)) {
        findPane->hide();
        stuffHidden = true;
    } else if (!( editorViewVisible && !editorViewActive && editorView->window() == activeWindow )) {
        QWidget *outputPane = OutputPanePlaceHolder::getCurrent();
        if (outputPane && outputPane->isVisible() && outputPane->window() == activeWindow) {
            OutputPaneManager::instance()->slotHide();
            stuffHidden = true;
        }
        QWidget *rightPane = RightPanePlaceHolder::current();
        if (rightPane && rightPane->isVisible() && rightPane->window() == activeWindow) {
            RightPaneWidget::instance()->setShown(false);
            stuffHidden = true;
        }
        if (findPane && findPane->isVisible() && findPane->window() == activeWindow) {
            findPane->hide();
            stuffHidden = true;
        }
    }
    if (stuffHidden)
        return;

    if (!editorViewActive && editorViewVisible) {
        setFocusToEditorViewAndUnmaximizePanes(editorView);
        return;
    }

    if (!editorViewActive && !editorViewVisible) {
        // assumption is that editorView is in main window then
        ModeManager::activateMode(Id(Constants::MODE_EDIT));
        QTC_CHECK(editorView->isVisible());
        setFocusToEditorViewAndUnmaximizePanes(editorView);
        return;
    }

    if (editorView->window() == ICore::mainWindow()) {
        // we are in a editor view and there's nothing to hide, switch to edit
        ModeManager::activateMode(Id(Constants::MODE_EDIT));
        QTC_CHECK(editorView->isVisible());
        // next call works only because editor views in main window are shared between modes
        setFocusToEditorViewAndUnmaximizePanes(editorView);
    }
}

OpenEditorsWindow *EditorManagerPrivate::windowPopup()
{
    return d->m_windowPopup;
}

void EditorManagerPrivate::showPopupOrSelectDocument()
{
    if (QApplication::keyboardModifiers() == Qt::NoModifier) {
        windowPopup()->selectAndHide();
    } else {
        QWidget *activeWindow = QApplication::activeWindow();
        // decide where to show the popup
        // if the active window has editors, we want that editor area as a reference
        // TODO: this does not work correctly with multiple editor areas in the same window
        EditorArea *activeEditorArea = nullptr;
        foreach (EditorArea *area, d->m_editorAreas) {
            if (area->window() == activeWindow) {
                activeEditorArea = area;
                break;
            }
        }
        // otherwise we take the "current" editor area
        if (!activeEditorArea)
            activeEditorArea = findEditorArea(EditorManagerPrivate::currentEditorView());
        QTC_ASSERT(activeEditorArea, activeEditorArea = d->m_editorAreas.first());

        // editor area in main window is invisible when invoked from Design Mode.
        QWidget *referenceWidget = activeEditorArea->isVisible() ? activeEditorArea : activeEditorArea->window();
        QTC_CHECK(referenceWidget->isVisible());
        const QPoint p = referenceWidget->mapToGlobal(QPoint(0, 0));
        OpenEditorsWindow *popup = windowPopup();
        popup->setMaximumSize(qMax(popup->minimumWidth(), referenceWidget->width() / 2),
                              qMax(popup->minimumHeight(), referenceWidget->height() / 2));
        popup->adjustSize();
        popup->move((referenceWidget->width() - popup->width()) / 2 + p.x(),
                    (referenceWidget->height() - popup->height()) / 2 + p.y());
        popup->setVisible(true);
    }
}

// Run the OpenWithDialog and return the editor id
// selected by the user.
Id EditorManagerPrivate::getOpenWithEditorId(const QString &fileName, bool *isExternalEditor)
{
    // Collect editors that can open the file
    QList<Id> allEditorIds;
    QStringList allEditorDisplayNames;
    QList<Id> externalEditorIds;
    // Built-in
    const EditorFactoryList editors = IEditorFactory::preferredEditorFactories(fileName);
    const int size = editors.size();
    allEditorDisplayNames.reserve(size);
    for (int i = 0; i < size; i++) {
        allEditorIds.push_back(editors.at(i)->id());
        allEditorDisplayNames.push_back(editors.at(i)->displayName());
    }
    // External editors
    const Utils::MimeType mt = Utils::mimeTypeForFile(fileName);
    const ExternalEditorList exEditors = IExternalEditor::externalEditors(mt);
    const int esize = exEditors.size();
    for (int i = 0; i < esize; i++) {
        externalEditorIds.push_back(exEditors.at(i)->id());
        allEditorIds.push_back(exEditors.at(i)->id());
        allEditorDisplayNames.push_back(exEditors.at(i)->displayName());
    }
    if (allEditorIds.empty())
        return Id();
    QTC_ASSERT(allEditorIds.size() == allEditorDisplayNames.size(), return Id());
    // Run dialog.
    OpenWithDialog dialog(fileName, ICore::dialogParent());
    dialog.setEditors(allEditorDisplayNames);
    dialog.setCurrentEditor(0);
    if (dialog.exec() != QDialog::Accepted)
        return Id();
    const Id selectedId = allEditorIds.at(dialog.editor());
    if (isExternalEditor)
        *isExternalEditor = externalEditorIds.contains(selectedId);
    return selectedId;
}

static QMap<QString, QVariant> toMap(const QHash<Utils::MimeType, IEditorFactory *> &hash)
{
    QMap<QString, QVariant> map;
    auto it = hash.begin();
    const auto end = hash.end();
    while (it != end) {
        map.insert(it.key().name(), it.value()->id().toSetting());
        ++it;
    }
    return map;
}

static QHash<Utils::MimeType, IEditorFactory *> fromMap(const QMap<QString, QVariant> &map)
{
    const EditorFactoryList factories = IEditorFactory::allEditorFactories();
    QHash<Utils::MimeType, IEditorFactory *> hash;
    auto it = map.begin();
    const auto end = map.end();
    while (it != end) {
        const Utils::MimeType mimeType = Utils::mimeTypeForName(it.key());
        if (mimeType.isValid()) {
            const Id factoryId = Id::fromSetting(it.value());
            IEditorFactory *factory = Utils::findOrDefault(factories,
                                                           Utils::equal(&IEditorFactory::id,
                                                                        factoryId));
            if (factory)
                hash.insert(mimeType, factory);
        }
        ++it;
    }
    return hash;
}

void EditorManagerPrivate::saveSettings()
{
    ICore::settingsDatabase()->setValue(documentStatesKey, d->m_editorStates);

    QSettings *qsettings = ICore::settings();
    qsettings->setValue(reloadBehaviorKey, d->m_reloadSetting);
    qsettings->setValue(autoSaveEnabledKey, d->m_autoSaveEnabled);
    qsettings->setValue(autoSaveIntervalKey, d->m_autoSaveInterval);
    qsettings->setValue(autoSuspendEnabledKey, d->m_autoSuspendEnabled);
    qsettings->setValue(autoSuspendMinDocumentCountKey, d->m_autoSuspendMinDocumentCount);
    qsettings->setValue(warnBeforeOpeningBigTextFilesKey,
                        d->m_warnBeforeOpeningBigFilesEnabled);
    qsettings->setValue(bigTextFileSizeLimitKey, d->m_bigFileSizeLimitInMB);
    qsettings->setValue(maxRecentFilesKey, d->m_maxRecentFiles);

    Qt::CaseSensitivity defaultSensitivity
            = OsSpecificAspects::fileNameCaseSensitivity(HostOsInfo::hostOs());
    Qt::CaseSensitivity sensitivity = HostOsInfo::fileNameCaseSensitivity();
    if (defaultSensitivity == sensitivity)
        qsettings->remove(fileSystemCaseSensitivityKey);
    else
        qsettings->setValue(fileSystemCaseSensitivityKey, sensitivity);
    qsettings->setValue(preferredEditorFactoriesKey, toMap(userPreferredEditorFactories()));
}

void EditorManagerPrivate::readSettings()
{
    QSettings *qs = ICore::settings();
    if (qs->contains(warnBeforeOpeningBigTextFilesKey)) {
        d->m_warnBeforeOpeningBigFilesEnabled
                = qs->value(warnBeforeOpeningBigTextFilesKey).toBool();
        d->m_bigFileSizeLimitInMB = qs->value(bigTextFileSizeLimitKey).toInt();
    }

    const int maxRecentFiles = qs->value(maxRecentFilesKey).toInt();
    if (maxRecentFiles > 0)
        d->m_maxRecentFiles = maxRecentFiles;

    if (qs->contains(fileSystemCaseSensitivityKey)) {
        Qt::CaseSensitivity defaultSensitivity
                = OsSpecificAspects::fileNameCaseSensitivity(HostOsInfo::hostOs());
        bool ok = false;
        Qt::CaseSensitivity sensitivity = defaultSensitivity;
        int sensitivitySetting = qs->value(fileSystemCaseSensitivityKey).toInt(&ok);
        if (ok) {
            switch (Qt::CaseSensitivity(sensitivitySetting)) {
            case Qt::CaseSensitive:
                sensitivity = Qt::CaseSensitive;
                break;
            case Qt::CaseInsensitive:
                sensitivity = Qt::CaseInsensitive;
            }
        }
        if (sensitivity == defaultSensitivity)
            HostOsInfo::unsetOverrideFileNameCaseSensitivity();
        else
            HostOsInfo::setOverrideFileNameCaseSensitivity(sensitivity);
    }
    const QHash<Utils::MimeType, IEditorFactory *> preferredEditorFactories = fromMap(
        qs->value(preferredEditorFactoriesKey).toMap());
    setUserPreferredEditorFactories(preferredEditorFactories);

    SettingsDatabase *settings = ICore::settingsDatabase();
    if (settings->contains(documentStatesKey)) {
        d->m_editorStates = settings->value(documentStatesKey)
            .value<QMap<QString, QVariant> >();
    }

    if (settings->contains(reloadBehaviorKey)) {
        d->m_reloadSetting = IDocument::ReloadSetting(settings->value(reloadBehaviorKey).toInt());
        settings->remove(reloadBehaviorKey);
    }

    if (settings->contains(autoSaveEnabledKey)) {
        d->m_autoSaveEnabled = settings->value(autoSaveEnabledKey).toBool();
        d->m_autoSaveInterval = settings->value(autoSaveIntervalKey).toInt();
        settings->remove(autoSaveEnabledKey);
        settings->remove(autoSaveIntervalKey);
    }

    if (qs->contains(reloadBehaviorKey))
        d->m_reloadSetting = IDocument::ReloadSetting(qs->value(reloadBehaviorKey).toInt());

    if (qs->contains(autoSaveEnabledKey)) {
        d->m_autoSaveEnabled = qs->value(autoSaveEnabledKey).toBool();
        d->m_autoSaveInterval = qs->value(autoSaveIntervalKey).toInt();
    }

    if (qs->contains(autoSuspendEnabledKey)) {
        d->m_autoSuspendEnabled = qs->value(autoSuspendEnabledKey).toBool();
        d->m_autoSuspendMinDocumentCount = qs->value(autoSuspendMinDocumentCountKey).toInt();
    }

    updateAutoSave();
}

void EditorManagerPrivate::setAutoSaveEnabled(bool enabled)
{
    d->m_autoSaveEnabled = enabled;
    updateAutoSave();
}

bool EditorManagerPrivate::autoSaveEnabled()
{
    return d->m_autoSaveEnabled;
}

void EditorManagerPrivate::setAutoSaveInterval(int interval)
{
    d->m_autoSaveInterval = interval;
    updateAutoSave();
}

int EditorManagerPrivate::autoSaveInterval()
{
    return d->m_autoSaveInterval;
}

void EditorManagerPrivate::setAutoSuspendEnabled(bool enabled)
{
    d->m_autoSuspendEnabled = enabled;
}

bool EditorManagerPrivate::autoSuspendEnabled()
{
    return d->m_autoSuspendEnabled;
}

void EditorManagerPrivate::setAutoSuspendMinDocumentCount(int count)
{
    d->m_autoSuspendMinDocumentCount = count;
}

int EditorManagerPrivate::autoSuspendMinDocumentCount()
{
    return d->m_autoSuspendMinDocumentCount;
}

bool EditorManagerPrivate::warnBeforeOpeningBigFilesEnabled()
{
    return d->m_warnBeforeOpeningBigFilesEnabled;
}

void EditorManagerPrivate::setWarnBeforeOpeningBigFilesEnabled(bool enabled)
{
    d->m_warnBeforeOpeningBigFilesEnabled = enabled;
}

int EditorManagerPrivate::bigFileSizeLimit()
{
    return d->m_bigFileSizeLimitInMB;
}

void EditorManagerPrivate::setMaxRecentFiles(int count)
{
    d->m_maxRecentFiles = count;
}

int EditorManagerPrivate::maxRecentFiles()
{
    return d->m_maxRecentFiles;
}

void EditorManagerPrivate::setBigFileSizeLimit(int limitInMB)
{
    d->m_bigFileSizeLimitInMB = limitInMB;
}

EditorFactoryList EditorManagerPrivate::findFactories(Id editorId, const QString &fileName)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << editorId.name() << fileName;

    EditorFactoryList factories;
    if (!editorId.isValid()) {
        factories = IEditorFactory::preferredEditorFactories(fileName);
    } else {
        // Find by editor id
        IEditorFactory *factory = Utils::findOrDefault(IEditorFactory::allEditorFactories(),
                                                       Utils::equal(&IEditorFactory::id, editorId));
        if (factory)
            factories.push_back(factory);
    }
    if (factories.empty()) {
        qWarning("%s: unable to find an editor factory for the file '%s', editor Id '%s'.",
                 Q_FUNC_INFO, fileName.toUtf8().constData(), editorId.name().constData());
    }

    return factories;
}

IEditor *EditorManagerPrivate::createEditor(IEditorFactory *factory, const QString &fileName)
{
    if (!factory)
        return nullptr;

    IEditor *editor = factory->createEditor();
    if (editor) {
        QTC_CHECK(editor->document()->id().isValid()); // sanity check that the editor has an id set
        connect(editor->document(), &IDocument::changed, d, &EditorManagerPrivate::handleDocumentStateChange);
        emit m_instance->editorCreated(editor, fileName);
    }

    return editor;
}

void EditorManagerPrivate::addEditor(IEditor *editor)
{
    if (!editor)
        return;
    ICore::addContextObject(editor);

    bool isNewDocument = false;
    DocumentModelPrivate::addEditor(editor, &isNewDocument);
    if (isNewDocument) {
        IDocument *document = editor->document();
        const bool isTemporary = document->isTemporary();
        const bool addWatcher = !isTemporary;
        DocumentManager::addDocument(document, addWatcher);
        if (!isTemporary)
            DocumentManager::addToRecentFiles(document->filePath().toString(),
                                              document->id());
        emit m_instance->documentOpened(document);
    }
    emit m_instance->editorOpened(editor);
    QTimer::singleShot(0, d, &EditorManagerPrivate::autoSuspendDocuments);
}

void EditorManagerPrivate::removeEditor(IEditor *editor, bool removeSuspendedEntry)
{
    DocumentModel::Entry *entry = DocumentModelPrivate::removeEditor(editor);
    QTC_ASSERT(entry, return);
    if (entry->isSuspended) {
        IDocument *document = editor->document();
        DocumentManager::removeDocument(document);
        if (removeSuspendedEntry)
            DocumentModelPrivate::removeEntry(entry);
        emit m_instance->documentClosed(document);
    }
    ICore::removeContextObject(editor);
}

IEditor *EditorManagerPrivate::placeEditor(EditorView *view, IEditor *editor)
{
    Q_ASSERT(view && editor);

    if (view->hasEditor(editor))
        return editor;
    if (IEditor *e = view->editorForDocument(editor->document()))
        return e;

    const QByteArray state = editor->saveState();
    if (EditorView *sourceView = viewForEditor(editor)) {
        // try duplication or pull editor over to new view
        bool duplicateSupported = editor->duplicateSupported();
        if (editor != sourceView->currentEditor() || !duplicateSupported) {
            // pull the IEditor over to the new view
            sourceView->removeEditor(editor);
            view->addEditor(editor);
            view->setCurrentEditor(editor);
            // possibly adapts old state to new layout
            editor->restoreState(state);
            if (!sourceView->currentEditor()) {
                EditorView *replacementView = nullptr;
                if (IEditor *replacement = pickUnusedEditor(&replacementView)) {
                    if (replacementView)
                        replacementView->removeEditor(replacement);
                    sourceView->addEditor(replacement);
                    sourceView->setCurrentEditor(replacement);
                }
            }
            return editor;
        } else if (duplicateSupported) {
            editor = duplicateEditor(editor);
            Q_ASSERT(editor);
        }
    }
    view->addEditor(editor);
    view->setCurrentEditor(editor);
    // possibly adapts old state to new layout
    editor->restoreState(state);
    return editor;
}

IEditor *EditorManagerPrivate::duplicateEditor(IEditor *editor)
{
    if (!editor->duplicateSupported())
        return nullptr;

    IEditor *duplicate = editor->duplicate();
    emit m_instance->editorCreated(duplicate, duplicate->document()->filePath().toString());
    addEditor(duplicate);
    return duplicate;
}

IEditor *EditorManagerPrivate::activateEditor(EditorView *view, IEditor *editor,
                                              EditorManager::OpenEditorFlags flags)
{
    Q_ASSERT(view);

    if (!editor)
        return nullptr;

    editor = placeEditor(view, editor);

    if (!(flags & EditorManager::DoNotChangeCurrentEditor)) {
        setCurrentEditor(editor, (flags & EditorManager::IgnoreNavigationHistory));
        if (!(flags & EditorManager::DoNotMakeVisible)) {
            // switch to design mode?
            if (!(flags & EditorManager::DoNotSwitchToDesignMode) && editor->isDesignModePreferred()) {
                ModeManager::activateMode(Constants::MODE_DESIGN);
                ModeManager::setFocusToCurrentMode();
            } else {
                if (!(flags & EditorManager::DoNotSwitchToEditMode)) {
                    int index;
                    findEditorArea(view, &index);
                    if (index == 0) // main window --> we might need to switch mode
                        if (!editor->widget()->isVisible())
                            ModeManager::activateMode(Constants::MODE_EDIT);
                }
                editor->widget()->setFocus();
                ICore::raiseWindow(editor->widget());
            }
        }
    } else if (!(flags & EditorManager::DoNotMakeVisible)) {
        view->setCurrentEditor(editor);
    }
    return editor;
}

bool EditorManagerPrivate::activateEditorForEntry(EditorView *view, DocumentModel::Entry *entry,
                                                  EditorManager::OpenEditorFlags flags)
{
    QTC_ASSERT(view, return false);
    if (!entry) { // no document
        view->setCurrentEditor(nullptr);
        setCurrentView(view);
        setCurrentEditor(nullptr);
        return false;
    }
    IDocument *document = entry->document;
    if (!entry->isSuspended)  {
        IEditor *editor = activateEditorForDocument(view, document, flags);
        return editor != nullptr;
    }

    if (!openEditor(view, entry->fileName().toString(), entry->id(), flags)) {
        DocumentModelPrivate::removeEntry(entry);
        return false;
    }
    return true;
}

void EditorManagerPrivate::closeEditorOrDocument(IEditor *editor)
{
    QTC_ASSERT(editor, return);
    QList<IEditor *> visible = EditorManager::visibleEditors();
    if (Utils::contains(visible,
                        [&editor](IEditor *other) {
                            return editor != other && other->document() == editor->document();
                        })) {
        EditorManager::closeEditor(editor);
    } else {
        EditorManager::closeDocument(editor->document());
    }
}

bool EditorManagerPrivate::closeEditors(const QList<IEditor*> &editors, CloseFlag flag)
{
    if (editors.isEmpty())
        return true;
    bool closingFailed = false;
    // close Editor History list
    windowPopup()->setVisible(false);


    EditorView *currentView = currentEditorView();

    // go through all editors to close and
    // 1. ask all core listeners to check whether the editor can be closed
    // 2. keep track of the document and all the editors that might remain open for it
    QSet<IEditor*> acceptedEditors;
    QHash<IDocument *, QList<IEditor *> > editorsForDocuments;
    foreach (IEditor *editor, editors) {
        bool editorAccepted = true;
        foreach (const std::function<bool(IEditor*)> listener, d->m_closeEditorListeners) {
            if (!listener(editor)) {
                editorAccepted = false;
                closingFailed = true;
                break;
            }
        }
        if (editorAccepted) {
            acceptedEditors.insert(editor);
            IDocument *document = editor->document();
            if (!editorsForDocuments.contains(document)) // insert the document to track
                editorsForDocuments.insert(document, DocumentModel::editorsForDocument(document));
            // keep track that we'll close this editor for the document
            editorsForDocuments[document].removeAll(editor);
        }
    }
    if (acceptedEditors.isEmpty())
        return false;

    //ask whether to save modified documents that we are about to close
    if (flag == CloseFlag::CloseWithAsking) {
        // Check for which documents we will close all editors, and therefore might have to ask the user
        QList<IDocument *> documentsToClose;
        for (auto i = editorsForDocuments.constBegin(); i != editorsForDocuments.constEnd(); ++i) {
            if (i.value().isEmpty())
                documentsToClose.append(i.key());
        }

        bool cancelled = false;
        QList<IDocument *> rejectedList;
        DocumentManager::saveModifiedDocuments(documentsToClose, QString(), &cancelled,
                                               QString(), nullptr, &rejectedList);
        if (cancelled)
            return false;
        if (!rejectedList.isEmpty()) {
            closingFailed = true;
            QSet<IEditor*> skipSet = Utils::toSet(DocumentModel::editorsForDocuments(rejectedList));
            acceptedEditors = acceptedEditors.subtract(skipSet);
        }
    }
    if (acceptedEditors.isEmpty())
        return false;

    QList<EditorView*> closedViews;
    EditorView *focusView = nullptr;

    // remove the editors
    foreach (IEditor *editor, acceptedEditors) {
        emit m_instance->editorAboutToClose(editor);
        if (!editor->document()->filePath().isEmpty()
                && !editor->document()->isTemporary()) {
            QByteArray state = editor->saveState();
            if (!state.isEmpty())
                d->m_editorStates.insert(editor->document()->filePath().toString(), QVariant(state));
        }

        removeEditor(editor, flag != CloseFlag::Suspend);
        if (EditorView *view = viewForEditor(editor)) {
            if (QApplication::focusWidget() && QApplication::focusWidget() == editor->widget()->focusWidget())
                focusView = view;
            if (editor == view->currentEditor())
                closedViews += view;
            if (d->m_currentEditor == editor) {
                // avoid having a current editor without view
                setCurrentView(view);
                setCurrentEditor(nullptr);
            }
            view->removeEditor(editor);
        }
    }

    // TODO doesn't work as expected with multiple areas in main window and some other cases
    // instead each view should have its own file history and handle solely themselves
    // which editor is shown if their current editor closes
    EditorView *forceViewToShowEditor = nullptr;
    if (!closedViews.isEmpty() && EditorManager::visibleEditors().isEmpty()) {
        if (closedViews.contains(currentView))
            forceViewToShowEditor = currentView;
        else
            forceViewToShowEditor = closedViews.first();
    }
    foreach (EditorView *view, closedViews) {
        IEditor *newCurrent = view->currentEditor();
        if (!newCurrent && forceViewToShowEditor == view)
            newCurrent = pickUnusedEditor();
        if (newCurrent) {
            activateEditor(view, newCurrent, EditorManager::DoNotChangeCurrentEditor);
        } else if (forceViewToShowEditor == view) {
            DocumentModel::Entry *entry = DocumentModelPrivate::firstSuspendedEntry();
            if (entry) {
                activateEditorForEntry(view, entry, EditorManager::DoNotChangeCurrentEditor);
            } else { // no "suspended" ones, so any entry left should have a document
                const QList<DocumentModel::Entry *> documents = DocumentModel::entries();
                if (!documents.isEmpty()) {
                    if (IDocument *document = documents.last()->document) {
                        activateEditorForDocument(view, document, EditorManager::DoNotChangeCurrentEditor);
                    }
                }
            }
        }
    }

    emit m_instance->editorsClosed(Utils::toList(acceptedEditors));

    foreach (IEditor *editor, acceptedEditors)
        delete editor;

    if (focusView)
        activateView(focusView);
    else
        setCurrentEditor(currentView->currentEditor());

    if (!EditorManager::currentEditor()) {
        emit m_instance->currentEditorChanged(nullptr);
        updateActions();
    }

    return !closingFailed;
}

void EditorManagerPrivate::activateView(EditorView *view)
{
    QTC_ASSERT(view, return);
    QWidget *focusWidget;
    if (IEditor *editor = view->currentEditor()) {
        setCurrentEditor(editor, true);
        focusWidget = editor->widget();
    } else {
        setCurrentView(view);
        focusWidget = view;
    }
    focusWidget->setFocus();
    ICore::raiseWindow(focusWidget);
}

void EditorManagerPrivate::restoreEditorState(IEditor *editor)
{
    QTC_ASSERT(editor, return);
    QString fileName = editor->document()->filePath().toString();
    editor->restoreState(d->m_editorStates.value(fileName).toByteArray());
}

int EditorManagerPrivate::visibleDocumentsCount()
{
    const QList<IEditor *> editors = EditorManager::visibleEditors();
    const int editorsCount = editors.count();
    if (editorsCount < 2)
        return editorsCount;

    QSet<const IDocument *> visibleDocuments;
    foreach (IEditor *editor, editors) {
        if (const IDocument *document = editor->document())
            visibleDocuments << document;
    }
    return visibleDocuments.count();
}

void EditorManagerPrivate::setCurrentEditor(IEditor *editor, bool ignoreNavigationHistory)
{
    if (editor)
        setCurrentView(nullptr);

    if (d->m_currentEditor == editor)
        return;

    emit m_instance->currentEditorAboutToChange(d->m_currentEditor);

    if (d->m_currentEditor && !ignoreNavigationHistory)
        EditorManager::addCurrentPositionToNavigationHistory();

    d->m_currentEditor = editor;
    if (editor) {
        if (EditorView *view = viewForEditor(editor))
            view->setCurrentEditor(editor);
        // update global history
        EditorView::updateEditorHistory(editor, d->m_globalHistory);
    }
    updateActions();
    emit m_instance->currentEditorChanged(editor);
}

void EditorManagerPrivate::setCurrentView(EditorView *view)
{
    if (view == d->m_currentView)
        return;

    EditorView *old = d->m_currentView;
    d->m_currentView = view;

    if (old)
        old->update();
    if (view)
        view->update();
}

EditorArea *EditorManagerPrivate::findEditorArea(const EditorView *view, int *areaIndex)
{
    SplitterOrView *current = view->parentSplitterOrView();
    while (current) {
        if (auto area = qobject_cast<EditorArea *>(current)) {
            int index = d->m_editorAreas.indexOf(area);
            QTC_ASSERT(index >= 0, return nullptr);
            if (areaIndex)
                *areaIndex = index;
            return area;
        }
        current = current->findParentSplitter();
    }
    QTC_CHECK(false); // we should never have views without a editor area
    return nullptr;
}

void EditorManagerPrivate::closeView(EditorView *view)
{
    if (!view)
        return;

    const QList<IEditor *> editorsToDelete = emptyView(view);

    SplitterOrView *splitterOrView = view->parentSplitterOrView();
    Q_ASSERT(splitterOrView);
    Q_ASSERT(splitterOrView->view() == view);
    SplitterOrView *splitter = splitterOrView->findParentSplitter();
    Q_ASSERT(splitterOrView->hasEditors() == false);
    splitterOrView->hide();
    delete splitterOrView;

    splitter->unsplit();

    EditorView *newCurrent = splitter->findFirstView();
    if (newCurrent)
        EditorManagerPrivate::activateView(newCurrent);
    deleteEditors(editorsToDelete);
}

/*!
    Removes all editors from the view and from the document model, taking care of
    the handling of editors that are the last ones for the document.
    Returns the list of editors that were actually removed from the document model and
    need to be deleted with \c EditorManagerPrivate::deleteEditors.
    \internal
*/
const QList<IEditor *> EditorManagerPrivate::emptyView(EditorView *view)
{
    if (!view)
        return {};
    const QList<IEditor *> editors = view->editors();
    QList<IEditor *> removedEditors;
    for (IEditor *editor : editors) {
        if (DocumentModel::editorsForDocument(editor->document()).size() == 1) {
            // it's the only editor for that file
            // so we need to keep it around (--> in the editor model)
            if (EditorManager::currentEditor() == editor) {
                // we don't want a current editor that is not open in a view
                setCurrentView(view);
                setCurrentEditor(nullptr);
            }
            view->removeEditor(editor);
        } else {
            emit m_instance->editorAboutToClose(editor);
            removeEditor(editor, true /*=removeSuspendedEntry, but doesn't matter since it's not the last editor anyhow*/);
            view->removeEditor(editor);
            removedEditors.append(editor);
        }
    }
    return removedEditors;
}

/*!
    Signals editorsClosed() and deletes the editors.
    \internal
*/
void EditorManagerPrivate::deleteEditors(const QList<IEditor *> &editors)
{
    if (!editors.isEmpty()) {
        emit m_instance->editorsClosed(editors);
        qDeleteAll(editors);
    }
}

EditorWindow *EditorManagerPrivate::createEditorWindow()
{
    auto win = new EditorWindow;
    EditorArea *area = win->editorArea();
    d->m_editorAreas.append(area);
    connect(area, &QObject::destroyed, d, &EditorManagerPrivate::editorAreaDestroyed);
    return win;
}

void EditorManagerPrivate::splitNewWindow(EditorView *view)
{
    IEditor *editor = view->currentEditor();
    IEditor *newEditor = nullptr;
    const QByteArray state = editor ? editor->saveState() : QByteArray();
    if (editor && editor->duplicateSupported())
        newEditor = EditorManagerPrivate::duplicateEditor(editor);
    else
        newEditor = editor; // move to the new view

    EditorWindow *win = createEditorWindow();
    win->show();
    ICore::raiseWindow(win);
    if (newEditor) {
        activateEditor(win->editorArea()->view(), newEditor, EditorManager::IgnoreNavigationHistory);
        // possibly adapts old state to new layout
        newEditor->restoreState(state);
    } else {
        win->editorArea()->view()->setFocus();
    }
    updateActions();
}

IEditor *EditorManagerPrivate::pickUnusedEditor(EditorView **foundView)
{
    foreach (IEditor *editor, DocumentModel::editorsForOpenedDocuments()) {
        EditorView *view = viewForEditor(editor);
        if (!view || view->currentEditor() != editor) {
            if (foundView)
                *foundView = view;
            return editor;
        }
    }
    return nullptr;
}

/* Adds the file name to the recent files if there is at least one non-temporary editor for it */
void EditorManagerPrivate::addDocumentToRecentFiles(IDocument *document)
{
    if (document->isTemporary())
        return;
    DocumentModel::Entry *entry = DocumentModel::entryForDocument(document);
    if (!entry)
        return;
    DocumentManager::addToRecentFiles(document->filePath().toString(), entry->id());
}

void EditorManagerPrivate::updateAutoSave()
{
    if (d->m_autoSaveEnabled)
        d->m_autoSaveTimer->start(d->m_autoSaveInterval * (60 * 1000));
    else
        d->m_autoSaveTimer->stop();
}

void EditorManagerPrivate::updateMakeWritableWarning()
{
    IDocument *document = EditorManager::currentDocument();
    QTC_ASSERT(document, return);
    bool ww = document->isModified() && document->isFileReadOnly();
    if (ww != document->hasWriteWarning()) {
        document->setWriteWarning(ww);

        // Do this after setWriteWarning so we don't re-evaluate this part even
        // if we do not really show a warning.
        bool promptVCS = false;
        const QString directory = document->filePath().toFileInfo().absolutePath();
        IVersionControl *versionControl = VcsManager::findVersionControlForDirectory(directory);
        if (versionControl && versionControl->openSupportMode(document->filePath().toString()) != IVersionControl::NoOpen) {
            if (versionControl->settingsFlags() & IVersionControl::AutoOpen) {
                vcsOpenCurrentEditor();
                ww = false;
            } else {
                promptVCS = true;
            }
        }

        if (ww) {
            // we are about to change a read-only file, warn user
            if (promptVCS) {
                InfoBarEntry info(Id(kMakeWritableWarning),
                                  tr("<b>Warning:</b> This file was not opened in %1 yet.")
                                  .arg(versionControl->displayName()));
                info.setCustomButtonInfo(tr("Open"), &vcsOpenCurrentEditor);
                document->infoBar()->addInfo(info);
            } else {
                InfoBarEntry info(Id(kMakeWritableWarning),
                                  tr("<b>Warning:</b> You are changing a read-only file."));
                info.setCustomButtonInfo(tr("Make Writable"), &makeCurrentEditorWritable);
                document->infoBar()->addInfo(info);
            }
        } else {
            document->infoBar()->removeInfo(Id(kMakeWritableWarning));
        }
    }
}

void EditorManagerPrivate::setupSaveActions(IDocument *document, QAction *saveAction,
                                            QAction *saveAsAction, QAction *revertToSavedAction)
{
    const bool hasFile = document && !document->filePath().isEmpty();
    saveAction->setEnabled(hasFile && document->isModified());
    saveAsAction->setEnabled(document && document->isSaveAsAllowed());
    revertToSavedAction->setEnabled(hasFile);

    if (document && !document->displayName().isEmpty()) {
        const QString quotedName = QLatin1Char('"')
                + Utils::quoteAmpersands(document->displayName()) + QLatin1Char('"');
        saveAction->setText(tr("&Save %1").arg(quotedName));
        saveAsAction->setText(tr("Save %1 &As...").arg(quotedName));
        revertToSavedAction->setText(document->isModified()
                                     ? tr("Revert %1 to Saved").arg(quotedName)
                                     : tr("Reload %1").arg(quotedName));
    } else {
        saveAction->setText(EditorManager::tr("&Save"));
        saveAsAction->setText(EditorManager::tr("Save &As..."));
        revertToSavedAction->setText(EditorManager::tr("Revert to Saved"));
    }
}

void EditorManagerPrivate::updateActions()
{
    IDocument *curDocument = EditorManager::currentDocument();
    const int openedCount = DocumentModel::entryCount();

    if (curDocument)
        updateMakeWritableWarning();

    QString quotedName;
    if (curDocument)
        quotedName = QLatin1Char('"') + Utils::quoteAmpersands(curDocument->displayName())
                + QLatin1Char('"');
    setupSaveActions(curDocument, d->m_saveAction, d->m_saveAsAction, d->m_revertToSavedAction);

    d->m_closeCurrentEditorAction->setEnabled(curDocument);
    d->m_closeCurrentEditorAction->setText(tr("Close %1").arg(quotedName));
    d->m_closeAllEditorsAction->setEnabled(openedCount > 0);
    d->m_closeOtherDocumentsAction->setEnabled(openedCount > 1);
    d->m_closeOtherDocumentsAction->setText((openedCount > 1 ? tr("Close All Except %1").arg(quotedName) : tr("Close Others")));

    d->m_closeAllEditorsExceptVisibleAction->setEnabled(visibleDocumentsCount() < openedCount);

    d->m_gotoNextDocHistoryAction->setEnabled(openedCount != 0);
    d->m_gotoPreviousDocHistoryAction->setEnabled(openedCount != 0);
    EditorView *view  = currentEditorView();
    d->m_goBackAction->setEnabled(view ? view->canGoBack() : false);
    d->m_goForwardAction->setEnabled(view ? view->canGoForward() : false);

    SplitterOrView *viewParent = (view ? view->parentSplitterOrView() : nullptr);
    SplitterOrView *parentSplitter = (viewParent ? viewParent->findParentSplitter() : nullptr);
    bool hasSplitter = parentSplitter && parentSplitter->isSplitter();
    d->m_removeCurrentSplitAction->setEnabled(hasSplitter);
    d->m_removeAllSplitsAction->setEnabled(hasSplitter);
    d->m_gotoNextSplitAction->setEnabled(hasSplitter || d->m_editorAreas.size() > 1);
}

void EditorManagerPrivate::updateWindowTitleForDocument(IDocument *document, QWidget *window)
{
    QTC_ASSERT(window, return);
    QString windowTitle;
    const QString dashSep(" - ");

    const QString documentName = document ? document->displayName() : QString();
    if (!documentName.isEmpty())
        windowTitle.append(documentName);

    const QString filePath = document ? document->filePath().toFileInfo().absoluteFilePath()
                              : QString();
    const QString windowTitleAddition = d->m_titleAdditionHandler
            ? d->m_titleAdditionHandler(filePath)
            : QString();
    if (!windowTitleAddition.isEmpty()) {
        if (!windowTitle.isEmpty())
            windowTitle.append(" ");
        windowTitle.append(windowTitleAddition);
    }

    const QString windowTitleVcsTopic = d->m_titleVcsTopicHandler
           ? d->m_titleVcsTopicHandler(filePath)
           : QString();
    if (!windowTitleVcsTopic.isEmpty()) {
        if (!windowTitle.isEmpty())
            windowTitle.append(" ");
        windowTitle.append(QStringLiteral("[") + windowTitleVcsTopic + QStringLiteral("]"));
    }

    const QString sessionTitle = d->m_sessionTitleHandler
           ? d->m_sessionTitleHandler(filePath)
           : QString();
    if (!sessionTitle.isEmpty()) {
        if (!windowTitle.isEmpty())
            windowTitle.append(dashSep);
        windowTitle.append(sessionTitle);
    }

    if (!windowTitle.isEmpty())
        windowTitle.append(dashSep);
    windowTitle.append(Core::Constants::IDE_DISPLAY_NAME);
    window->window()->setWindowTitle(windowTitle);
    window->window()->setWindowFilePath(filePath);

    if (HostOsInfo::isMacHost()) {
        if (document)
            window->window()->setWindowModified(document->isModified());
        else
            window->window()->setWindowModified(false);
    }
}

void EditorManagerPrivate::updateWindowTitle()
{
    EditorArea *mainArea = mainEditorArea();
    IDocument *document = mainArea->currentDocument();
    updateWindowTitleForDocument(document, mainArea->window());
}

void EditorManagerPrivate::gotoNextDocHistory()
{
    OpenEditorsWindow *dialog = windowPopup();
    if (dialog->isVisible()) {
        dialog->selectNextEditor();
    } else {
        EditorView *view = currentEditorView();
        dialog->setEditors(d->m_globalHistory, view);
        dialog->selectNextEditor();
        showPopupOrSelectDocument();
    }
}

void EditorManagerPrivate::gotoPreviousDocHistory()
{
    OpenEditorsWindow *dialog = windowPopup();
    if (dialog->isVisible()) {
        dialog->selectPreviousEditor();
    } else {
        EditorView *view = currentEditorView();
        dialog->setEditors(d->m_globalHistory, view);
        dialog->selectPreviousEditor();
        showPopupOrSelectDocument();
    }
}

void EditorManagerPrivate::gotoLastEditLocation()
{
    currentEditorView()->goToEditLocation(d->m_globalLastEditLocation);
}

void EditorManagerPrivate::gotoNextSplit()
{
    EditorView *view = currentEditorView();
    if (!view)
        return;
    EditorView *nextView = view->findNextView();
    if (!nextView) {
        // we are in the "last" view in this editor area
        int index = -1;
        EditorArea *area = findEditorArea(view, &index);
        QTC_ASSERT(area, return);
        QTC_ASSERT(index >= 0 && index < d->m_editorAreas.size(), return);
        // find next editor area. this might be the same editor area if there's only one.
        int nextIndex = index + 1;
        if (nextIndex >= d->m_editorAreas.size())
            nextIndex = 0;
        nextView = d->m_editorAreas.at(nextIndex)->findFirstView();
    }

    if (QTC_GUARD(nextView))
        activateView(nextView);
}

void EditorManagerPrivate::gotoPreviousSplit()
{
    EditorView *view = currentEditorView();
    if (!view)
        return;
    EditorView *prevView = view->findPreviousView();
    if (!prevView) {
        // we are in the "first" view in this editor area
        int index = -1;
        EditorArea *area = findEditorArea(view, &index);
        QTC_ASSERT(area, return);
        QTC_ASSERT(index >= 0 && index < d->m_editorAreas.size(), return);
        // find previous editor area. this might be the same editor area if there's only one.
        int nextIndex = index - 1;
        if (nextIndex < 0)
            nextIndex = d->m_editorAreas.count() - 1;
        prevView = d->m_editorAreas.at(nextIndex)->findLastView();
    }

    if (QTC_GUARD(prevView))
        activateView(prevView);
}

void EditorManagerPrivate::makeCurrentEditorWritable()
{
    if (IDocument* doc = EditorManager::currentDocument())
        makeFileWritable(doc);
}

void EditorManagerPrivate::setPlaceholderText(const QString &text)
{
    if (d->m_placeholderText == text)
        return;
    d->m_placeholderText = text;
    emit d->placeholderTextChanged(d->m_placeholderText);
}

QString EditorManagerPrivate::placeholderText()
{
    return d->m_placeholderText;
}

void EditorManagerPrivate::vcsOpenCurrentEditor()
{
    IDocument *document = EditorManager::currentDocument();
    if (!document)
        return;

    const QString directory = document->filePath().toFileInfo().absolutePath();
    IVersionControl *versionControl = VcsManager::findVersionControlForDirectory(directory);
    if (!versionControl || versionControl->openSupportMode(document->filePath().toString()) == IVersionControl::NoOpen)
        return;

    if (!versionControl->vcsOpen(document->filePath().toString())) {
        // TODO: wrong dialog parent
        QMessageBox::warning(ICore::dialogParent(), tr("Cannot Open File"),
                             tr("Cannot open the file for editing with VCS."));
    }
}

void EditorManagerPrivate::handleDocumentStateChange()
{
    updateActions();
    auto document = qobject_cast<IDocument *>(sender());
    if (!document->isModified())
        document->removeAutoSaveFile();
    if (EditorManager::currentDocument() == document)
        emit m_instance->currentDocumentStateChanged();
    emit m_instance->documentStateChanged(document);
}

void EditorManagerPrivate::editorAreaDestroyed(QObject *area)
{
    QWidget *activeWin = QApplication::activeWindow();
    EditorArea *newActiveArea = nullptr;
    for (int i = 0; i < d->m_editorAreas.size(); ++i) {
        EditorArea *r = d->m_editorAreas.at(i);
        if (r == area) {
            d->m_editorAreas.removeAt(i);
            --i; // we removed the current one
        } else if (r->window() == activeWin) {
            newActiveArea = r;
        }
    }
    // check if the destroyed editor area had the current view or current editor
    if (d->m_currentEditor || (d->m_currentView && d->m_currentView->parentSplitterOrView() != area))
        return;
    // we need to set a new current editor or view
    if (!newActiveArea) {
        // some window managers behave weird and don't activate another window
        // or there might be a Qt Creator toplevel activated that doesn't have editor windows
        newActiveArea = d->m_editorAreas.first();
    }

    // check if the focusWidget points to some view
    SplitterOrView *focusSplitterOrView = nullptr;
    QWidget *candidate = newActiveArea->focusWidget();
    while (candidate && candidate != newActiveArea) {
        if ((focusSplitterOrView = qobject_cast<SplitterOrView *>(candidate)))
            break;
        candidate = candidate->parentWidget();
    }
    // focusWidget might have been 0
    if (!focusSplitterOrView)
        focusSplitterOrView = newActiveArea->findFirstView()->parentSplitterOrView();
    QTC_ASSERT(focusSplitterOrView, focusSplitterOrView = newActiveArea);
    EditorView *focusView = focusSplitterOrView->findFirstView(); // can be just focusSplitterOrView
    QTC_ASSERT(focusView, focusView = newActiveArea->findFirstView());
    QTC_ASSERT(focusView, return);
    EditorManagerPrivate::activateView(focusView);
}

void EditorManagerPrivate::autoSave()
{
    QStringList errors;
    // FIXME: the saving should be staggered
    foreach (IDocument *document, DocumentModel::openedDocuments()) {
        if (!document->isModified() || !document->shouldAutoSave())
            continue;
        const QString saveName = autoSaveName(document->filePath().toString());
        const QString savePath = QFileInfo(saveName).absolutePath();
        if (document->filePath().isEmpty()
                || !QFileInfo(savePath).isWritable()) // FIXME: save them to a dedicated directory
            continue;
        QString errorString;
        if (!document->autoSave(&errorString, saveName))
            errors << errorString;
    }
    if (!errors.isEmpty())
        QMessageBox::critical(ICore::dialogParent(),
                              tr("File Error"),
                              errors.join(QLatin1Char('\n')));
    emit m_instance->autoSaved();
}

void EditorManagerPrivate::handleContextChange(const QList<IContext *> &context)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO;
    d->m_scheduledCurrentEditor = nullptr;
    IEditor *editor = nullptr;
    foreach (IContext *c, context)
        if ((editor = qobject_cast<IEditor*>(c)))
            break;
    if (editor && editor != d->m_currentEditor) {
        // Delay actually setting the current editor to after the current event queue has been handled
        // Without doing this, e.g. clicking into projects tree or locator would always open editors
        // in the main window. That is because clicking anywhere in the main window (even over e.g.
        // the locator line edit) first activates the window and sets focus to its focus widget.
        // Only afterwards the focus is shifted to the widget that received the click.
        d->m_scheduledCurrentEditor = editor;
        QTimer::singleShot(0, d, &EditorManagerPrivate::setCurrentEditorFromContextChange);
    } else {
        updateActions();
    }
}

void EditorManagerPrivate::copyFilePathFromContextMenu()
{
    if (!d->m_contextMenuEntry)
        return;
    QApplication::clipboard()->setText(d->m_contextMenuEntry->fileName().toUserOutput());
}

void EditorManagerPrivate::copyLocationFromContextMenu()
{
    const auto action = qobject_cast<const QAction *>(sender());
    if (!d->m_contextMenuEntry || !action)
        return;
    const QString text = d->m_contextMenuEntry->fileName().toUserOutput()
            + QLatin1Char(':') + action->data().toString();
    QApplication::clipboard()->setText(text);
}

void EditorManagerPrivate::copyFileNameFromContextMenu()
{
    if (!d->m_contextMenuEntry)
        return;
    QApplication::clipboard()->setText(d->m_contextMenuEntry->fileName().fileName());
}

void EditorManagerPrivate::saveDocumentFromContextMenu()
{
    IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : nullptr;
    if (document)
        saveDocument(document);
}

void EditorManagerPrivate::saveDocumentAsFromContextMenu()
{
    IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : nullptr;
    if (document)
        saveDocumentAs(document);
}

void EditorManagerPrivate::revertToSavedFromContextMenu()
{
    IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : nullptr;
    if (document)
        revertToSaved(document);
}

void EditorManagerPrivate::closeEditorFromContextMenu()
{
    if (d->m_contextMenuEditor) {
        closeEditorOrDocument(d->m_contextMenuEditor);
    } else {
        IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : nullptr;
        if (document)
            EditorManager::closeDocument(document);
    }
}

void EditorManagerPrivate::closeOtherDocumentsFromContextMenu()
{
    IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : nullptr;
    EditorManager::closeOtherDocuments(document);
}

bool EditorManagerPrivate::saveDocument(IDocument *document)
{
    if (!document)
        return false;

    document->checkPermissions();

    const QString fileName = document->filePath().toString();

    if (fileName.isEmpty())
        return saveDocumentAs(document);

    bool success = false;
    bool isReadOnly;

    emit m_instance->aboutToSave(document);
    // try saving, no matter what isReadOnly tells us
    success = DocumentManager::saveDocument(document, QString(), &isReadOnly);

    if (!success && isReadOnly) {
        MakeWritableResult answer = makeFileWritable(document);
        if (answer == Failed)
            return false;
        if (answer == SavedAs)
            return true;

        document->checkPermissions();

        success = DocumentManager::saveDocument(document);
    }

    if (success) {
        addDocumentToRecentFiles(document);
        emit m_instance->saved(document);
    }

    return success;
}

bool EditorManagerPrivate::saveDocumentAs(IDocument *document)
{
    if (!document)
        return false;

    const QString &absoluteFilePath =
        DocumentManager::getSaveAsFileName(document);

    if (absoluteFilePath.isEmpty())
        return false;

    if (absoluteFilePath != document->filePath().toString()) {
        // close existing editors for the new file name
        IDocument *otherDocument = DocumentModel::documentForFilePath(absoluteFilePath);
        if (otherDocument)
            EditorManager::closeDocuments({otherDocument}, false);
    }

    emit m_instance->aboutToSave(document);
    const bool success = DocumentManager::saveDocument(document, absoluteFilePath);
    document->checkPermissions();

    // TODO: There is an issue to be treated here. The new file might be of a different mime
    // type than the original and thus require a different editor. An alternative strategy
    // would be to close the current editor and open a new appropriate one, but this is not
    // a good way out either (also the undo stack would be lost). Perhaps the best is to
    // re-think part of the editors design.

    if (success) {
        addDocumentToRecentFiles(document);
        emit m_instance->saved(document);
    }

    updateActions();
    return success;
}

void EditorManagerPrivate::closeAllEditorsExceptVisible()
{
    DocumentModelPrivate::removeAllSuspendedEntries(DocumentModelPrivate::DoNotRemovePinnedFiles);
    QList<IDocument *> documentsToClose = DocumentModel::openedDocuments();
    // Remove all pinned files from the list of files to close.
    documentsToClose = Utils::filtered(documentsToClose, [](IDocument *document) {
        DocumentModel::Entry *entry = DocumentModel::entryForDocument(document);
        return !entry->pinned;
    });
    foreach (IEditor *editor, EditorManager::visibleEditors())
        documentsToClose.removeAll(editor->document());
    EditorManager::closeDocuments(documentsToClose, true);
}

void EditorManagerPrivate::revertToSaved(IDocument *document)
{
    if (!document)
        return;
    const QString fileName =  document->filePath().toString();
    if (fileName.isEmpty())
        return;
    if (document->isModified()) {
        QMessageBox msgBox(QMessageBox::Question,
                           tr("Revert to Saved"),
                           tr("You will lose your current changes if you proceed reverting %1.")
                               .arg(QDir::toNativeSeparators(fileName)),
                           QMessageBox::Yes | QMessageBox::No,
                           ICore::dialogParent());
        msgBox.button(QMessageBox::Yes)->setText(tr("Proceed"));
        msgBox.button(QMessageBox::No)->setText(tr("Cancel"));

        QPushButton *diffButton = nullptr;
        auto diffService = DiffService::instance();
        if (diffService)
            diffButton = msgBox.addButton(tr("Cancel && &Diff"), QMessageBox::RejectRole);

        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;

        if (diffService && msgBox.clickedButton() == diffButton) {
            diffService->diffModifiedFiles(QStringList(fileName));
            return;
        }
    }
    QString errorString;
    if (!document->reload(&errorString, IDocument::FlagReload, IDocument::TypeContents))
        QMessageBox::critical(ICore::dialogParent(), tr("File Error"), errorString);
}

void EditorManagerPrivate::autoSuspendDocuments()
{
    if (!d->m_autoSuspendEnabled)
        return;

    auto visibleDocuments = Utils::transform<QSet>(EditorManager::visibleEditors(),
                                                   &IEditor::document);
    int keptEditorCount = 0;
    QList<IDocument *> documentsToSuspend;
    foreach (const EditLocation &editLocation, d->m_globalHistory) {
        IDocument *document = editLocation.document;
        if (!document || !document->isSuspendAllowed() || document->isModified()
                || document->isTemporary() || document->filePath().isEmpty()
                || visibleDocuments.contains(document))
            continue;
        if (keptEditorCount >= d->m_autoSuspendMinDocumentCount)
            documentsToSuspend.append(document);
        else
            ++keptEditorCount;
    }
    closeEditors(DocumentModel::editorsForDocuments(documentsToSuspend), CloseFlag::Suspend);
}

void EditorManagerPrivate::showInGraphicalShell()
{
    if (!d->m_contextMenuEntry || d->m_contextMenuEntry->fileName().isEmpty())
        return;
    FileUtils::showInGraphicalShell(ICore::dialogParent(),
                                    d->m_contextMenuEntry->fileName().toString());
}

void EditorManagerPrivate::openTerminal()
{
    if (!d->m_contextMenuEntry || d->m_contextMenuEntry->fileName().isEmpty())
        return;
    FileUtils::openTerminal(d->m_contextMenuEntry->fileName().parentDir().toString());
}

void EditorManagerPrivate::findInDirectory()
{
    if (!d->m_contextMenuEntry || d->m_contextMenuEntry->fileName().isEmpty())
        return;
    const FilePath path = d->m_contextMenuEntry->fileName();
    emit m_instance->findOnFileSystemRequest(
        (path.isDir() ? path : path.parentDir()).toString());
}

void EditorManagerPrivate::togglePinned()
{
    if (!d->m_contextMenuEntry || d->m_contextMenuEntry->fileName().isEmpty())
        return;

    const bool currentlyPinned = d->m_contextMenuEntry->pinned;
    DocumentModelPrivate::setPinned(d->m_contextMenuEntry, !currentlyPinned);
}

void EditorManagerPrivate::split(Qt::Orientation orientation)
{
    EditorView *view = currentEditorView();

    if (view)
        view->parentSplitterOrView()->split(orientation);

    updateActions();
}

void EditorManagerPrivate::removeCurrentSplit()
{
    EditorView *viewToClose = currentEditorView();

    QTC_ASSERT(viewToClose, return);
    QTC_ASSERT(!qobject_cast<EditorArea *>(viewToClose->parentSplitterOrView()), return);

    closeView(viewToClose);
    updateActions();
}

void EditorManagerPrivate::removeAllSplits()
{
    EditorView *view = currentEditorView();
    QTC_ASSERT(view, return);
    EditorArea *currentArea = findEditorArea(view);
    QTC_ASSERT(currentArea, return);
    currentArea->unsplitAll();
}

void EditorManagerPrivate::setCurrentEditorFromContextChange()
{
    if (!d->m_scheduledCurrentEditor)
        return;
    IEditor *newCurrent = d->m_scheduledCurrentEditor;
    d->m_scheduledCurrentEditor = nullptr;
    setCurrentEditor(newCurrent);
}

EditorView *EditorManagerPrivate::currentEditorView()
{
    EditorView *view = d->m_currentView;
    if (!view) {
        if (d->m_currentEditor) {
            view = EditorManagerPrivate::viewForEditor(d->m_currentEditor);
            QTC_ASSERT(view, view = d->m_editorAreas.first()->findFirstView());
        }
        QTC_CHECK(view);
        if (!view) { // should not happen, we should always have either currentview or currentdocument
            foreach (EditorArea *area, d->m_editorAreas) {
                if (area->window()->isActiveWindow()) {
                    view = area->findFirstView();
                    break;
                }
            }
            QTC_ASSERT(view, view = d->m_editorAreas.first()->findFirstView());
        }
    }
    return view;
}


EditorManager *EditorManager::instance() { return m_instance; }

/*!
    \internal
*/
EditorManager::EditorManager(QObject *parent) :
    QObject(parent)
{
    m_instance = this;
    d = new EditorManagerPrivate(this);
    d->init();
}

/*!
    \internal
*/
EditorManager::~EditorManager()
{
    delete d;
    m_instance = nullptr;
}

/*!
    Returns the document of the currently active editor.
*/
IDocument *EditorManager::currentDocument()
{
    return d->m_currentEditor ? d->m_currentEditor->document() : nullptr;
}

/*!
    Returns the currently active editor.
*/
IEditor *EditorManager::currentEditor()
{
    return d->m_currentEditor;
}

/*!
    Closes all open editors. If \a askAboutModifiedEditors is \c true, prompts
    users to save their changes before closing the editors.

    Returns whether all editors were closed.
*/
bool EditorManager::closeAllEditors(bool askAboutModifiedEditors)
{
    DocumentModelPrivate::removeAllSuspendedEntries();
    return closeDocuments(DocumentModel::openedDocuments(), askAboutModifiedEditors);
}

/*!
    Closes all open documents except \a document and pinned files.
*/
void EditorManager::closeOtherDocuments(IDocument *document)
{
    DocumentModelPrivate::removeAllSuspendedEntries(DocumentModelPrivate::DoNotRemovePinnedFiles);
    QList<IDocument *> documentsToClose = DocumentModel::openedDocuments();
    // Remove all pinned files from the list of files to close.
    documentsToClose = Utils::filtered(documentsToClose, [](IDocument *document) {
        DocumentModel::Entry *entry = DocumentModel::entryForDocument(document);
        return !entry->pinned;
    });
    documentsToClose.removeAll(document);
    closeDocuments(documentsToClose, true);
}

/*!
    Closes all open documents except pinned files.

    Returns whether all editors were closed.
*/
bool EditorManager::closeAllDocuments()
{
    // Only close the files that aren't pinned.
    const QList<DocumentModel::Entry *> entriesToClose
            = Utils::filtered(DocumentModel::entries(), Utils::equal(&DocumentModel::Entry::pinned, false));
    return EditorManager::closeDocuments(entriesToClose);
}

// SLOT connected to action
void EditorManager::slotCloseCurrentEditorOrDocument()
{
    if (!d->m_currentEditor)
        return;
    addCurrentPositionToNavigationHistory();
    d->closeEditorOrDocument(d->m_currentEditor);
}

/*!
    Closes all open documents except the current document.
*/
void EditorManager::closeOtherDocuments()
{
    closeOtherDocuments(currentDocument());
}

static void assignAction(QAction *self, QAction *other)
{
    self->setText(other->text());
    self->setIcon(other->icon());
    self->setShortcut(other->shortcut());
    self->setEnabled(other->isEnabled());
    self->setIconVisibleInMenu(other->isIconVisibleInMenu());
}

/*!
    Adds save, close and other editor context menu items for the document
    \a entry and editor \a editor to the context menu \a contextMenu.
*/
void EditorManager::addSaveAndCloseEditorActions(QMenu *contextMenu, DocumentModel::Entry *entry,
                                                 IEditor *editor)
{
    QTC_ASSERT(contextMenu, return);
    d->m_contextMenuEntry = entry;
    d->m_contextMenuEditor = editor;

    const FilePath filePath = entry ? entry->fileName() : FilePath();
    const bool copyActionsEnabled = !filePath.isEmpty();
    d->m_copyFilePathContextAction->setEnabled(copyActionsEnabled);
    d->m_copyLocationContextAction->setEnabled(copyActionsEnabled);
    d->m_copyFileNameContextAction->setEnabled(copyActionsEnabled);
    contextMenu->addAction(d->m_copyFilePathContextAction);
    if (editor && entry) {
        if (const int lineNumber = editor->currentLine()) {
            d->m_copyLocationContextAction->setData(QVariant(lineNumber));
            contextMenu->addAction(d->m_copyLocationContextAction);
        }
    }
    contextMenu->addAction(d->m_copyFileNameContextAction);
    contextMenu->addSeparator();

    assignAction(d->m_saveCurrentEditorContextAction, ActionManager::command(Constants::SAVE)->action());
    assignAction(d->m_saveAsCurrentEditorContextAction, ActionManager::command(Constants::SAVEAS)->action());
    assignAction(d->m_revertToSavedCurrentEditorContextAction, ActionManager::command(Constants::REVERTTOSAVED)->action());

    IDocument *document = entry ? entry->document : nullptr;

    EditorManagerPrivate::setupSaveActions(document,
                                           d->m_saveCurrentEditorContextAction,
                                           d->m_saveAsCurrentEditorContextAction,
                                           d->m_revertToSavedCurrentEditorContextAction);

    contextMenu->addAction(d->m_saveCurrentEditorContextAction);
    contextMenu->addAction(d->m_saveAsCurrentEditorContextAction);
    contextMenu->addAction(ActionManager::command(Constants::SAVEALL)->action());
    contextMenu->addAction(d->m_revertToSavedCurrentEditorContextAction);

    contextMenu->addSeparator();

    const QString quotedDisplayName = entry ? Utils::quoteAmpersands(entry->displayName()) : QString();
    d->m_closeCurrentEditorContextAction->setText(entry
                                                    ? tr("Close \"%1\"").arg(quotedDisplayName)
                                                    : tr("Close Editor"));
    d->m_closeOtherDocumentsContextAction->setText(entry
                                                   ? tr("Close All Except \"%1\"").arg(quotedDisplayName)
                                                   : tr("Close Other Editors"));
    d->m_closeCurrentEditorContextAction->setEnabled(entry != nullptr);
    d->m_closeOtherDocumentsContextAction->setEnabled(entry != nullptr);
    d->m_closeAllEditorsContextAction->setEnabled(!DocumentModel::entries().isEmpty());
    d->m_closeAllEditorsExceptVisibleContextAction->setEnabled(
                EditorManagerPrivate::visibleDocumentsCount() < DocumentModel::entries().count());
    contextMenu->addAction(d->m_closeCurrentEditorContextAction);
    contextMenu->addAction(d->m_closeAllEditorsContextAction);
    contextMenu->addAction(d->m_closeOtherDocumentsContextAction);
    contextMenu->addAction(d->m_closeAllEditorsExceptVisibleContextAction);
}

/*!
    Adds the pin editor menu items for the document \a entry to the context menu
    \a contextMenu.
*/
void EditorManager::addPinEditorActions(QMenu *contextMenu, DocumentModel::Entry *entry)
{
    const QString quotedDisplayName = entry ? Utils::quoteAmpersands(entry->displayName()) : QString();
    if (entry) {
        d->m_pinAction->setText(entry->pinned
                                ? tr("Unpin \"%1\"").arg(quotedDisplayName)
                                : tr("Pin \"%1\"").arg(quotedDisplayName));
    } else {
        d->m_pinAction->setText(tr("Pin Editor"));
    }
    d->m_pinAction->setEnabled(entry != nullptr);
    contextMenu->addAction(d->m_pinAction);
}

/*!
    Adds the native directory handling and open with menu items for the document
    \a entry to the context menu \a contextMenu.
*/
void EditorManager::addNativeDirAndOpenWithActions(QMenu *contextMenu, DocumentModel::Entry *entry)
{
    QTC_ASSERT(contextMenu, return);
    d->m_contextMenuEntry = entry;
    bool enabled = entry && !entry->fileName().isEmpty();
    d->m_openGraphicalShellAction->setEnabled(enabled);
    d->m_openTerminalAction->setEnabled(enabled);
    d->m_findInDirectoryAction->setEnabled(enabled);
    d->m_filePropertiesAction->setEnabled(enabled);
    contextMenu->addAction(d->m_openGraphicalShellAction);
    contextMenu->addAction(d->m_openTerminalAction);
    contextMenu->addAction(d->m_findInDirectoryAction);
    contextMenu->addAction(d->m_filePropertiesAction);
    QMenu *openWith = contextMenu->addMenu(tr("Open With"));
    openWith->setEnabled(enabled);
    if (enabled)
        populateOpenWithMenu(openWith, entry->fileName().toString());
}

/*!
    Populates the \uicontrol {Open With} menu \a menu with editors that are
    suitable for opening the document \a fileName.
*/
void EditorManager::populateOpenWithMenu(QMenu *menu, const QString &fileName)
{
    using EditorFactoryList = QList<IEditorFactory*>;
    using ExternalEditorList = QList<IExternalEditor*>;

    menu->clear();

    const EditorFactoryList factories = IEditorFactory::preferredEditorFactories(fileName);
    const Utils::MimeType mt = Utils::mimeTypeForFile(fileName);
    const ExternalEditorList extEditors = IExternalEditor::externalEditors(mt);
    const bool anyMatches = !factories.empty() || !extEditors.empty();
    if (anyMatches) {
        // Add all suitable editors
        foreach (IEditorFactory *editorFactory, factories) {
            Utils::Id editorId = editorFactory->id();
            // Add action to open with this very editor factory
            QString const actionTitle = editorFactory->displayName();
            QAction *action = menu->addAction(actionTitle);
            // Below we need QueuedConnection because otherwise, if a qrc file
            // is inside of a qrc file itself, and the qrc editor opens the Open with menu,
            // crashes happen, because the editor instance is deleted by openEditorWith
            // while the menu is still being processed.
            connect(action, &QAction::triggered, d,
                    [fileName, editorId]() {
                        EditorManagerPrivate::openEditorWith(fileName, editorId);
                    }, Qt::QueuedConnection);
        }
        // Add all suitable external editors
        foreach (IExternalEditor *externalEditor, extEditors) {
            QAction *action = menu->addAction(externalEditor->displayName());
            Utils::Id editorId = externalEditor->id();
            connect(action, &QAction::triggered, [fileName, editorId]() {
                EditorManager::openExternalEditor(fileName, editorId);
            });
        }
    }
    menu->setEnabled(anyMatches);
}

/*!
    Returns reload behavior settings.
*/
IDocument::ReloadSetting EditorManager::reloadSetting()
{
    return d->m_reloadSetting;
}

/*!
    Sets editor reaload behavior settings to \a behavior.
*/
void EditorManager::setReloadSetting(IDocument::ReloadSetting behavior)
{
     d->m_reloadSetting = behavior;
}

/*!
    Saves the current document.
*/
void EditorManager::saveDocument()
{
    EditorManagerPrivate::saveDocument(currentDocument());
}

/*!
    Saves the current document under a different file name.
*/
void EditorManager::saveDocumentAs()
{
    EditorManagerPrivate::saveDocumentAs(currentDocument());
}

/*!
    Reverts the current document to its last saved state.
*/
void EditorManager::revertToSaved()
{
    EditorManagerPrivate::revertToSaved(currentDocument());
}

/*!
    Closes \a editor. If \a askAboutModifiedEditors is \c true, prompts
    users to save their changes before closing the editor.
*/
void EditorManager::closeEditor(IEditor *editor, bool askAboutModifiedEditors)
{
    if (editor)
        closeEditors({editor}, askAboutModifiedEditors);
}

/*!
    Closes the document specified by \a entry.
*/
void EditorManager::closeDocument(DocumentModel::Entry *entry)
{
    if (!entry)
        return;
    if (entry->isSuspended)
        DocumentModelPrivate::removeEntry(entry);
    else
        closeDocuments({entry->document});
}

/*!
    Closes the documents specified by \a entries.

    Returns whether all documents were closed.
*/
bool EditorManager::closeDocuments(const QList<DocumentModel::Entry *> &entries)
{
    QList<IDocument *> documentsToClose;
    for (DocumentModel::Entry *entry : entries) {
        if (!entry)
            continue;
        if (entry->isSuspended)
            DocumentModelPrivate::removeEntry(entry);
        else
            documentsToClose << entry->document;
    }
    return closeDocuments(documentsToClose);
}

/*!
    Closes the editors specified by \a editorsToClose. If
    \a askAboutModifiedEditors is \c true, prompts users
    to save their changes before closing the editor.

    Returns whether all editors were closed.
*/
bool EditorManager::closeEditors(const QList<IEditor*> &editorsToClose, bool askAboutModifiedEditors)
{
    return EditorManagerPrivate::closeEditors(editorsToClose,
                                              askAboutModifiedEditors ? EditorManagerPrivate::CloseFlag::CloseWithAsking
                                                                      : EditorManagerPrivate::CloseFlag::CloseWithoutAsking);
}

/*!
    Activates the editor for the document specified by \a entry using the editor
    settings specified by \a flags.
*/
void EditorManager::activateEditorForEntry(DocumentModel::Entry *entry, OpenEditorFlags flags)
{
    EditorManagerPrivate::activateEditorForEntry(EditorManagerPrivate::currentEditorView(),
                                                 entry, flags);
}

/*!
    Activates the editor \a editor using the editor settings specified by
    \a flags.
*/
void EditorManager::activateEditor(IEditor *editor, OpenEditorFlags flags)
{
    QTC_ASSERT(editor, return);
    EditorView *view = EditorManagerPrivate::viewForEditor(editor);
    // an IEditor doesn't have to belong to a view, it might be kept in storage by the editor model
    if (!view)
        view = EditorManagerPrivate::currentEditorView();
    EditorManagerPrivate::activateEditor(view, editor, flags);
}

/*!
    Activates the editor for the document specified by \a document in the
    current view using the editor settings specified by \a flags.
*/
IEditor *EditorManager::activateEditorForDocument(IDocument *document, OpenEditorFlags flags)
{
    return EditorManagerPrivate::activateEditorForDocument(EditorManagerPrivate::currentEditorView(), document, flags);
}

/*!
    Opens the document specified by \a fileName using the editor type specified
    by \a editorId and the settings specified by \a flags.

    If \a newEditor is not \c nullptr, and a new editor instance was created,
    it is set to \c true. If an existing editor instance was used, it is set
    to \c false.
*/
IEditor *EditorManager::openEditor(const QString &fileName, Id editorId,
                                   OpenEditorFlags flags, bool *newEditor)
{
    if (flags & EditorManager::OpenInOtherSplit)
        EditorManager::gotoOtherSplit();

    return EditorManagerPrivate::openEditor(EditorManagerPrivate::currentEditorView(),
                                            fileName, editorId, flags, newEditor);
}

/*!
    Opens the document specified by \a fileName to \a line and \a column using
    the editor type specified by \a editorId and the settings specified by
    \a flags.

    If \a newEditor is not \c nullptr, and a new editor instance was created,
    it is set to \c true. If an existing editor instance was used, it is set
    to \c false.
*/
IEditor *EditorManager::openEditorAt(const QString &fileName, int line, int column,
                                     Id editorId, OpenEditorFlags flags, bool *newEditor)
{
    if (flags & EditorManager::OpenInOtherSplit)
        EditorManager::gotoOtherSplit();

    return EditorManagerPrivate::openEditorAt(EditorManagerPrivate::currentEditorView(),
                                              fileName, line, column, editorId, flags, newEditor);
}

/*!
    Opens the document at the position of the search hit \a item in the editor
    using the settings specified by \a flags.
*/
void EditorManager::openEditorAtSearchResult(const SearchResultItem &item, OpenEditorFlags flags)
{
    if (item.path.empty()) {
        openEditor(QDir::fromNativeSeparators(item.text), Id(), flags);
        return;
    }

    openEditorAt(QDir::fromNativeSeparators(item.path.first()), item.mainRange.begin.line,
                 item.mainRange.begin.column, Id(), flags);
}

/*!
    Returns the file path \a fullFilePath with appended line and column
    information split into file path, line, and column components.

    The following patterns are supported: \c {filepath.txt:19},
    \c{filepath.txt:19:12}, \c {filepath.txt+19},
    \c {filepath.txt+19+12}, and \c {filepath.txt(19)}.
*/
EditorManager::FilePathInfo EditorManager::splitLineAndColumnNumber(const QString &fullFilePath)
{
    // :10:2 GCC/Clang-style
    static const auto regexp = QRegularExpression("[:+](\\d+)?([:+](\\d+)?)?$");
    // (10) MSVC-style
    static const auto vsRegexp = QRegularExpression("[(]((\\d+)[)]?)?$");
    const QRegularExpressionMatch match = regexp.match(fullFilePath);
    QString postfix;
    QString filePath = fullFilePath;
    int line = -1;
    int column = -1;
    if (match.hasMatch()) {
        postfix = match.captured(0);
        filePath = fullFilePath.left(match.capturedStart(0));
        line = 0; // for the case that there's only a : at the end
        if (match.lastCapturedIndex() > 0) {
            line = match.captured(1).toInt();
            if (match.lastCapturedIndex() > 2) // index 2 includes the + or : for the column number
                column = match.captured(3).toInt() - 1; //column is 0 based, despite line being 1 based
        }
    } else {
        const QRegularExpressionMatch vsMatch = vsRegexp.match(fullFilePath);
        postfix = vsMatch.captured(0);
        filePath = fullFilePath.left(vsMatch.capturedStart(0));
        if (vsMatch.lastCapturedIndex() > 1) // index 1 includes closing )
            line = vsMatch.captured(2).toInt();
    }
    return {filePath, postfix, line, column};
}

/*!
    Returns whether \a fileName is an auto-save file created by \QC.
*/
bool EditorManager::isAutoSaveFile(const QString &fileName)
{
    return fileName.endsWith(".autosave");
}

/*!
    Opens the document specified by \a fileName in the external editor specified
    by \a editorId.

    If \a editorId is not the ID of an external editor or the external editor
    cannot be opened, returns \c false and displays an error message.
*/
bool EditorManager::openExternalEditor(const QString &fileName, Id editorId)
{
    IExternalEditor *ee = Utils::findOrDefault(IExternalEditor::allExternalEditors(),
                                               Utils::equal(&IExternalEditor::id, editorId));
    if (!ee)
        return false;
    QString errorMessage;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    const bool ok = ee->startEditor(fileName, &errorMessage);
    QApplication::restoreOverrideCursor();
    if (!ok)
        QMessageBox::critical(ICore::dialogParent(), tr("Opening File"), errorMessage);
    return ok;
}

/*!
    Provides a hook for plugins to veto on closing editors.

    When an editor requests a close, all listeners are called. If one of the
    \a listener calls returns \c false, the process is aborted and the event is ignored.
    If all calls return \c true, editorAboutToClose() is emitted and the event
    is accepted.
*/
void EditorManager::addCloseEditorListener(const std::function<bool (IEditor *)> &listener)
{
    d->m_closeEditorListeners.append(listener);
}

/*!
    Asks the user for a list of files to open and returns the choice.

    \sa QFileDialog::getOpenFileNames()
*/
QStringList EditorManager::getOpenFileNames()
{
    QString selectedFilter;
    const QString &fileFilters = DocumentManager::allDocumentFactoryFiltersString(&selectedFilter);
    return DocumentManager::getOpenFileNames(fileFilters, QString(), &selectedFilter);
}

static QString makeTitleUnique(QString *titlePattern)
{
    QString title;
    if (titlePattern) {
        const QChar dollar = QLatin1Char('$');

        QString base = *titlePattern;
        if (base.isEmpty())
            base = "unnamed$";
        if (base.contains(dollar)) {
            int i = 1;
            QSet<QString> docnames;
            foreach (DocumentModel::Entry *entry, DocumentModel::entries()) {
                QString name = entry->fileName().toString();
                if (name.isEmpty())
                    name = entry->displayName();
                else
                    name = QFileInfo(name).completeBaseName();
                docnames << name;
            }

            do {
                title = base;
                title.replace(QString(dollar), QString::number(i++));
            } while (docnames.contains(title));
        } else {
            title = *titlePattern;
        }
        *titlePattern = title;
    }
    return title;
}

/*!
    Opens \a contents in an editor of the type \a editorId using the settings
    specified by \a flags.

    The editor is given a display name based on \a titlePattern. If a
    non-empty \a uniqueId is specified and an editor with that unique
     ID is found, it is re-used. Otherwise, a new editor with that
     unique ID is created.

    Returns the new or re-used editor.
*/
IEditor *EditorManager::openEditorWithContents(Id editorId,
                                        QString *titlePattern,
                                        const QByteArray &contents,
                                        const QString &uniqueId,
                                        OpenEditorFlags flags)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << editorId.name() << titlePattern << uniqueId << contents;

    if (flags & EditorManager::OpenInOtherSplit)
            EditorManager::gotoOtherSplit();

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    Utils::ExecuteOnDestruction appRestoreCursor(&QApplication::restoreOverrideCursor);
    Q_UNUSED(appRestoreCursor)


    const QString title = makeTitleUnique(titlePattern);

    IEditor *edt = nullptr;
    if (!uniqueId.isEmpty()) {
        foreach (IDocument *document, DocumentModel::openedDocuments())
            if (document->property(scratchBufferKey).toString() == uniqueId) {
                edt = DocumentModel::editorsForDocument(document).constFirst();

                document->setContents(contents);
                if (!title.isEmpty())
                    edt->document()->setPreferredDisplayName(title);

                activateEditor(edt, flags);
                return edt;
            }
    }

    EditorFactoryList factories = EditorManagerPrivate::findFactories(editorId, title);
    if (factories.isEmpty())
        return nullptr;

    edt = EditorManagerPrivate::createEditor(factories.first(), title);
    if (!edt)
        return nullptr;
    if (!edt->document()->setContents(contents)) {
        delete edt;
        edt = nullptr;
        return nullptr;
    }

    if (!uniqueId.isEmpty())
        edt->document()->setProperty(scratchBufferKey, uniqueId);

    if (!title.isEmpty())
        edt->document()->setPreferredDisplayName(title);

    EditorManagerPrivate::addEditor(edt);
    activateEditor(edt, flags);
    return edt;
}

/*!
    Returns whether the document specified by \a filePath should be opened even
    though it is big. Depending on the settings, might ask the user to decide
    whether the file should be opened.

    Returns the path to the file that was skipped.
*/
bool EditorManager::skipOpeningBigTextFile(const QString &filePath)
{
    return EditorManagerPrivate::skipOpeningBigTextFile(filePath);
}

/*!
    Clears the unique ID of \a document.

    \sa openEditorWithContents()
*/
void EditorManager::clearUniqueId(IDocument *document)
{
    document->setProperty(scratchBufferKey, QVariant());
}

/*!
    Saves the changes in \a document.

    Returns whether the operation was successful.
*/
bool EditorManager::saveDocument(IDocument *document)
{
    return EditorManagerPrivate::saveDocument(document);
}

/*!
    \internal
*/
bool EditorManager::hasSplitter()
{
    EditorView *view = EditorManagerPrivate::currentEditorView();
    QTC_ASSERT(view, return false);
    EditorArea *area = EditorManagerPrivate::findEditorArea(view);
    QTC_ASSERT(area, return false);
    return area->isSplitter();
}

/*!
    Returns the list of visible editors.
*/
QList<IEditor*> EditorManager::visibleEditors()
{
    QList<IEditor *> editors;
    foreach (EditorArea *area, d->m_editorAreas) {
        if (area->isSplitter()) {
            EditorView *firstView = area->findFirstView();
            EditorView *view = firstView;
            if (view) {
                do {
                    if (view->currentEditor())
                        editors.append(view->currentEditor());
                    view = view->findNextView();
                    QTC_ASSERT(view != firstView, break); // we start with firstView and shouldn't have cycles
                } while (view);
            }
        } else {
            if (area->editor())
                editors.append(area->editor());
        }
    }
    return editors;
}

/*!
    Closes \a document. If \a askAboutModifiedEditors is \c true, prompts
    users to save their changes before closing the document.

    Returns whether the document was closed.
*/
bool EditorManager::closeDocument(IDocument *document, bool askAboutModifiedEditors)
{
    return closeDocuments({document}, askAboutModifiedEditors);
}

/*!
    Closes \a documents. If \a askAboutModifiedEditors is \c true, prompts
    users to save their changes before closing the documents.

    Returns whether the documents were closed.
*/
bool EditorManager::closeDocuments(const QList<IDocument *> &documents, bool askAboutModifiedEditors)
{
    return m_instance->closeEditors(DocumentModel::editorsForDocuments(documents), askAboutModifiedEditors);
}

/*!
    Adds the current cursor position to the navigation history specified by
    \a saveState.
*/
void EditorManager::addCurrentPositionToNavigationHistory(const QByteArray &saveState)
{
    EditorManagerPrivate::currentEditorView()->addCurrentPositionToNavigationHistory(saveState);
    EditorManagerPrivate::updateActions();
}

/*!
    Sets the location that was last modified to \a editor.
*/
void EditorManager::setLastEditLocation(const IEditor* editor)
{
    IDocument *document = editor->document();
    if (!document)
        return;

    const QByteArray &state = editor->saveState();
    EditLocation location;
    location.document = document;
    location.fileName = document->filePath().toString();
    location.id = document->id();
    location.state = QVariant(state);

    d->m_globalLastEditLocation = location;
}

/*!
    Cuts forward in the navigation history.
*/
void EditorManager::cutForwardNavigationHistory()
{
    EditorManagerPrivate::currentEditorView()->cutForwardNavigationHistory();
    EditorManagerPrivate::updateActions();
}

/*!
    Goes back in the navigation history.
*/
void EditorManager::goBackInNavigationHistory()
{
    EditorManagerPrivate::currentEditorView()->goBackInNavigationHistory();
    EditorManagerPrivate::updateActions();
    return;
}

/*!
    Goes forward in the navigation history.
*/
void EditorManager::goForwardInNavigationHistory()
{
    EditorManagerPrivate::currentEditorView()->goForwardInNavigationHistory();
    EditorManagerPrivate::updateActions();
}

EditorWindow *windowForEditorArea(EditorArea *area)
{
    return qobject_cast<EditorWindow *>(area->window());
}

QVector<EditorWindow *> editorWindows(const QList<EditorArea *> &areas)
{
    QVector<EditorWindow *> result;
    for (EditorArea *area : areas)
        if (EditorWindow *window = windowForEditorArea(area))
            result.append(window);
    return result;
}

/*!
    Returns the serialized state of all non-temporary editors.
*/
QByteArray EditorManager::saveState()
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);

    stream << QByteArray("EditorManagerV5");

    // TODO: In case of split views it's not possible to restore these for all correctly with this
    QList<IDocument *> documents = DocumentModel::openedDocuments();
    foreach (IDocument *document, documents) {
        if (!document->filePath().isEmpty() && !document->isTemporary()) {
            IEditor *editor = DocumentModel::editorsForDocument(document).constFirst();
            QByteArray state = editor->saveState();
            if (!state.isEmpty())
                d->m_editorStates.insert(document->filePath().toString(), QVariant(state));
        }
    }

    stream << d->m_editorStates;

    QList<DocumentModel::Entry *> entries = DocumentModel::entries();
    int entriesCount = 0;
    foreach (DocumentModel::Entry *entry, entries) {
        // The editor may be 0 if it was not loaded yet: In that case it is not temporary
        if (!entry->document->isTemporary())
            ++entriesCount;
    }

    stream << entriesCount;

    foreach (DocumentModel::Entry *entry, entries) {
        if (!entry->document->isTemporary()) {
            stream << entry->fileName().toString() << entry->plainDisplayName() << entry->id()
                   << entry->pinned;
        }
    }

    stream << d->m_editorAreas.first()->saveState(); // TODO

    // windows
    const QVector<EditorWindow *> windows = editorWindows(d->m_editorAreas);
    const QVector<QVariantHash> windowStates = Utils::transform(windows, &EditorWindow::saveState);
    stream << windowStates;
    return bytes;
}

/*!
    Restores the editor to \a state.

    Returns \c true if the editor state can be restored.
*/
bool EditorManager::restoreState(const QByteArray &state)
{
    closeAllEditors(true);
    // remove extra windows
    for (int i = d->m_editorAreas.count() - 1; i > 0 /* keep first alive */; --i)
        delete d->m_editorAreas.at(i); // automatically removes it from list
    if (d->m_editorAreas.first()->isSplitter())
        EditorManagerPrivate::removeAllSplits();
    QDataStream stream(state);

    QByteArray version;
    stream >> version;

    const bool isVersion5 = version == "EditorManagerV5";
    if (version != "EditorManagerV4" && !isVersion5)
        return false;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    stream >> d->m_editorStates;

    int editorCount = 0;
    stream >> editorCount;
    while (--editorCount >= 0) {
        QString fileName;
        stream >> fileName;
        QString displayName;
        stream >> displayName;
        Id id;
        stream >> id;
        bool pinned = false;
        if (isVersion5)
            stream >> pinned;

        if (!fileName.isEmpty() && !displayName.isEmpty()) {
            QFileInfo fi(fileName);
            if (!fi.exists())
                continue;
            QFileInfo rfi(autoSaveName(fileName));
            if (rfi.exists() && fi.lastModified() < rfi.lastModified()) {
                if (IEditor *editor = openEditor(fileName, id, DoNotMakeVisible))
                    DocumentModelPrivate::setPinned(DocumentModel::entryForDocument(editor->document()), pinned);
            } else {
                 if (DocumentModel::Entry *entry = DocumentModelPrivate::addSuspendedDocument(fileName, displayName, id))
                     DocumentModelPrivate::setPinned(entry, pinned);
            }
        }
    }

    QByteArray splitterstates;
    stream >> splitterstates;
    d->m_editorAreas.first()->restoreState(splitterstates); // TODO

    if (!stream.atEnd()) { // safety for settings from Qt Creator 4.5 and earlier
        // restore windows
        QVector<QVariantHash> windowStates;
        stream >> windowStates;
        for (const QVariantHash &windowState : windowStates) {
            EditorWindow *window = d->createEditorWindow();
            window->restoreState(windowState);
            window->show();
        }
    }

    // splitting and stuff results in focus trouble, that's why we set the focus again after restoration
    if (d->m_currentEditor) {
        d->m_currentEditor->widget()->setFocus();
    } else if (Internal::EditorView *view = EditorManagerPrivate::currentEditorView()) {
        if (IEditor *e = view->currentEditor())
            e->widget()->setFocus();
        else
            view->setFocus();
    }

    QApplication::restoreOverrideCursor();

    return true;
}

/*!
    Shows the editor status bar \a object with the \a id, \a infoText,
    \a buttonText, and \a function.
*/
void EditorManager::showEditorStatusBar(const QString &id,
                                        const QString &infoText,
                                        const QString &buttonText,
                                        QObject *object,
                                        const std::function<void()> &function)
{

    EditorManagerPrivate::currentEditorView()->showEditorStatusBar(
                id, infoText, buttonText, object, function);
}

/*!
    Hides the editor status bar specified by \a id.
*/
void EditorManager::hideEditorStatusBar(const QString &id)
{
    // TODO: what if the current editor view betwenn show and hideEditorStatusBar changed?
    EditorManagerPrivate::currentEditorView()->hideEditorStatusBar(id);
}

/*!
    Returns the default text codec for the locale.
*/
QTextCodec *EditorManager::defaultTextCodec()
{
    QSettings *settings = ICore::settings();
    const QByteArray codecName =
            settings->value(Constants::SETTINGS_DEFAULTTEXTENCODING).toByteArray();
    if (QTextCodec *candidate = QTextCodec::codecForName(codecName))
        return candidate;
    // Qt5 doesn't return a valid codec when looking up the "System" codec, but will return
    // such a codec when asking for the codec for locale and no matching codec is available.
    // So check whether such a codec was saved to the settings.
    QTextCodec *localeCodec = QTextCodec::codecForLocale();
    if (codecName == localeCodec->name())
        return localeCodec;
    if (QTextCodec *defaultUTF8 = QTextCodec::codecForName("UTF-8"))
        return defaultUTF8;
    return QTextCodec::codecForLocale();
}

/*!
    Returns the default line ending as the user specified in the settings.
*/
TextFileFormat::LineTerminationMode EditorManager::defaultLineEnding()
{
    QSettings *settings = ICore::settings();
    const int defaultLineTerminator = settings->value(Constants::SETTINGS_DEFAULT_LINE_TERMINATOR,
            TextFileFormat::LineTerminationMode::NativeLineTerminator).toInt();

    return static_cast<TextFileFormat::LineTerminationMode>(defaultLineTerminator);
}

/*!
    Splits the editor view horizontally into adjacent views.
*/
void EditorManager::splitSideBySide()
{
    EditorManagerPrivate::split(Qt::Horizontal);
}

/*!
 * Moves focus to another split, creating it if necessary.
 * If there's no split and no other window, a side-by-side split is created.
 * If the current window is split, focus is moved to the next split within this window, cycling.
 * If the current window is not split, focus is moved to the next window.
 */
void EditorManager::gotoOtherSplit()
{
    EditorView *view = EditorManagerPrivate::currentEditorView();
    if (!view)
        return;
    EditorView *nextView = view->findNextView();
    if (!nextView) {
        // we are in the "last" view in this editor area
        int index = -1;
        EditorArea *area = EditorManagerPrivate::findEditorArea(view, &index);
        QTC_ASSERT(area, return);
        QTC_ASSERT(index >= 0 && index < d->m_editorAreas.size(), return);
        // stay in same window if it is split
        if (area->isSplitter()) {
            nextView = area->findFirstView();
            QTC_CHECK(nextView != view);
        } else {
            // find next editor area. this might be the same editor area if there's only one.
            int nextIndex = index + 1;
            if (nextIndex >= d->m_editorAreas.size())
                nextIndex = 0;
            nextView = d->m_editorAreas.at(nextIndex)->findFirstView();
            QTC_CHECK(nextView);
            // if we had only one editor area with only one view, we end up at the startpoint
            // in that case we need to split
            if (nextView == view) {
                QTC_CHECK(!area->isSplitter());
                splitSideBySide(); // that deletes 'view'
                view = area->findFirstView();
                nextView = view->findNextView();
                QTC_CHECK(nextView != view);
                QTC_CHECK(nextView);
            }
        }
    }

    if (nextView)
        EditorManagerPrivate::activateView(nextView);
}

/*!
    Returns the maximum file size.
*/
qint64 EditorManager::maxTextFileSize()
{
    return qint64(3) << 24;
}

/*!
    Sets the window title addition handler to \a handler.
*/
void EditorManager::setWindowTitleAdditionHandler(WindowTitleHandler handler)
{
    d->m_titleAdditionHandler = handler;
}

/*!
    Sets the session title addition handler to \a handler.
*/
void EditorManager::setSessionTitleHandler(WindowTitleHandler handler)
{
    d->m_sessionTitleHandler = handler;
}

void EditorManager::updateWindowTitles()
{
    foreach (EditorArea *area, d->m_editorAreas)
        emit area->windowTitleNeedsUpdate();
}

void EditorManager::setWindowTitleVcsTopicHandler(WindowTitleHandler handler)
{
    d->m_titleVcsTopicHandler = handler;
}

#if defined(WITH_TESTS)

void CorePlugin::testSplitLineAndColumnNumber()
{
    QFETCH(QString, testFile);
    QFETCH(QString, filePath);
    QFETCH(QString, postfix);
    QFETCH(int, line);
    QFETCH(int, column);
    const EditorManager::FilePathInfo fp = EditorManager::splitLineAndColumnNumber(testFile);
    QCOMPARE(fp.filePath, filePath);
    QCOMPARE(fp.postfix, postfix);
    QCOMPARE(fp.lineNumber, line);
    QCOMPARE(fp.columnNumber, column);
}

void CorePlugin::testSplitLineAndColumnNumber_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("postfix");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("column");

    QTest::newRow("no-line-no-column") << QString::fromLatin1("someFile.txt")
                                       << QString::fromLatin1("someFile.txt")
                                       << QString() << -1 << -1;
    QTest::newRow(": at end") << QString::fromLatin1("someFile.txt:")
                                       << QString::fromLatin1("someFile.txt")
                                       << QString::fromLatin1(":") << 0 << -1;
    QTest::newRow("+ at end") << QString::fromLatin1("someFile.txt+")
                                       << QString::fromLatin1("someFile.txt")
                                       << QString::fromLatin1("+") << 0 << -1;
    QTest::newRow(": for column") << QString::fromLatin1("someFile.txt:10:")
                                       << QString::fromLatin1("someFile.txt")
                                       << QString::fromLatin1(":10:") << 10 << -1;
    QTest::newRow("+ for column") << QString::fromLatin1("someFile.txt:10+")
                                       << QString::fromLatin1("someFile.txt")
                                       << QString::fromLatin1(":10+") << 10 << -1;
    QTest::newRow(": and + at end") << QString::fromLatin1("someFile.txt:+")
                                       << QString::fromLatin1("someFile.txt")
                                       << QString::fromLatin1(":+") << 0 << -1;
    QTest::newRow("empty line") << QString::fromLatin1("someFile.txt:+10")
                                       << QString::fromLatin1("someFile.txt")
                                       << QString::fromLatin1(":+10") << 0 << 9;
    QTest::newRow(":line-no-column") << QString::fromLatin1("/some/path/file.txt:42")
                                     << QString::fromLatin1("/some/path/file.txt")
                                     << QString::fromLatin1(":42") << 42 << -1;
    QTest::newRow("+line-no-column") << QString::fromLatin1("/some/path/file.txt+42")
                                     << QString::fromLatin1("/some/path/file.txt")
                                     << QString::fromLatin1("+42") << 42 << -1;
    QTest::newRow(":line-:column") << QString::fromLatin1("/some/path/file.txt:42:3")
                                     << QString::fromLatin1("/some/path/file.txt")
                                     << QString::fromLatin1(":42:3") << 42 << 2;
    QTest::newRow(":line-+column") << QString::fromLatin1("/some/path/file.txt:42+33")
                                     << QString::fromLatin1("/some/path/file.txt")
                                     << QString::fromLatin1(":42+33") << 42 << 32;
    QTest::newRow("+line-:column") << QString::fromLatin1("/some/path/file.txt+142:30")
                                     << QString::fromLatin1("/some/path/file.txt")
                                     << QString::fromLatin1("+142:30") << 142 << 29;
    QTest::newRow("+line-+column") << QString::fromLatin1("/some/path/file.txt+142+33")
                                     << QString::fromLatin1("/some/path/file.txt")
                                     << QString::fromLatin1("+142+33") << 142 << 32;
    QTest::newRow("( at end") << QString::fromLatin1("/some/path/file.txt(")
                              << QString::fromLatin1("/some/path/file.txt")
                              << QString::fromLatin1("(") << -1 << -1;
    QTest::newRow("(42 at end") << QString::fromLatin1("/some/path/file.txt(42")
                              << QString::fromLatin1("/some/path/file.txt")
                              << QString::fromLatin1("(42") << 42 << -1;
    QTest::newRow("(42) at end") << QString::fromLatin1("/some/path/file.txt(42)")
                              << QString::fromLatin1("/some/path/file.txt")
                              << QString::fromLatin1("(42)") << 42 << -1;
}

#endif // WITH_TESTS
