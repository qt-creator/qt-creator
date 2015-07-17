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

#include "editormanager.h"
#include "editormanager_p.h"
#include "editorwindow.h"

#include "editorview.h"
#include "openeditorswindow.h"
#include "openeditorsview.h"
#include "documentmodel.h"
#include "ieditor.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/openwithdialog.h>
#include <coreplugin/dialogs/readonlyfilesdialog.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/iexternaleditor.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/imode.h>
#include <coreplugin/infobar.h>
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
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/mimetypes/mimetype.h>
#include <utils/qtcassert.h>
#include <utils/overridecursor.h>

#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QMap>
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
static const char warnBeforeOpeningBigTextFilesKey[] = "EditorManager/WarnBeforeOpeningBigTextFiles";
static const char bigTextFileSizeLimitKey[] = "EditorManager/BigTextFileSizeLimitInMB";

static const char scratchBufferKey[] = "_q_emScratchBuffer";

//===================EditorClosingCoreListener======================

namespace Core {
namespace Internal {

class EditorClosingCoreListener : public ICoreListener
{
public:
    bool editorAboutToClose(IEditor *) { return true; }
    bool coreAboutToClose()
    {
        // Do not ask for files to save.
        // MainWindow::closeEvent has already done that.
        return EditorManager::closeAllEditors(false);
    }
};

} // namespace Internal
} // namespace Core

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

//===================EditorManager=====================

EditorManagerPlaceHolder::EditorManagerPlaceHolder(IMode *mode, QWidget *parent)
    : QWidget(parent), m_mode(mode)
{
    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    setFocusProxy(EditorManagerPrivate::mainEditorArea());
    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &EditorManagerPlaceHolder::currentModeChanged);

    currentModeChanged(ModeManager::currentMode());
}

EditorManagerPlaceHolder::~EditorManagerPlaceHolder()
{
    // EditorManager will be deleted in ~MainWindow()
    QWidget *em = EditorManagerPrivate::mainEditorArea();
    if (em && em->parent() == this) {
        em->hide();
        em->setParent(0);
    }
}

void EditorManagerPlaceHolder::currentModeChanged(IMode *mode)
{
    if (m_mode == mode) {
        QWidget *previousFocus = 0;
        QWidget *em = EditorManagerPrivate::mainEditorArea();
        if (em->focusWidget() && em->focusWidget()->hasFocus())
            previousFocus = em->focusWidget();
        layout()->addWidget(em);
        em->show();
        if (previousFocus)
            previousFocus->setFocus();
    }
}

// ---------------- EditorManager

static EditorManager *m_instance = 0;
static EditorManagerPrivate *d;

static int extractNumericSuffix(QString *fileName, QString *postfix = 0)
{
    int i = fileName->length() - 1;
    for (; i >= 0; --i) {
        if (!fileName->at(i).isNumber())
            break;
    }
    if (i == -1)
        return -1;
    const QChar c = fileName->at(i);
    if (c == QLatin1Char(':') || c == QLatin1Char('+')) {
        bool ok;
        const QString suffix = fileName->mid(i + 1);
        const int result = suffix.toInt(&ok);
        if (suffix.isEmpty() || ok) {
            if (postfix)
                *postfix = fileName->mid(i);
            fileName->truncate(i);
            return result;
        }
    }
    return -1;
}

static void extractLineAndColumnNumbers(QString *fileName, int *lineNumber, int *columnNumber)
{
    *lineNumber = -1;
    *columnNumber = -1;
    int lastSuffix = extractNumericSuffix(fileName);
    if (lastSuffix == -1)
        return;
    int secondToLastSuffix = extractNumericSuffix(fileName);
    if (secondToLastSuffix == -1) {
        *lineNumber = lastSuffix;
        return;
    }
    *lineNumber = secondToLastSuffix;
    *columnNumber = lastSuffix - 1; //column is 0 based, despite line being 1 based
}

static QString autoSaveName(const QString &fileName)
{
    return fileName + QLatin1String(".autosave");
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
            holder->unmaximize();
    }
}

/* For something that has a 'QString id' (IEditorFactory
 * or IExternalEditor), find the one matching a id. */
template <class EditorFactoryLike>
EditorFactoryLike *findById(Id id)
{
    return ExtensionSystem::PluginManager::getObject<EditorFactoryLike>(
        [&id](EditorFactoryLike *efl) {
            return id == efl->id();
        });
}

EditorManagerPrivate::EditorManagerPrivate(QObject *parent) :
    QObject(parent),
    m_autoSaveTimer(0),
    m_revertToSavedAction(new QAction(EditorManager::tr("Revert to Saved"), this)),
    m_saveAction(new QAction(this)),
    m_saveAsAction(new QAction(this)),
    m_closeCurrentEditorAction(new QAction(EditorManager::tr("Close"), this)),
    m_closeAllEditorsAction(new QAction(EditorManager::tr("Close All"), this)),
    m_closeOtherDocumentsAction(new QAction(EditorManager::tr("Close Others"), this)),
    m_closeAllEditorsExceptVisibleAction(new QAction(EditorManager::tr("Close All Except Visible"), this)),
    m_gotoNextDocHistoryAction(new QAction(EditorManager::tr("Next Open Document in History"), this)),
    m_gotoPreviousDocHistoryAction(new QAction(EditorManager::tr("Previous Open Document in History"), this)),
    m_goBackAction(new QAction(QIcon(QLatin1String(Constants::ICON_PREV)), EditorManager::tr("Go Back"), this)),
    m_goForwardAction(new QAction(QIcon(QLatin1String(Constants::ICON_NEXT)), EditorManager::tr("Go Forward"), this)),
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
    m_openTerminalAction(new QAction(FileUtils::msgTerminalAction(), this)),
    m_findInDirectoryAction(new QAction(FileUtils::msgFindInDirectory(), this)),
    m_windowPopup(0),
    m_coreListener(0),
    m_reloadSetting(IDocument::AlwaysAsk),
    m_autoSaveEnabled(true),
    m_autoSaveInterval(5),
    m_warnBeforeOpeningBigFilesEnabled(true),
    m_bigFileSizeLimitInMB(5)
{
    d = this;
}

EditorManagerPrivate::~EditorManagerPrivate()
{
    if (ICore::instance()) {
        if (m_coreListener) {
            ExtensionSystem::PluginManager::removeObject(m_coreListener);
            delete m_coreListener;
        }
        ExtensionSystem::PluginManager::removeObject(m_openEditorsFactory);
        delete m_openEditorsFactory;
    }

    // close all extra windows
    for (int i = 0; i < m_editorAreas.size(); ++i) {
        EditorArea *area = m_editorAreas.at(i);
        disconnect(area, SIGNAL(destroyed(QObject*)), this, SLOT(editorAreaDestroyed(QObject*)));
        delete area;
    }
    m_editorAreas.clear();

    DocumentModel::destroy();
    d = 0;
}

void EditorManagerPrivate::init()
{
    DocumentModel::init();
    connect(ICore::instance(), &ICore::contextAboutToChange,
            this, &EditorManagerPrivate::handleContextChange);

    const Context editManagerContext(Constants::C_EDITORMANAGER);
    // combined context for edit & design modes
    const Context editDesignContext(Constants::C_EDITORMANAGER, Constants::C_DESIGN_MODE);

    ActionContainer *mfile = ActionManager::actionContainer(Constants::M_FILE);

    // Revert to saved
    m_revertToSavedAction->setIcon(QIcon::fromTheme(QLatin1String("document-revert")));
    Command *cmd = ActionManager::registerAction(m_revertToSavedAction,
                                       Constants::REVERTTOSAVED, editManagerContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Revert File to Saved"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_revertToSavedAction, SIGNAL(triggered()), m_instance, SLOT(revertToSaved()));

    // Save Action
    ActionManager::registerAction(m_saveAction, Constants::SAVE, editManagerContext);
    connect(m_saveAction, SIGNAL(triggered()), m_instance, SLOT(saveDocument()));

    // Save As Action
    ActionManager::registerAction(m_saveAsAction, Constants::SAVEAS, editManagerContext);
    connect(m_saveAsAction, SIGNAL(triggered()), m_instance, SLOT(saveDocumentAs()));

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
    connect(m_closeCurrentEditorAction, SIGNAL(triggered()),
            m_instance, SLOT(slotCloseCurrentEditorOrDocument()));

    if (HostOsInfo::isWindowsHost()) {
        // workaround for QTCREATORBUG-72
        QAction *action = new QAction(tr("Alternative Close"), this);
        cmd = ActionManager::registerAction(action, Constants::CLOSE_ALTERNATIVE, editManagerContext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+F4")));
        cmd->setDescription(EditorManager::tr("Close"));
        connect(action, SIGNAL(triggered()), m_instance, SLOT(slotCloseCurrentEditorOrDocument()));
    }

    // Close All Action
    cmd = ActionManager::registerAction(m_closeAllEditorsAction, Constants::CLOSEALL, editManagerContext, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+W")));
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(m_closeAllEditorsAction, SIGNAL(triggered()), m_instance, SLOT(closeAllEditors()));

    // Close All Others Action
    cmd = ActionManager::registerAction(m_closeOtherDocumentsAction, Constants::CLOSEOTHERS, editManagerContext, true);
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    cmd->setAttribute(Command::CA_UpdateText);
    connect(m_closeOtherDocumentsAction, SIGNAL(triggered()), m_instance, SLOT(closeOtherDocuments()));

    // Close All Others Except Visible Action
    cmd = ActionManager::registerAction(m_closeAllEditorsExceptVisibleAction, Constants::CLOSEALLEXCEPTVISIBLE, editManagerContext, true);
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(m_closeAllEditorsExceptVisibleAction, SIGNAL(triggered()),
            this, SLOT(closeAllEditorsExceptVisible()));

    //Save XXX Context Actions
    connect(m_copyFilePathContextAction, SIGNAL(triggered()),
            this, SLOT(copyFilePathFromContextMenu()));
    connect(m_copyLocationContextAction, SIGNAL(triggered()),
            this, SLOT(copyLocationFromContextMenu()));
    connect(m_copyFileNameContextAction, SIGNAL(triggered()),
            this, SLOT(copyFileNameFromContextMenu()));
    connect(m_saveCurrentEditorContextAction, SIGNAL(triggered()),
            this, SLOT(saveDocumentFromContextMenu()));
    connect(m_saveAsCurrentEditorContextAction, SIGNAL(triggered()),
            this, SLOT(saveDocumentAsFromContextMenu()));
    connect(m_revertToSavedCurrentEditorContextAction, SIGNAL(triggered()),
            this, SLOT(revertToSavedFromContextMenu()));

    // Close XXX Context Actions
    connect(m_closeAllEditorsContextAction, SIGNAL(triggered()),
            m_instance, SLOT(closeAllEditors()));
    connect(m_closeCurrentEditorContextAction, SIGNAL(triggered()),
            this, SLOT(closeEditorFromContextMenu()));
    connect(m_closeOtherDocumentsContextAction, SIGNAL(triggered()),
            this, SLOT(closeOtherDocumentsFromContextMenu()));
    connect(m_closeAllEditorsExceptVisibleContextAction, SIGNAL(triggered()),
            this, SLOT(closeAllEditorsExceptVisible()));

    connect(m_openGraphicalShellAction, SIGNAL(triggered()), this, SLOT(showInGraphicalShell()));
    connect(m_openTerminalAction, SIGNAL(triggered()), this, SLOT(openTerminal()));
    connect(m_findInDirectoryAction, SIGNAL(triggered()), this, SLOT(findInDirectory()));

    // Goto Previous In History Action
    cmd = ActionManager::registerAction(m_gotoPreviousDocHistoryAction, Constants::GOTOPREVINHISTORY, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Alt+Tab") : tr("Ctrl+Tab")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_gotoPreviousDocHistoryAction, SIGNAL(triggered()),
            this, SLOT(gotoPreviousDocHistory()));

    // Goto Next In History Action
    cmd = ActionManager::registerAction(m_gotoNextDocHistoryAction, Constants::GOTONEXTINHISTORY, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Alt+Shift+Tab") : tr("Ctrl+Shift+Tab")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_gotoNextDocHistoryAction, SIGNAL(triggered()), this, SLOT(gotoNextDocHistory()));

    // Go back in navigation history
    cmd = ActionManager::registerAction(m_goBackAction, Constants::GO_BACK, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Alt+Left") : tr("Alt+Left")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_goBackAction, SIGNAL(triggered()), m_instance, SLOT(goBackInNavigationHistory()));

    // Go forward in navigation history
    cmd = ActionManager::registerAction(m_goForwardAction, Constants::GO_FORWARD, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Alt+Right") : tr("Alt+Right")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_goForwardAction, SIGNAL(triggered()), m_instance, SLOT(goForwardInNavigationHistory()));

    m_splitAction = new QAction(tr("Split"), this);
    cmd = ActionManager::registerAction(m_splitAction, Constants::SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,2") : tr("Ctrl+E,2")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_splitAction, SIGNAL(triggered()), this, SLOT(split()));

    m_splitSideBySideAction = new QAction(tr("Split Side by Side"), this);
    cmd = ActionManager::registerAction(m_splitSideBySideAction, Constants::SPLIT_SIDE_BY_SIDE, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,3") : tr("Ctrl+E,3")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_splitSideBySideAction, SIGNAL(triggered()), m_instance, SLOT(splitSideBySide()));

    m_splitNewWindowAction = new QAction(tr("Open in New Window"), this);
    cmd = ActionManager::registerAction(m_splitNewWindowAction, Constants::SPLIT_NEW_WINDOW, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,4") : tr("Ctrl+E,4")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_splitNewWindowAction, SIGNAL(triggered()), this, SLOT(splitNewWindow()));

    m_removeCurrentSplitAction = new QAction(tr("Remove Current Split"), this);
    cmd = ActionManager::registerAction(m_removeCurrentSplitAction, Constants::REMOVE_CURRENT_SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,0") : tr("Ctrl+E,0")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_removeCurrentSplitAction, SIGNAL(triggered()), this, SLOT(removeCurrentSplit()));

    m_removeAllSplitsAction = new QAction(tr("Remove All Splits"), this);
    cmd = ActionManager::registerAction(m_removeAllSplitsAction, Constants::REMOVE_ALL_SPLITS, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,1") : tr("Ctrl+E,1")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_removeAllSplitsAction, SIGNAL(triggered()), this, SLOT(removeAllSplits()));

    m_gotoNextSplitAction = new QAction(tr("Go to Next Split or Window"), this);
    cmd = ActionManager::registerAction(m_gotoNextSplitAction, Constants::GOTO_NEXT_SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,o") : tr("Ctrl+E,o")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_gotoNextSplitAction, SIGNAL(triggered()), this, SLOT(gotoNextSplit()));

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
    mainEditorArea->hide();
    connect(mainEditorArea, &EditorArea::windowTitleNeedsUpdate,
            this, &EditorManagerPrivate::updateWindowTitle);
    connect(mainEditorArea, SIGNAL(destroyed(QObject*)), this, SLOT(editorAreaDestroyed(QObject*)));
    d->m_editorAreas.append(mainEditorArea);
    d->m_currentView = mainEditorArea->view();

    updateActions();

    // The popup needs a parent to get keyboard focus.
    m_windowPopup = new OpenEditorsWindow(mainEditorArea);
    m_windowPopup->hide();

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setObjectName(QLatin1String("EditorManager::m_autoSaveTimer"));
    connect(m_autoSaveTimer, SIGNAL(timeout()), SLOT(autoSave()));
    updateAutoSave();

    d->m_coreListener = new EditorClosingCoreListener();
    ExtensionSystem::PluginManager::addObject(d->m_coreListener);

    d->m_openEditorsFactory = new OpenEditorsViewFactory();
    ExtensionSystem::PluginManager::addObject(d->m_openEditorsFactory);

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

    Utils::MimeDatabase mdb;
    Utils::MimeType mimeType = mdb.mimeTypeForFile(filePath);
    if (!mimeType.inherits(QLatin1String("text/plain")))
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

        CheckableMessageBox messageBox(ICore::mainWindow());
        messageBox.setWindowTitle(title);
        messageBox.setText(text);
        messageBox.setStandardButtons(QDialogButtonBox::Yes|QDialogButtonBox::No);
        messageBox.setDefaultButton(QDialogButtonBox::No);
        messageBox.setIconPixmap(QMessageBox::standardIcon(QMessageBox::Question));
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
        extractLineAndColumnNumbers(&fn, &lineNumber, &columnNumber);
        if (lineNumber != -1)
            fi.setFile(fn);
    }

    if (fn.isEmpty())
        return 0;

    if (newEditor)
        *newEditor = false;

    const QList<IEditor *> editors = DocumentModel::editorsForFilePath(fn);
    if (!editors.isEmpty()) {
        IEditor *editor = editors.first();
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

    EditorManager::EditorFactoryList factories = EditorManagerPrivate::findFactories(Id(), fn);
    if (factories.isEmpty()) {
        Utils::MimeDatabase mdb;
        Utils::MimeType mimeType = mdb.mimeTypeForFile(fn);
        QMessageBox msgbox(QMessageBox::Critical, EditorManager::tr("File Error"),
                           tr("Could not open \"%1\": Cannot open files of type \"%2\".")
                           .arg(FileName::fromString(realFn).toUserOutput()).arg(mimeType.name()),
                           QMessageBox::Ok, ICore::dialogParent());
        msgbox.exec();
        return 0;
    }
    if (editorId.isValid()) {
        if (IEditorFactory *factory = findById<IEditorFactory>(editorId)) {
            factories.removeOne(factory);
            factories.push_front(factory);
        }
    }

    IEditor *editor = 0;
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
        editor = 0;

        if (openResult == IDocument::OpenResult::ReadError) {
            QMessageBox msgbox(QMessageBox::Critical, EditorManager::tr("File Error"),
                               tr("Could not open \"%1\" for reading. "
                                  "Either the file does not exist or you do not have "
                                  "the permissions to open it.")
                               .arg(FileName::fromString(realFn).toUserOutput()),
                               QMessageBox::Ok, ICore::dialogParent());
            msgbox.exec();
            return 0;
        }
        QTC_CHECK(openResult == IDocument::OpenResult::CannotHandle);

        if (errorString.isEmpty()) {
            errorString = tr("Could not open \"%1\": Unknown error.")
                    .arg(FileName::fromString(realFn).toUserOutput());
        }

        QMessageBox msgbox(QMessageBox::Critical, EditorManager::tr("File Error"), errorString, QMessageBox::Open | QMessageBox::Cancel, ICore::mainWindow());

        IEditorFactory *selectedFactory = 0;
        if (!factories.isEmpty()) {
            QPushButton *button = qobject_cast<QPushButton *>(msgbox.button(QMessageBox::Open));
            QTC_ASSERT(button, return 0);
            QMenu *menu = new QMenu(button);
            foreach (IEditorFactory *factory, factories) {
                QAction *action = menu->addAction(factory->displayName());
                connect(action, &QAction::triggered, [&selectedFactory, factory, &msgbox]() {
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
            return 0;

        overrideCursor.set();

        factories.removeOne(selectedFactory);
        factory = selectedFactory;
    }

    if (!editor)
        return 0;

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

IEditor *EditorManagerPrivate::openEditorWith(const QString &fileName, Core::Id editorId)
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
        return 0;

    IEditor *openedEditor = 0;
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
            return 0;
        editor = editors.first();
    }
    return activateEditor(view, editor, flags);
}

EditorView *EditorManagerPrivate::viewForEditor(IEditor *editor)
{
    QWidget *w = editor->widget();
    while (w) {
        w = w->parentWidget();
        if (EditorView *view = qobject_cast<EditorView *>(w))
            return view;
    }
    return 0;
}

MakeWritableResult EditorManagerPrivate::makeFileWritable(IDocument *document)
{
    if (!document)
        return Failed;
    // TODO: dialog parent is wrong
    ReadOnlyFilesDialog roDialog(document, ICore::mainWindow(), document->isSaveAsAllowed());
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

    QWidget *activeWindow = qApp->activeWindow();
    if (!activeWindow)
        return;
    QWidget *focus = qApp->focusWidget();
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
        QWidget *activeWindow = qApp->activeWindow();
        // decide where to show the popup
        // if the active window has editors, we want that editor area as a reference
        // TODO: this does not work correctly with multiple editor areas in the same window
        EditorArea *activeEditorArea = 0;
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
    Utils::MimeDatabase mdb;
    Utils::MimeType mt = mdb.mimeTypeForFile(fileName);
    //Unable to determine mime type of fileName. Falling back to text/plain",
    if (!mt.isValid())
        mt = mdb.mimeTypeForName(QLatin1String("text/plain"));
    QList<Id> allEditorIds;
    QStringList allEditorDisplayNames;
    QList<Id> externalEditorIds;
    // Built-in
    const EditorManager::EditorFactoryList editors = EditorManager::editorFactories(mt, false);
    const int size = editors.size();
    for (int i = 0; i < size; i++) {
        allEditorIds.push_back(editors.at(i)->id());
        allEditorDisplayNames.push_back(editors.at(i)->displayName());
    }
    // External editors
    const EditorManager::ExternalEditorList exEditors = EditorManager::externalEditors(mt, false);
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
    OpenWithDialog dialog(fileName, ICore::mainWindow());
    dialog.setEditors(allEditorDisplayNames);
    dialog.setCurrentEditor(0);
    if (dialog.exec() != QDialog::Accepted)
        return Id();
    const Id selectedId = allEditorIds.at(dialog.editor());
    if (isExternalEditor)
        *isExternalEditor = externalEditorIds.contains(selectedId);
    return selectedId;
}

void EditorManagerPrivate::saveSettings()
{
    SettingsDatabase *settings = ICore::settingsDatabase();
    settings->beginTransaction();
    settings->setValue(QLatin1String(documentStatesKey), d->m_editorStates);
    settings->setValue(QLatin1String(reloadBehaviorKey), d->m_reloadSetting);
    settings->setValue(QLatin1String(autoSaveEnabledKey), d->m_autoSaveEnabled);
    settings->setValue(QLatin1String(autoSaveIntervalKey), d->m_autoSaveInterval);
    settings->endTransaction();

    QSettings *qsettings = ICore::settings();
    qsettings->setValue(QLatin1String(warnBeforeOpeningBigTextFilesKey),
                        d->m_warnBeforeOpeningBigFilesEnabled);
    qsettings->setValue(QLatin1String(bigTextFileSizeLimitKey), d->m_bigFileSizeLimitInMB);
}

void EditorManagerPrivate::readSettings()
{
    // TODO remove
    // Backward compatibility to old locations for these settings
    QSettings *qs = ICore::settings();
    if (qs->contains(QLatin1String(documentStatesKey))) {
        d->m_editorStates = qs->value(QLatin1String(documentStatesKey))
            .value<QMap<QString, QVariant> >();
        qs->remove(QLatin1String(documentStatesKey));
    }

    if (qs->contains(QLatin1String(warnBeforeOpeningBigTextFilesKey))) {
        d->m_warnBeforeOpeningBigFilesEnabled
                = qs->value(QLatin1String(warnBeforeOpeningBigTextFilesKey)).toBool();
        d->m_bigFileSizeLimitInMB = qs->value(QLatin1String(bigTextFileSizeLimitKey)).toInt();
    }

    SettingsDatabase *settings = ICore::settingsDatabase();
    if (settings->contains(QLatin1String(documentStatesKey)))
        d->m_editorStates = settings->value(QLatin1String(documentStatesKey))
            .value<QMap<QString, QVariant> >();

    if (settings->contains(QLatin1String(reloadBehaviorKey)))
        d->m_reloadSetting = (IDocument::ReloadSetting)settings->value(QLatin1String(reloadBehaviorKey)).toInt();

    if (settings->contains(QLatin1String(autoSaveEnabledKey))) {
        d->m_autoSaveEnabled = settings->value(QLatin1String(autoSaveEnabledKey)).toBool();
        d->m_autoSaveInterval = settings->value(QLatin1String(autoSaveIntervalKey)).toInt();
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

void EditorManagerPrivate::setBigFileSizeLimit(int limitInMB)
{
    d->m_bigFileSizeLimitInMB = limitInMB;
}

EditorManager::EditorFactoryList EditorManagerPrivate::findFactories(Id editorId, const QString &fileName)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << editorId.name() << fileName;

    EditorManager::EditorFactoryList factories;
    if (!editorId.isValid()) {
        const QFileInfo fileInfo(fileName);
        // Find by mime type
        Utils::MimeDatabase mdb;
        Utils::MimeType mimeType = mdb.mimeTypeForFile(fileInfo);
        if (!mimeType.isValid()) {
            qWarning("%s unable to determine mime type of %s/%s. Falling back to text/plain",
                     Q_FUNC_INFO, fileName.toUtf8().constData(), editorId.name().constData());
            mimeType = mdb.mimeTypeForName(QLatin1String("text/plain"));
        }
        // open text files > 48 MB in binary editor
        if (fileInfo.size() > EditorManager::maxTextFileSize()
                && mimeType.name().startsWith(QLatin1String("text"))) {
            mimeType = mdb.mimeTypeForName(QLatin1String("application/octet-stream"));
        }
        factories = EditorManager::editorFactories(mimeType, false);
    } else {
        // Find by editor id
        if (IEditorFactory *factory = findById<IEditorFactory>(editorId))
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
        return 0;

    IEditor *editor = factory->createEditor();
    if (editor) {
        QTC_CHECK(editor->document()->id().isValid()); // sanity check that the editor has an id set
        connect(editor->document(), SIGNAL(changed()), d, SLOT(handleDocumentStateChange()));
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
    DocumentModel::addEditor(editor, &isNewDocument);
    if (isNewDocument) {
        const bool isTemporary = editor->document()->isTemporary();
        const bool addWatcher = !isTemporary;
        DocumentManager::addDocument(editor->document(), addWatcher);
        if (!isTemporary)
            DocumentManager::addToRecentFiles(editor->document()->filePath().toString(),
                                              editor->document()->id());
    }
    emit m_instance->editorOpened(editor);
}

void EditorManagerPrivate::removeEditor(IEditor *editor)
{
    bool lastOneForDocument = false;
    DocumentModel::removeEditor(editor, &lastOneForDocument);
    if (lastOneForDocument)
        DocumentManager::removeDocument(editor->document());
    ICore::removeContextObject(editor);
}

IEditor *EditorManagerPrivate::placeEditor(EditorView *view, IEditor *editor)
{
    Q_ASSERT(view && editor);

    if (view->hasEditor(editor))
        return editor;
    if (IEditor *e = view->editorForDocument(editor->document()))
        return e;

    if (EditorView *sourceView = viewForEditor(editor)) {
        // try duplication or pull editor over to new view
        bool duplicateSupported = editor->duplicateSupported();
        if (editor != sourceView->currentEditor() || !duplicateSupported) {
            // pull the IEditor over to the new view
            sourceView->removeEditor(editor);
            view->addEditor(editor);
            view->setCurrentEditor(editor);
            if (!sourceView->currentEditor()) {
                EditorView *replacementView = 0;
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
    return editor;
}

IEditor *EditorManagerPrivate::duplicateEditor(IEditor *editor)
{
    if (!editor->duplicateSupported())
        return 0;

    IEditor *duplicate = editor->duplicate();
    duplicate->restoreState(editor->saveState());
    emit m_instance->editorCreated(duplicate, duplicate->document()->filePath().toString());
    addEditor(duplicate);
    return duplicate;
}

IEditor *EditorManagerPrivate::activateEditor(EditorView *view, IEditor *editor,
                                              EditorManager::OpenEditorFlags flags)
{
    Q_ASSERT(view);

    if (!editor) {
        if (!d->m_currentEditor)
            setCurrentEditor(0, (flags & EditorManager::IgnoreNavigationHistory));
        return 0;
    }

    editor = placeEditor(view, editor);

    if (!(flags & EditorManager::DoNotChangeCurrentEditor)) {
        setCurrentEditor(editor, (flags & EditorManager::IgnoreNavigationHistory));
        if (!(flags & EditorManager::DoNotMakeVisible)) {
            // switch to design mode?
            if (!(flags & EditorManager::DoNotSwitchToDesignMode) && editor->isDesignModePreferred()) {
                ModeManager::activateMode(Constants::MODE_DESIGN);
                ModeManager::setFocusToCurrentMode();
            } else {
                int index;
                findEditorArea(view, &index);
                if (index == 0) // main window --> we might need to switch mode
                    if (!editor->widget()->isVisible())
                        ModeManager::activateMode(Constants::MODE_EDIT);
                editor->widget()->setFocus();
                ICore::raiseWindow(editor->widget());
            }
        }
    } else if (!(flags & EditorManager::DoNotMakeVisible)) {
        view->setCurrentEditor(editor);
    }
    return editor;
}

void EditorManagerPrivate::activateEditorForEntry(EditorView *view, DocumentModel::Entry *entry, EditorManager::OpenEditorFlags flags)
{
    QTC_ASSERT(view, return);
    if (!entry) { // no document
        view->setCurrentEditor(0);
        setCurrentView(view);
        setCurrentEditor(0);
        return;
    }
    IDocument *document = entry->document;
    if (!entry->isRestored)  {
        activateEditorForDocument(view, document, flags);
        return;
    }

    if (!openEditor(view, entry->fileName().toString(), entry->id(), flags))
        DocumentModel::removeEntry(entry);
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
        setCurrentView(0);

    if (d->m_currentEditor == editor)
        return;
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
        if (EditorArea *area = qobject_cast<EditorArea *>(current)) {
            int index = d->m_editorAreas.indexOf(area);
            QTC_ASSERT(index >= 0, return 0);
            if (areaIndex)
                *areaIndex = index;
            return area;
        }
        current = current->findParentSplitter();
    }
    QTC_CHECK(false); // we should never have views without a editor area
    return 0;
}

void EditorManagerPrivate::closeView(EditorView *view)
{
    if (!view)
        return;

    emptyView(view);

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
}

void EditorManagerPrivate::emptyView(EditorView *view)
{
    if (!view)
        return;

    QList<IEditor *> editors = view->editors();
    foreach (IEditor *editor, editors) {
        if (DocumentModel::editorsForDocument(editor->document()).size() == 1) {
            // it's the only editor for that file
            // so we need to keep it around (--> in the editor model)
            if (EditorManager::currentEditor() == editor) {
                // we don't want a current editor that is not open in a view
                setCurrentView(view);
                setCurrentEditor(0);
            }
            editors.removeAll(editor);
            view->removeEditor(editor);
            continue; // don't close the editor
        }
        emit m_instance->editorAboutToClose(editor);
        removeEditor(editor);
        view->removeEditor(editor);
    }
    if (!editors.isEmpty()) {
        emit m_instance->editorsClosed(editors);
        foreach (IEditor *editor, editors) {
            delete editor;
        }
    }
}

void EditorManagerPrivate::splitNewWindow(EditorView *view)
{
    IEditor *editor = view->currentEditor();
    IEditor *newEditor = 0;
    if (editor && editor->duplicateSupported())
        newEditor = EditorManagerPrivate::duplicateEditor(editor);
    else
        newEditor = editor; // move to the new view

    auto win = new EditorWindow;
    EditorArea *area = win->editorArea();
    d->m_editorAreas.append(area);
    connect(area, SIGNAL(destroyed(QObject*)), d, SLOT(editorAreaDestroyed(QObject*)));
    win->show();
    ICore::raiseWindow(win);
    if (newEditor)
        activateEditor(area->view(), newEditor, EditorManager::IgnoreNavigationHistory);
    else
        area->view()->setFocus();
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
    return 0;
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
    const bool hasFile = document != 0 && !document->filePath().isEmpty();
    saveAction->setEnabled(hasFile && document->isModified());
    saveAsAction->setEnabled(document != 0 && document->isSaveAsAllowed());
    revertToSavedAction->setEnabled(hasFile);

    const QString documentName = document ? document->displayName() : QString();
    QString quotedName;

    if (!documentName.isEmpty()) {
        quotedName = QLatin1Char('"') + documentName + QLatin1Char('"');
        saveAction->setText(tr("&Save %1").arg(quotedName));
        saveAsAction->setText(tr("Save %1 &As...").arg(quotedName));
        revertToSavedAction->setText(document->isModified()
                                     ? tr("Revert %1 to Saved").arg(quotedName)
                                     : tr("Reload %1").arg(quotedName));
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
        quotedName = QLatin1Char('"') + curDocument->displayName() + QLatin1Char('"');
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

    SplitterOrView *viewParent = (view ? view->parentSplitterOrView() : 0);
    SplitterOrView *parentSplitter = (viewParent ? viewParent->findParentSplitter() : 0);
    bool hasSplitter = parentSplitter && parentSplitter->isSplitter();
    d->m_removeCurrentSplitAction->setEnabled(hasSplitter);
    d->m_removeAllSplitsAction->setEnabled(hasSplitter);
    d->m_gotoNextSplitAction->setEnabled(hasSplitter || d->m_editorAreas.size() > 1);
}

void EditorManagerPrivate::updateWindowTitleForDocument(IDocument *document, QWidget *window)
{
    QTC_ASSERT(window, return);
    QString windowTitle;
    const QString dashSep = QLatin1String(" - ");

    QString filePath = document ? document->filePath().toFileInfo().absoluteFilePath()
                              : QString();

    const QString windowTitleAddition = d->m_titleAdditionHandler
            ? d->m_titleAdditionHandler(filePath)
            : QString();

    QString windowTitleVcsTopic;
    if (d->m_titleVcsTopicHandler)
        windowTitleVcsTopic = d->m_titleVcsTopicHandler(filePath);
    if (!windowTitleVcsTopic.isEmpty())
        windowTitleVcsTopic = QStringLiteral(" [") + windowTitleVcsTopic + QStringLiteral("]");

    const QString documentName = document ? document->displayName() : QString();

    if (!documentName.isEmpty())
        windowTitle.append(documentName + windowTitleVcsTopic + dashSep);
    if (!windowTitleAddition.isEmpty()) {
        windowTitle.append(windowTitleAddition);
        if (documentName.isEmpty()) // vcs topic not already added
            windowTitle.append(windowTitleVcsTopic);
        windowTitle.append(dashSep);
    }

    windowTitle.append(tr("Qt Creator"));
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
        QTC_CHECK(nextView);
    }

    if (nextView)
        activateView(nextView);
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
        QMessageBox::warning(ICore::mainWindow(), tr("Cannot Open File"),
                             tr("Cannot open the file for editing with VCS."));
    }
}

void EditorManagerPrivate::handleDocumentStateChange()
{
    updateActions();
    IDocument *document = qobject_cast<IDocument *>(sender());
    if (!document->isModified())
        document->removeAutoSaveFile();
    if (EditorManager::currentDocument() == document) {
        emit m_instance->currentDocumentStateChanged();
    }
}

void EditorManagerPrivate::editorAreaDestroyed(QObject *area)
{
    QWidget *activeWin = qApp->activeWindow();
    EditorArea *newActiveArea = 0;
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
    SplitterOrView *focusSplitterOrView = 0;
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
        if (document->filePath().isEmpty()) // FIXME: save them to a dedicated directory
            continue;
        QString errorString;
        if (!document->autoSave(&errorString, autoSaveName(document->filePath().toString())))
            errors << errorString;
    }
    if (!errors.isEmpty())
        QMessageBox::critical(ICore::mainWindow(), tr("File Error"),
                              errors.join(QLatin1Char('\n')));
}

void EditorManagerPrivate::handleContextChange(const QList<IContext *> &context)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO;
    d->m_scheduledCurrentEditor = 0;
    IEditor *editor = 0;
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
        QTimer::singleShot(0, d, SLOT(setCurrentEditorFromContextChange()));
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
    const QAction *action = qobject_cast<const QAction *>(sender());
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
    IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : 0;
    if (document)
        saveDocument(document);
}

void EditorManagerPrivate::saveDocumentAsFromContextMenu()
{
    IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : 0;
    if (document)
        saveDocumentAs(document);
}

void EditorManagerPrivate::revertToSavedFromContextMenu()
{
    IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : 0;
    if (document)
        revertToSaved(document);
}

void EditorManagerPrivate::closeEditorFromContextMenu()
{
    if (d->m_contextMenuEditor) {
        closeEditorOrDocument(d->m_contextMenuEditor);
    } else {
        IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : 0;
        if (document)
            EditorManager::closeDocument(document);
    }
}

void EditorManagerPrivate::closeOtherDocumentsFromContextMenu()
{
    IDocument *document = d->m_contextMenuEntry ? d->m_contextMenuEntry->document : 0;
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

    if (success)
        addDocumentToRecentFiles(document);

    return success;
}

bool EditorManagerPrivate::saveDocumentAs(IDocument *document)
{
    if (!document)
        return false;

    Utils::MimeDatabase mdb;
    const QString filter = Utils::MimeDatabase::allFiltersString();
    QString selectedFilter;
    QString filePath = document->filePath().toString();
    if (filePath.isEmpty())
        filePath = document->defaultPath() + QLatin1Char('/') + document->suggestedFileName();
    if (!filePath.isEmpty())
        selectedFilter = mdb.mimeTypeForFile(filePath).filterString();
    if (selectedFilter.isEmpty())
        selectedFilter = mdb.mimeTypeForName(document->mimeType()).filterString();
    const QString &absoluteFilePath =
        DocumentManager::getSaveAsFileName(document, filter, &selectedFilter);

    if (absoluteFilePath.isEmpty())
        return false;

    if (absoluteFilePath != document->filePath().toString()) {
        // close existing editors for the new file name
        IDocument *otherDocument = DocumentModel::documentForFilePath(absoluteFilePath);
        if (otherDocument)
            EditorManager::closeDocuments(QList<IDocument *>() << otherDocument, false);
    }

    const bool success = DocumentManager::saveDocument(document, absoluteFilePath);
    document->checkPermissions();

    // TODO: There is an issue to be treated here. The new file might be of a different mime
    // type than the original and thus require a different editor. An alternative strategy
    // would be to close the current editor and open a new appropriate one, but this is not
    // a good way out either (also the undo stack would be lost). Perhaps the best is to
    // re-think part of the editors design.

    if (success)
        addDocumentToRecentFiles(document);

    updateActions();
    return success;
}

void EditorManagerPrivate::closeAllEditorsExceptVisible()
{
    DocumentModel::removeAllRestoredEntries();
    QList<IDocument *> documentsToClose = DocumentModel::openedDocuments();
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
        // TODO: wrong dialog parent
        QMessageBox msgBox(QMessageBox::Question, tr("Revert to Saved"),
                           tr("You will lose your current changes if you proceed reverting %1.").arg(QDir::toNativeSeparators(fileName)),
                           QMessageBox::Yes|QMessageBox::No, ICore::mainWindow());
        msgBox.button(QMessageBox::Yes)->setText(tr("Proceed"));
        msgBox.button(QMessageBox::No)->setText(tr("Cancel"));
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;

    }
    QString errorString;
    if (!document->reload(&errorString, IDocument::FlagReload, IDocument::TypeContents))
        QMessageBox::critical(ICore::mainWindow(), tr("File Error"), errorString);
}

void EditorManagerPrivate::showInGraphicalShell()
{
    if (!d->m_contextMenuEntry || d->m_contextMenuEntry->fileName().isEmpty())
        return;
    FileUtils::showInGraphicalShell(ICore::mainWindow(), d->m_contextMenuEntry->fileName().toString());
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
    emit m_instance->findOnFileSystemRequest(d->m_contextMenuEntry->fileName().parentDir().toString());
}

void EditorManagerPrivate::split(Qt::Orientation orientation)
{
    EditorView *view = currentEditorView();

    if (view)
        view->parentSplitterOrView()->split(orientation);

    updateActions();
}

void EditorManagerPrivate::split()
{
    split(Qt::Vertical);
}

void EditorManagerPrivate::splitNewWindow()
{
    splitNewWindow(currentEditorView());
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
    d->m_scheduledCurrentEditor = 0;
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

EditorManager::EditorManager(QObject *parent) :
    QObject(parent)
{
    m_instance = this;
    d = new EditorManagerPrivate(this);
    d->init();
}

EditorManager::~EditorManager()
{
    delete d;
    m_instance = 0;
}

IDocument *EditorManager::currentDocument()
{
    return d->m_currentEditor ? d->m_currentEditor->document() : 0;
}

IEditor *EditorManager::currentEditor()
{
    return d->m_currentEditor;
}

bool EditorManager::closeAllEditors(bool askAboutModifiedEditors)
{
    DocumentModel::removeAllRestoredEntries();
    if (closeDocuments(DocumentModel::openedDocuments(), askAboutModifiedEditors))
        return true;
    return false;
}

void EditorManager::closeOtherDocuments(IDocument *document)
{
    DocumentModel::removeAllRestoredEntries();
    QList<IDocument *> documentsToClose = DocumentModel::openedDocuments();
    documentsToClose.removeAll(document);
    closeDocuments(documentsToClose, true);
}

// SLOT connected to action
void EditorManager::slotCloseCurrentEditorOrDocument()
{
    if (!d->m_currentEditor)
        return;
    addCurrentPositionToNavigationHistory();
    d->closeEditorOrDocument(d->m_currentEditor);
}

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

void EditorManager::addSaveAndCloseEditorActions(QMenu *contextMenu, DocumentModel::Entry *entry,
                                                 IEditor *editor)
{
    QTC_ASSERT(contextMenu, return);
    d->m_contextMenuEntry = entry;
    d->m_contextMenuEditor = editor;

    const FileName filePath = entry ? entry->fileName() : FileName();
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

    IDocument *document = entry ? entry->document : 0;

    EditorManagerPrivate::setupSaveActions(document,
                                           d->m_saveCurrentEditorContextAction,
                                           d->m_saveAsCurrentEditorContextAction,
                                           d->m_revertToSavedCurrentEditorContextAction);

    contextMenu->addAction(d->m_saveCurrentEditorContextAction);
    contextMenu->addAction(d->m_saveAsCurrentEditorContextAction);
    contextMenu->addAction(ActionManager::command(Constants::SAVEALL)->action());
    contextMenu->addAction(d->m_revertToSavedCurrentEditorContextAction);

    contextMenu->addSeparator();

    d->m_closeCurrentEditorContextAction->setText(entry
                                                    ? tr("Close \"%1\"").arg(entry->displayName())
                                                    : tr("Close Editor"));
    d->m_closeOtherDocumentsContextAction->setText(entry
                                                   ? tr("Close All Except \"%1\"").arg(entry->displayName())
                                                   : tr("Close Other Editors"));
    d->m_closeCurrentEditorContextAction->setEnabled(entry != 0);
    d->m_closeOtherDocumentsContextAction->setEnabled(entry != 0);
    d->m_closeAllEditorsContextAction->setEnabled(!DocumentModel::entries().isEmpty());
    d->m_closeAllEditorsExceptVisibleContextAction->setEnabled(
                EditorManagerPrivate::visibleDocumentsCount() < DocumentModel::entries().count());
    contextMenu->addAction(d->m_closeCurrentEditorContextAction);
    contextMenu->addAction(d->m_closeAllEditorsContextAction);
    contextMenu->addAction(d->m_closeOtherDocumentsContextAction);
    contextMenu->addAction(d->m_closeAllEditorsExceptVisibleContextAction);
}

void EditorManager::addNativeDirAndOpenWithActions(QMenu *contextMenu, DocumentModel::Entry *entry)
{
    QTC_ASSERT(contextMenu, return);
    d->m_contextMenuEntry = entry;
    bool enabled = entry && !entry->fileName().isEmpty();
    d->m_openGraphicalShellAction->setEnabled(enabled);
    d->m_openTerminalAction->setEnabled(enabled);
    d->m_findInDirectoryAction->setEnabled(enabled);
    contextMenu->addAction(d->m_openGraphicalShellAction);
    contextMenu->addAction(d->m_openTerminalAction);
    contextMenu->addAction(d->m_findInDirectoryAction);
    QMenu *openWith = contextMenu->addMenu(tr("Open With"));
    openWith->setEnabled(enabled);
    if (enabled)
        populateOpenWithMenu(openWith, entry->fileName().toString());
}

void EditorManager::populateOpenWithMenu(QMenu *menu, const QString &fileName)
{
    typedef QList<IEditorFactory*> EditorFactoryList;
    typedef QList<IExternalEditor*> ExternalEditorList;

    menu->clear();

    bool anyMatches = false;

    Utils::MimeDatabase mdb;
    const Utils::MimeType mt = mdb.mimeTypeForFile(fileName);
    if (mt.isValid()) {
        const EditorFactoryList factories = editorFactories(mt, false);
        const ExternalEditorList extEditors = externalEditors(mt, false);
        anyMatches = !factories.empty() || !extEditors.empty();
        if (anyMatches) {
            // Add all suitable editors
            foreach (IEditorFactory *editorFactory, factories) {
                Core::Id editorId = editorFactory->id();
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
                Core::Id editorId = externalEditor->id();
                connect(action, &QAction::triggered, [fileName, editorId]() {
                    EditorManager::openExternalEditor(fileName, editorId);
                });
            }
        }
    }
    menu->setEnabled(anyMatches);
}

IDocument::ReloadSetting EditorManager::reloadSetting()
{
    return d->m_reloadSetting;
}

void EditorManager::setReloadSetting(IDocument::ReloadSetting behavior)
{
     d->m_reloadSetting = behavior;
}

void EditorManager::saveDocument()
{
    EditorManagerPrivate::saveDocument(currentDocument());
}

void EditorManager::saveDocumentAs()
{
    EditorManagerPrivate::saveDocumentAs(currentDocument());
}

void EditorManager::revertToSaved()
{
    EditorManagerPrivate::revertToSaved(currentDocument());
}

void EditorManager::closeEditor(IEditor *editor, bool askAboutModifiedEditors)
{
    if (!editor)
        return;
    closeEditors(QList<IEditor *>() << editor, askAboutModifiedEditors);
}

void EditorManager::closeDocument(DocumentModel::Entry *entry)
{
    if (!entry)
        return;
    if (entry->isRestored)
        DocumentModel::removeEntry(entry);
    else
        closeDocuments(QList<IDocument *>() << entry->document);
}

bool EditorManager::closeEditors(const QList<IEditor*> &editorsToClose, bool askAboutModifiedEditors)
{
    if (editorsToClose.isEmpty())
        return true;
    bool closingFailed = false;
    // close Editor History list
    EditorManagerPrivate::windowPopup()->setVisible(false);


    EditorView *currentView = EditorManagerPrivate::currentEditorView();

    // go through all editors to close and
    // 1. ask all core listeners to check whether the editor can be closed
    // 2. keep track of the document and all the editors that might remain open for it
    QSet<IEditor*> acceptedEditors;
    QMap<IDocument *, QList<IEditor *> > documentMap;
    const QList<ICoreListener *> listeners =
        ExtensionSystem::PluginManager::getObjects<ICoreListener>();
    foreach (IEditor *editor, editorsToClose) {
        bool editorAccepted = true;
        foreach (ICoreListener *listener, listeners) {
            if (!listener->editorAboutToClose(editor)) {
                editorAccepted = false;
                closingFailed = true;
                break;
            }
        }
        if (editorAccepted) {
            acceptedEditors.insert(editor);
            IDocument *document = editor->document();
            if (!documentMap.contains(document)) // insert the document to track
                documentMap.insert(document, DocumentModel::editorsForDocument(document));
            // keep track that we'll close this editor for the document
            documentMap[document].removeAll(editor);
        }
    }
    if (acceptedEditors.isEmpty())
        return false;

    //ask whether to save modified documents that we are about to close
    if (askAboutModifiedEditors) {
        // Check for which documents we will close all editors, and therefore might have to ask the user
        QList<IDocument *> documentsToClose;
        for (auto i = documentMap.constBegin(); i != documentMap.constEnd(); ++i) {
            if (i.value().isEmpty())
                documentsToClose.append(i.key());
        }

        bool cancelled = false;
        QList<IDocument *> rejectedList;
        DocumentManager::saveModifiedDocuments(documentsToClose, QString(), &cancelled,
                                               QString(), 0, &rejectedList);
        if (cancelled)
            return false;
        if (!rejectedList.isEmpty()) {
            closingFailed = true;
            QSet<IEditor*> skipSet = DocumentModel::editorsForDocuments(rejectedList).toSet();
            acceptedEditors = acceptedEditors.subtract(skipSet);
        }
    }
    if (acceptedEditors.isEmpty())
        return false;

    QList<EditorView*> closedViews;
    EditorView *focusView = 0;

    // remove the editors
    foreach (IEditor *editor, acceptedEditors) {
        emit m_instance->editorAboutToClose(editor);
        if (!editor->document()->filePath().isEmpty()
                && !editor->document()->isTemporary()) {
            QByteArray state = editor->saveState();
            if (!state.isEmpty())
                d->m_editorStates.insert(editor->document()->filePath().toString(), QVariant(state));
        }

        EditorManagerPrivate::removeEditor(editor);
        if (EditorView *view = EditorManagerPrivate::viewForEditor(editor)) {
            if (qApp->focusWidget() && qApp->focusWidget() == editor->widget()->focusWidget())
                focusView = view;
            if (editor == view->currentEditor())
                closedViews += view;
            if (d->m_currentEditor == editor) {
                // avoid having a current editor without view
                EditorManagerPrivate::setCurrentView(view);
                EditorManagerPrivate::setCurrentEditor(0);
            }
            view->removeEditor(editor);
        }
    }

    // TODO doesn't work as expected with multiple areas in main window and some other cases
    // instead each view should have its own file history and handle solely themselves
    // which editor is shown if their current editor closes
    EditorView *forceViewToShowEditor = 0;
    if (!closedViews.isEmpty() && visibleEditors().isEmpty()) {
        if (closedViews.contains(currentView))
            forceViewToShowEditor = currentView;
        else
            forceViewToShowEditor = closedViews.first();
    }
    foreach (EditorView *view, closedViews) {
        IEditor *newCurrent = view->currentEditor();
        if (!newCurrent && forceViewToShowEditor == view)
            newCurrent = EditorManagerPrivate::pickUnusedEditor();
        if (newCurrent) {
            EditorManagerPrivate::activateEditor(view, newCurrent, DoNotChangeCurrentEditor);
        } else if (forceViewToShowEditor == view) {
            DocumentModel::Entry *entry = DocumentModel::firstRestoredEntry();
            if (entry) {
                EditorManagerPrivate::activateEditorForEntry(view, entry, DoNotChangeCurrentEditor);
            } else { // no "restored" ones, so any entry left should have a document
                const QList<DocumentModel::Entry *> documents = DocumentModel::entries();
                if (!documents.isEmpty()) {
                    if (IDocument *document = documents.last()->document) {
                        EditorManagerPrivate::activateEditorForDocument(
                                    view, document, DoNotChangeCurrentEditor);
                    }
                }
            }
        }
    }

    emit m_instance->editorsClosed(acceptedEditors.toList());

    foreach (IEditor *editor, acceptedEditors)
        delete editor;

    if (focusView)
        EditorManagerPrivate::activateView(focusView);
    else
        EditorManagerPrivate::setCurrentEditor(currentView->currentEditor());

    if (!currentEditor()) {
        emit m_instance->currentEditorChanged(0);
        EditorManagerPrivate::updateActions();
    }

    return !closingFailed;
}

void EditorManager::activateEditorForEntry(DocumentModel::Entry *entry, OpenEditorFlags flags)
{
    EditorManagerPrivate::activateEditorForEntry(EditorManagerPrivate::currentEditorView(),
                                                 entry, flags);
}

void EditorManager::activateEditor(IEditor *editor, OpenEditorFlags flags)
{
    QTC_ASSERT(editor, return);
    EditorView *view = EditorManagerPrivate::viewForEditor(editor);
    // an IEditor doesn't have to belong to a view, it might be kept in storage by the editor model
    if (!view)
        view = EditorManagerPrivate::currentEditorView();
    EditorManagerPrivate::activateEditor(view, editor, flags);
}

IEditor *EditorManager::activateEditorForDocument(IDocument *document, OpenEditorFlags flags)
{
    return EditorManagerPrivate::activateEditorForDocument(EditorManagerPrivate::currentEditorView(), document, flags);
}

/* For something that has a 'QStringList mimeTypes' (IEditorFactory
 * or IExternalEditor), find the one best matching the mimetype passed in.
 *  Recurse over the parent classes of the mimetype to find them. */
template <class EditorFactoryLike>
static void mimeTypeFactoryLookup(const Utils::MimeType &mimeType,
                                     const QList<EditorFactoryLike*> &allFactories,
                                     bool firstMatchOnly,
                                     QList<EditorFactoryLike*> *list)
{
    Utils::MimeDatabase mdb;
    QSet<EditorFactoryLike *> matches;
    // search breadth-first through parent hierarchy, e.g. for hierarchy
    // * application/x-ruby
    //     * application/x-executable
    //         * application/octet-stream
    //     * text/plain
    QList<Utils::MimeType> queue;
    queue.append(mimeType);
    while (!queue.isEmpty()) {
        Utils::MimeType mt = queue.takeFirst();
        // check for matching factories
        foreach (EditorFactoryLike *factory, allFactories) {
            if (!matches.contains(factory)) {
                foreach (const QString &mimeName, factory->mimeTypes()) {
                    if (mt.matchesName(mimeName)) {
                        list->append(factory);
                        if (firstMatchOnly)
                            return;
                        matches.insert(factory);
                    }
                }
            }
        }
        // add parent mime types
        QStringList parentNames = mt.parentMimeTypes();
        foreach (const QString &parentName, parentNames) {
            const Utils::MimeType parent = mdb.mimeTypeForName(parentName);
            if (parent.isValid())
                queue.append(parent);
        }
    }
}

EditorManager::EditorFactoryList
    EditorManager::editorFactories(const Utils::MimeType &mimeType, bool bestMatchOnly)
{
    EditorFactoryList rc;
    const EditorFactoryList allFactories = ExtensionSystem::PluginManager::getObjects<IEditorFactory>();
    mimeTypeFactoryLookup(mimeType, allFactories, bestMatchOnly, &rc);
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << mimeType.name() << " returns " << rc;
    return rc;
}

EditorManager::ExternalEditorList
        EditorManager::externalEditors(const Utils::MimeType &mimeType, bool bestMatchOnly)
{
    ExternalEditorList rc;
    const ExternalEditorList allEditors = ExtensionSystem::PluginManager::getObjects<IExternalEditor>();
    mimeTypeFactoryLookup(mimeType, allEditors, bestMatchOnly, &rc);
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << mimeType.name() << " returns " << rc;
    return rc;
}

IEditor *EditorManager::openEditor(const QString &fileName, Id editorId,
                                   OpenEditorFlags flags, bool *newEditor)
{
    if (EditorManagerPrivate::skipOpeningBigTextFile(fileName))
        return 0;

    if (flags & EditorManager::OpenInOtherSplit)
        EditorManager::gotoOtherSplit();

    return EditorManagerPrivate::openEditor(EditorManagerPrivate::currentEditorView(),
                                            fileName, editorId, flags, newEditor);
}

IEditor *EditorManager::openEditorAt(const QString &fileName, int line, int column,
                                     Id editorId, OpenEditorFlags flags, bool *newEditor)
{
    if (EditorManagerPrivate::skipOpeningBigTextFile(fileName))
        return 0;

    if (flags & EditorManager::OpenInOtherSplit)
        EditorManager::gotoOtherSplit();

    return EditorManagerPrivate::openEditorAt(EditorManagerPrivate::currentEditorView(),
                                              fileName, line, column, editorId, flags, newEditor);
}

// Extract line and column number suffix. Return the suffix (e.g. ":132") and truncates the filename accordingly.
QString EditorManager::splitLineAndColumnNumber(QString *fileName)
{
    QString postfix;
    if (extractNumericSuffix(fileName, &postfix)) {
        QString previousPostfix;
        if (extractNumericSuffix(fileName, &previousPostfix))
            postfix.prepend(previousPostfix);
        return postfix;
    }
    return QString();
}

bool EditorManager::isAutoSaveFile(const QString &fileName)
{
    return fileName.endsWith(QLatin1String(".autosave"));
}

bool EditorManager::openExternalEditor(const QString &fileName, Id editorId)
{
    IExternalEditor *ee = findById<IExternalEditor>(editorId);
    if (!ee)
        return false;
    QString errorMessage;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    const bool ok = ee->startEditor(fileName, &errorMessage);
    QApplication::restoreOverrideCursor();
    if (!ok)
        QMessageBox::critical(ICore::mainWindow(), tr("Opening File"), errorMessage);
    return ok;
}

QStringList EditorManager::getOpenFileNames()
{
    QString selectedFilter;
    const QString &fileFilters = Utils::MimeDatabase::allFiltersString(&selectedFilter);
    return DocumentManager::getOpenFileNames(fileFilters, QString(), &selectedFilter);
}

static QString makeTitleUnique(QString *titlePattern)
{
    QString title;
    if (titlePattern) {
        const QChar dollar = QLatin1Char('$');

        QString base = *titlePattern;
        if (base.isEmpty())
            base = QLatin1String("unnamed$");
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

    IEditor *edt = 0;
    if (!uniqueId.isEmpty()) {
        foreach (IDocument *document, DocumentModel::openedDocuments())
            if (document->property(scratchBufferKey).toString() == uniqueId) {
                edt = DocumentModel::editorsForDocument(document).first();

                document->setContents(contents);
                if (!title.isEmpty())
                    edt->document()->setPreferredDisplayName(title);

                activateEditor(edt, flags);
                return edt;
            }
    }

    EditorFactoryList factories = EditorManagerPrivate::findFactories(editorId, title);
    if (factories.isEmpty())
        return 0;

    edt = EditorManagerPrivate::createEditor(factories.first(), title);
    if (!edt)
        return 0;
    if (!edt->document()->setContents(contents)) {
        delete edt;
        edt = 0;
        return 0;
    }

    if (!uniqueId.isEmpty())
        edt->document()->setProperty(scratchBufferKey, uniqueId);

    if (!title.isEmpty())
        edt->document()->setPreferredDisplayName(title);

    EditorManagerPrivate::addEditor(edt);
    activateEditor(edt, flags);
    return edt;
}

bool EditorManager::saveDocument(IDocument *document)
{
    return EditorManagerPrivate::saveDocument(document);
}

bool EditorManager::hasSplitter()
{
    EditorView *view = EditorManagerPrivate::currentEditorView();
    QTC_ASSERT(view, return false);
    EditorArea *area = EditorManagerPrivate::findEditorArea(view);
    QTC_ASSERT(area, return false);
    return area->isSplitter();
}

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

bool EditorManager::closeDocument(IDocument *document, bool askAboutModifiedEditors)
{
    return closeDocuments(QList<IDocument *>() << document, askAboutModifiedEditors);
}

bool EditorManager::closeDocuments(const QList<IDocument *> &documents, bool askAboutModifiedEditors)
{
    return m_instance->closeEditors(DocumentModel::editorsForDocuments(documents), askAboutModifiedEditors);
}

void EditorManager::addCurrentPositionToNavigationHistory(const QByteArray &saveState)
{
    EditorManagerPrivate::currentEditorView()->addCurrentPositionToNavigationHistory(saveState);
    EditorManagerPrivate::updateActions();
}

void EditorManager::cutForwardNavigationHistory()
{
    EditorManagerPrivate::currentEditorView()->cutForwardNavigationHistory();
    EditorManagerPrivate::updateActions();
}

void EditorManager::goBackInNavigationHistory()
{
    EditorManagerPrivate::currentEditorView()->goBackInNavigationHistory();
    EditorManagerPrivate::updateActions();
    return;
}

void EditorManager::goForwardInNavigationHistory()
{
    EditorManagerPrivate::currentEditorView()->goForwardInNavigationHistory();
    EditorManagerPrivate::updateActions();
}

// Save state of all non-teporary editors.
QByteArray EditorManager::saveState()
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);

    stream << QByteArray("EditorManagerV4");

    // TODO: In case of split views it's not possible to restore these for all correctly with this
    QList<IDocument *> documents = DocumentModel::openedDocuments();
    foreach (IDocument *document, documents) {
        if (!document->filePath().isEmpty() && !document->isTemporary()) {
            IEditor *editor = DocumentModel::editorsForDocument(document).first();
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
        if (!entry->document->isTemporary())
            stream << entry->fileName().toString() << entry->plainDisplayName() << entry->id();
    }

    stream << d->m_editorAreas.first()->saveState(); // TODO

    return bytes;
}

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

    if (version != "EditorManagerV4")
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

        if (!fileName.isEmpty() && !displayName.isEmpty()) {
            QFileInfo fi(fileName);
            if (!fi.exists())
                continue;
            QFileInfo rfi(autoSaveName(fileName));
            if (rfi.exists() && fi.lastModified() < rfi.lastModified())
                openEditor(fileName, id, DoNotMakeVisible);
            else
                DocumentModel::addRestoredDocument(fileName, displayName, id);
        }
    }

    QByteArray splitterstates;
    stream >> splitterstates;
    d->m_editorAreas.first()->restoreState(splitterstates); // TODO

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

void EditorManager::showEditorStatusBar(const QString &id,
                                      const QString &infoText,
                                      const QString &buttonText,
                                      QObject *object, const char *member)
{

    EditorManagerPrivate::currentEditorView()->showEditorStatusBar(id, infoText, buttonText, object, member);
}

void EditorManager::hideEditorStatusBar(const QString &id)
{
    // TODO: what if the current editor view betwenn show and hideEditorStatusBar changed?
    EditorManagerPrivate::currentEditorView()->hideEditorStatusBar(id);
}

QTextCodec *EditorManager::defaultTextCodec()
{
    QSettings *settings = ICore::settings();
    if (QTextCodec *candidate = QTextCodec::codecForName(
            settings->value(QLatin1String(Constants::SETTINGS_DEFAULTTEXTENCODING)).toByteArray()))
        return candidate;
    if (QTextCodec *defaultUTF8 = QTextCodec::codecForName("UTF-8"))
        return defaultUTF8;
    return QTextCodec::codecForLocale();
}

void EditorManager::splitSideBySide()
{
    EditorManagerPrivate::split(Qt::Horizontal);
}

/*!
 * Moves focus to "other" split, possibly creating a split if necessary.
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

qint64 EditorManager::maxTextFileSize()
{
    return qint64(3) << 24;
}

void EditorManager::setWindowTitleAdditionHandler(WindowTitleHandler handler)
{
    d->m_titleAdditionHandler = handler;
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
