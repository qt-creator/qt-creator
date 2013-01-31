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

#include "editormanager.h"
#include "editorview.h"
#include "openeditorswindow.h"
#include "openeditorsview.h"
#include "openeditorsmodel.h"
#include "openwithdialog.h"
#include "documentmanager.h"
#include "icore.h"
#include "ieditor.h"
#include "iversioncontrol.h"
#include "mimedatabase.h"
#include "tabpositionindicator.h"
#include "vcsmanager.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/editormanager/iexternaleditor.h>
#include <coreplugin/editortoolbar.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/icorelistener.h>
#include <coreplugin/id.h>
#include <coreplugin/imode.h>
#include <coreplugin/infobar.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/settingsdatabase.h>
#include <coreplugin/variablemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/consoleprocess.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QMap>
#include <QProcess>
#include <QSet>
#include <QSettings>
#include <QTextCodec>
#include <QTimer>

#include <QAction>
#include <QShortcut>
#include <QApplication>
#include <QFileDialog>
#include <QLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStackedLayout>

enum { debugEditorManager=0 };

static const char kCurrentDocumentFilePath[] = "CurrentDocument:FilePath";
static const char kCurrentDocumentPath[] = "CurrentDocument:Path";
static const char kCurrentDocumentXPos[] = "CurrentDocument:XPos";
static const char kCurrentDocumentYPos[] = "CurrentDocument:YPos";
static const char kMakeWritableWarning[] = "Core.EditorManager.MakeWritable";

//===================EditorClosingCoreListener======================

namespace Core {
namespace Internal {

class EditorClosingCoreListener : public ICoreListener
{
public:
    EditorClosingCoreListener(EditorManager *em);
    bool editorAboutToClose(IEditor *editor);
    bool coreAboutToClose();

private:
    EditorManager *m_em;
};

EditorClosingCoreListener::EditorClosingCoreListener(EditorManager *em)
        : m_em(em)
{
}

bool EditorClosingCoreListener::editorAboutToClose(IEditor *)
{
    return true;
}

bool EditorClosingCoreListener::coreAboutToClose()
{
    // Do not ask for files to save.
    // MainWindow::closeEvent has already done that.
    return m_em->closeAllEditors(false);
}

} // namespace Internal
} // namespace Core

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

//===================EditorManager=====================

EditorManagerPlaceHolder *EditorManagerPlaceHolder::m_current = 0;

EditorManagerPlaceHolder::EditorManagerPlaceHolder(Core::IMode *mode, QWidget *parent)
    : QWidget(parent), m_mode(mode)
{
    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode*)),
            this, SLOT(currentModeChanged(Core::IMode*)));

    currentModeChanged(ModeManager::currentMode());
}

EditorManagerPlaceHolder::~EditorManagerPlaceHolder()
{
    if (m_current == this) {
        EditorManager::instance()->setParent(0);
        EditorManager::instance()->hide();
    }
}

void EditorManagerPlaceHolder::currentModeChanged(Core::IMode *mode)
{
    if (m_current == this) {
        m_current = 0;
        EditorManager::instance()->setParent(0);
        EditorManager::instance()->hide();
    }
    if (m_mode == mode) {
        m_current = this;
        layout()->addWidget(EditorManager::instance());
        EditorManager::instance()->show();
    }
}

EditorManagerPlaceHolder* EditorManagerPlaceHolder::current()
{
    return m_current;
}

// ---------------- EditorManager

namespace Core {


struct EditorManagerPrivate
{
    explicit EditorManagerPrivate(QWidget *parent);
    ~EditorManagerPrivate();
    Internal::EditorView *m_view;
    Internal::SplitterOrView *m_splitter;
    QPointer<IEditor> m_currentEditor;
    QPointer<SplitterOrView> m_currentView;
    QTimer *m_autoSaveTimer;

    // actions
    QAction *m_revertToSavedAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_closeCurrentEditorAction;
    QAction *m_closeAllEditorsAction;
    QAction *m_closeOtherEditorsAction;
    QAction *m_gotoNextDocHistoryAction;
    QAction *m_gotoPreviousDocHistoryAction;
    QAction *m_goBackAction;
    QAction *m_goForwardAction;
    QAction *m_splitAction;
    QAction *m_splitSideBySideAction;
    QAction *m_removeCurrentSplitAction;
    QAction *m_removeAllSplitsAction;
    QAction *m_gotoOtherSplitAction;

    QAction *m_saveCurrentEditorContextAction;
    QAction *m_saveAsCurrentEditorContextAction;
    QAction *m_revertToSavedCurrentEditorContextAction;

    QAction *m_closeCurrentEditorContextAction;
    QAction *m_closeAllEditorsContextAction;
    QAction *m_closeOtherEditorsContextAction;
    QAction *m_openGraphicalShellAction;
    QAction *m_openTerminalAction;
    QModelIndex m_contextMenuEditorIndex;

    Internal::OpenEditorsWindow *m_windowPopup;
    Internal::EditorClosingCoreListener *m_coreListener;

    QMap<QString, QVariant> m_editorStates;
    Internal::OpenEditorsViewFactory *m_openEditorsFactory;

    OpenEditorsModel *m_editorModel;

    IDocument::ReloadSetting m_reloadSetting;

    QString m_titleAddition;

    bool m_autoSaveEnabled;
    int m_autoSaveInterval;
};
}

EditorManagerPrivate::EditorManagerPrivate(QWidget *parent) :
    m_view(0),
    m_splitter(0),
    m_autoSaveTimer(0),
    m_revertToSavedAction(new QAction(EditorManager::tr("Revert to Saved"), parent)),
    m_saveAction(new QAction(parent)),
    m_saveAsAction(new QAction(parent)),
    m_closeCurrentEditorAction(new QAction(EditorManager::tr("Close"), parent)),
    m_closeAllEditorsAction(new QAction(EditorManager::tr("Close All"), parent)),
    m_closeOtherEditorsAction(new QAction(EditorManager::tr("Close Others"), parent)),
    m_gotoNextDocHistoryAction(new QAction(EditorManager::tr("Next Open Document in History"), parent)),
    m_gotoPreviousDocHistoryAction(new QAction(EditorManager::tr("Previous Open Document in History"), parent)),
    m_goBackAction(new QAction(QIcon(QLatin1String(Constants::ICON_PREV)), EditorManager::tr("Go Back"), parent)),
    m_goForwardAction(new QAction(QIcon(QLatin1String(Constants::ICON_NEXT)), EditorManager::tr("Go Forward"), parent)),
    m_saveCurrentEditorContextAction(new QAction(EditorManager::tr("&Save"), parent)),
    m_saveAsCurrentEditorContextAction(new QAction(EditorManager::tr("Save &As..."), parent)),
    m_revertToSavedCurrentEditorContextAction(new QAction(EditorManager::tr("Revert to Saved"), parent)),
    m_closeCurrentEditorContextAction(new QAction(EditorManager::tr("Close"), parent)),
    m_closeAllEditorsContextAction(new QAction(EditorManager::tr("Close All"), parent)),
    m_closeOtherEditorsContextAction(new QAction(EditorManager::tr("Close Others"), parent)),
    m_openGraphicalShellAction(new QAction(FileUtils::msgGraphicalShellAction(), parent)),
    m_openTerminalAction(new QAction(FileUtils::msgTerminalAction(), parent)),
    m_windowPopup(0),
    m_coreListener(0),
    m_reloadSetting(IDocument::AlwaysAsk),
    m_autoSaveEnabled(true),
    m_autoSaveInterval(5)
{
    m_editorModel = new OpenEditorsModel(parent);
}

EditorManagerPrivate::~EditorManagerPrivate()
{
//    clearNavigationHistory();
}

static EditorManager *m_instance = 0;

EditorManager *EditorManager::instance() { return m_instance; }

EditorManager::EditorManager(QWidget *parent) :
    QWidget(parent),
    d(new EditorManagerPrivate(parent))
{
    m_instance = this;

    connect(ICore::instance(), SIGNAL(contextAboutToChange(Core::IContext*)),
            this, SLOT(handleContextChange(Core::IContext*)));

    const Context editManagerContext(Constants::C_EDITORMANAGER);
    // combined context for edit & design modes
    const Context editDesignContext(Constants::C_EDITORMANAGER, Constants::C_DESIGN_MODE);

    ActionContainer *mfile = ActionManager::actionContainer(Constants::M_FILE);

    // Revert to saved
    d->m_revertToSavedAction->setIcon(QIcon::fromTheme(QLatin1String("document-revert")));
    Command *cmd = ActionManager::registerAction(d->m_revertToSavedAction,
                                       Constants::REVERTTOSAVED, editManagerContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDescription(tr("Revert File to Saved"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(d->m_revertToSavedAction, SIGNAL(triggered()), this, SLOT(revertToSaved()));

    // Save Action
    ActionManager::registerAction(d->m_saveAction, Constants::SAVE, editManagerContext);
    connect(d->m_saveAction, SIGNAL(triggered()), this, SLOT(saveDocument()));

    // Save As Action
    ActionManager::registerAction(d->m_saveAsAction, Constants::SAVEAS, editManagerContext);
    connect(d->m_saveAsAction, SIGNAL(triggered()), this, SLOT(saveDocumentAs()));

    // Window Menu
    ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);

    // Window menu separators
    mwindow->addSeparator(editManagerContext, Constants::G_WINDOW_SPLIT);
    mwindow->addSeparator(editManagerContext, Constants::G_WINDOW_NAVIGATE);

    // Close Action
    cmd = ActionManager::registerAction(d->m_closeCurrentEditorAction, Constants::CLOSE, editManagerContext, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+W")));
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDescription(d->m_closeCurrentEditorAction->text());
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(d->m_closeCurrentEditorAction, SIGNAL(triggered()), this, SLOT(closeEditor()));

#ifdef Q_OS_WIN
    // workaround for QTCREATORBUG-72
    QShortcut *sc = new QShortcut(parent);
    cmd = ActionManager::registerShortcut(sc, Constants::CLOSE_ALTERNATIVE, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+F4")));
    cmd->setDescription(EditorManager::tr("Close"));
    connect(sc, SIGNAL(activated()), this, SLOT(closeEditor()));
#endif

    // Close All Action
    cmd = ActionManager::registerAction(d->m_closeAllEditorsAction, Constants::CLOSEALL, editManagerContext, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+W")));
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(d->m_closeAllEditorsAction, SIGNAL(triggered()), this, SLOT(closeAllEditors()));

    // Close All Others Action
    cmd = ActionManager::registerAction(d->m_closeOtherEditorsAction, Constants::CLOSEOTHERS, editManagerContext, true);
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    cmd->setAttribute(Core::Command::CA_UpdateText);
    connect(d->m_closeOtherEditorsAction, SIGNAL(triggered()), this, SLOT(closeOtherEditors()));

    //Save XXX Context Actions
    connect(d->m_saveCurrentEditorContextAction, SIGNAL(triggered()), this, SLOT(saveDocumentFromContextMenu()));
    connect(d->m_saveAsCurrentEditorContextAction, SIGNAL(triggered()), this, SLOT(saveDocumentAsFromContextMenu()));
    connect(d->m_revertToSavedCurrentEditorContextAction, SIGNAL(triggered()), this, SLOT(revertToSavedFromContextMenu()));

    // Close XXX Context Actions
    connect(d->m_closeAllEditorsContextAction, SIGNAL(triggered()), this, SLOT(closeAllEditors()));
    connect(d->m_closeCurrentEditorContextAction, SIGNAL(triggered()), this, SLOT(closeEditorFromContextMenu()));
    connect(d->m_closeOtherEditorsContextAction, SIGNAL(triggered()), this, SLOT(closeOtherEditorsFromContextMenu()));

    connect(d->m_openGraphicalShellAction, SIGNAL(triggered()), this, SLOT(showInGraphicalShell()));
    connect(d->m_openTerminalAction, SIGNAL(triggered()), this, SLOT(openTerminal()));

    // Goto Previous In History Action
    cmd = ActionManager::registerAction(d->m_gotoPreviousDocHistoryAction, Constants::GOTOPREVINHISTORY, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Alt+Tab") : tr("Ctrl+Tab")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(d->m_gotoPreviousDocHistoryAction, SIGNAL(triggered()), this, SLOT(gotoPreviousDocHistory()));

    // Goto Next In History Action
    cmd = ActionManager::registerAction(d->m_gotoNextDocHistoryAction, Constants::GOTONEXTINHISTORY, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Alt+Shift+Tab") : tr("Ctrl+Shift+Tab")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(d->m_gotoNextDocHistoryAction, SIGNAL(triggered()), this, SLOT(gotoNextDocHistory()));

    // Go back in navigation history
    cmd = ActionManager::registerAction(d->m_goBackAction, Constants::GO_BACK, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Alt+Left") : tr("Alt+Left")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(d->m_goBackAction, SIGNAL(triggered()), this, SLOT(goBackInNavigationHistory()));

    // Go forward in navigation history
    cmd = ActionManager::registerAction(d->m_goForwardAction, Constants::GO_FORWARD, editDesignContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Ctrl+Alt+Right") : tr("Alt+Right")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(d->m_goForwardAction, SIGNAL(triggered()), this, SLOT(goForwardInNavigationHistory()));

    d->m_splitAction = new QAction(tr("Split"), this);
    cmd = ActionManager::registerAction(d->m_splitAction, Constants::SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,2") : tr("Ctrl+E,2")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(d->m_splitAction, SIGNAL(triggered()), this, SLOT(split()));

    d->m_splitSideBySideAction = new QAction(tr("Split Side by Side"), this);
    cmd = ActionManager::registerAction(d->m_splitSideBySideAction, Constants::SPLIT_SIDE_BY_SIDE, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,3") : tr("Ctrl+E,3")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(d->m_splitSideBySideAction, SIGNAL(triggered()), this, SLOT(splitSideBySide()));

    d->m_removeCurrentSplitAction = new QAction(tr("Remove Current Split"), this);
    cmd = ActionManager::registerAction(d->m_removeCurrentSplitAction, Constants::REMOVE_CURRENT_SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,0") : tr("Ctrl+E,0")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(d->m_removeCurrentSplitAction, SIGNAL(triggered()), this, SLOT(removeCurrentSplit()));

    d->m_removeAllSplitsAction = new QAction(tr("Remove All Splits"), this);
    cmd = ActionManager::registerAction(d->m_removeAllSplitsAction, Constants::REMOVE_ALL_SPLITS, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,1") : tr("Ctrl+E,1")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(d->m_removeAllSplitsAction, SIGNAL(triggered()), this, SLOT(removeAllSplits()));

    d->m_gotoOtherSplitAction = new QAction(tr("Go to Next Split"), this);
    cmd = ActionManager::registerAction(d->m_gotoOtherSplitAction, Constants::GOTO_OTHER_SPLIT, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+E,o") : tr("Ctrl+E,o")));
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(d->m_gotoOtherSplitAction, SIGNAL(triggered()), this, SLOT(gotoOtherSplit()));

    ActionContainer *medit = ActionManager::actionContainer(Constants::M_EDIT);
    ActionContainer *advancedMenu = ActionManager::createMenu(Constants::M_EDIT_ADVANCED);
    medit->addMenu(advancedMenu, Constants::G_EDIT_ADVANCED);
    advancedMenu->menu()->setTitle(tr("Ad&vanced"));
    advancedMenu->appendGroup(Constants::G_EDIT_FORMAT);
    advancedMenu->appendGroup(Constants::G_EDIT_COLLAPSING);
    advancedMenu->appendGroup(Constants::G_EDIT_BLOCKS);
    advancedMenu->appendGroup(Constants::G_EDIT_FONT);
    advancedMenu->appendGroup(Constants::G_EDIT_EDITOR);

    // Advanced menu separators
    advancedMenu->addSeparator(editManagerContext, Constants::G_EDIT_COLLAPSING);
    advancedMenu->addSeparator(editManagerContext, Constants::G_EDIT_BLOCKS);
    advancedMenu->addSeparator(editManagerContext, Constants::G_EDIT_FONT);
    advancedMenu->addSeparator(editManagerContext, Constants::G_EDIT_EDITOR);

    // other setup
    d->m_splitter = new SplitterOrView(d->m_editorModel);
    d->m_view = d->m_splitter->view();


    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(d->m_splitter);

    updateActions();

    d->m_windowPopup = new OpenEditorsWindow(this);

    d->m_autoSaveTimer = new QTimer(this);
    connect(d->m_autoSaveTimer, SIGNAL(timeout()), SLOT(autoSave()));
    updateAutoSave();
}

EditorManager::~EditorManager()
{
    m_instance = 0;
    if (ICore::instance()) {
        if (d->m_coreListener) {
            ExtensionSystem::PluginManager::removeObject(d->m_coreListener);
            delete d->m_coreListener;
        }
        ExtensionSystem::PluginManager::removeObject(d->m_openEditorsFactory);
        delete d->m_openEditorsFactory;
    }
    delete d;
}

void EditorManager::init()
{
    d->m_coreListener = new EditorClosingCoreListener(this);
    ExtensionSystem::PluginManager::addObject(d->m_coreListener);

    d->m_openEditorsFactory = new OpenEditorsViewFactory();
    ExtensionSystem::PluginManager::addObject(d->m_openEditorsFactory);

    VariableManager *vm = VariableManager::instance();
    vm->registerVariable(kCurrentDocumentFilePath,
        tr("Full path of the current document including file name."));
    vm->registerVariable(kCurrentDocumentPath,
        tr("Full path of the current document excluding file name."));
    vm->registerVariable(kCurrentDocumentXPos,
        tr("X-coordinate of the current editor's upper left corner, relative to screen."));
    vm->registerVariable(kCurrentDocumentYPos,
        tr("Y-coordinate of the current editor's upper left corner, relative to screen."));
    connect(vm, SIGNAL(variableUpdateRequested(QByteArray)),
            this, SLOT(updateVariable(QByteArray)));
}

void EditorManager::updateAutoSave()
{
    if (d->m_autoSaveEnabled)
        d->m_autoSaveTimer->start(d->m_autoSaveInterval * (60 * 1000));
    else
        d->m_autoSaveTimer->stop();
}

EditorToolBar *EditorManager::createToolBar(QWidget *parent)
{
    return new EditorToolBar(parent);
}

void EditorManager::removeEditor(IEditor *editor)
{
    bool isDuplicate = d->m_editorModel->isDuplicate(editor);
    d->m_editorModel->removeEditor(editor);
    if (!isDuplicate)
        DocumentManager::removeDocument(editor->document());
    ICore::removeContextObject(editor);
}

void EditorManager::handleContextChange(Core::IContext *context)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO;
    IEditor *editor = context ? qobject_cast<IEditor*>(context) : 0;
    if (editor)
        setCurrentEditor(editor);
    else
        updateActions();
}

void EditorManager::setCurrentEditor(IEditor *editor, bool ignoreNavigationHistory)
{
    if (editor)
        setCurrentView(0);

    if (d->m_currentEditor == editor)
        return;
    if (d->m_currentEditor && !ignoreNavigationHistory)
        addCurrentPositionToNavigationHistory();

    d->m_currentEditor = editor;
    if (editor) {
        if (SplitterOrView *splitterOrView = d->m_splitter->findView(editor))
            splitterOrView->view()->setCurrentEditor(editor);
        d->m_view->updateEditorHistory(editor); // the global view should have a complete history
    }
    updateActions();
    updateWindowTitle();
    emit currentEditorChanged(editor);
}


void EditorManager::setCurrentView(Core::Internal::SplitterOrView *view)
{
    if (view == d->m_currentView)
        return;

    SplitterOrView *old = d->m_currentView;
    d->m_currentView = view;

    if (old)
        old->update();
    if (view)
        view->update();

    if (view && !view->editor())
        view->setFocus();
}

Core::Internal::SplitterOrView *EditorManager::currentSplitterOrView() const
{
    SplitterOrView *view = d->m_currentView;
    if (!view)
        view = d->m_currentEditor?
               d->m_splitter->findView(d->m_currentEditor):
               d->m_splitter->findFirstView();
    if (!view)
        return d->m_splitter;
    return view;
}

Core::Internal::SplitterOrView *EditorManager::topSplitterOrView() const
{
    return d->m_splitter;
}

Core::Internal::EditorView *EditorManager::currentEditorView() const
{
    return currentSplitterOrView()->view();
}

QList<IEditor *> EditorManager::editorsForFileName(const QString &filename) const
{
    QList<IEditor *> found;
    QString fixedname = DocumentManager::fixFileName(filename, DocumentManager::KeepLinks);
    foreach (IEditor *editor, openedEditors()) {
        if (fixedname == DocumentManager::fixFileName(editor->document()->fileName(), DocumentManager::KeepLinks))
            found << editor;
    }
    return found;
}

QList<IEditor *> EditorManager::editorsForDocument(IDocument *document) const
{
    QList<IEditor *> found;
    foreach (IEditor *editor, openedEditors()) {
        if (editor->document() == document)
            found << editor;
    }
    return found;
}

IEditor *EditorManager::currentEditor()
{
    return m_instance->d->m_currentEditor;
}

void EditorManager::emptyView(Core::Internal::EditorView *view)
{
    if (!view)
        return;

    QList<IEditor *> editors = view->editors();
    foreach (IEditor *editor, editors) {
        if (!d->m_editorModel->isDuplicate(editor)) {
            editors.removeAll(editor);
            view->removeEditor(editor);
            continue;
        }
        emit editorAboutToClose(editor);
        removeEditor(editor);
        view->removeEditor(editor);
    }
    emit editorsClosed(editors);
    foreach (IEditor *editor, editors) {
        delete editor;
    }
}

void EditorManager::closeView(Core::Internal::EditorView *view)
{
    if (!view)
        return;

    if (view == d->m_view) {
        if (IEditor *e = view->currentEditor())
            closeEditors(QList<IEditor *>() << e);
        return;
    }

    if (IEditor *e = view->currentEditor()) {
        /*
           when we are closing a view with an original editor which has
           duplicates, then make one of the duplicates the original.
           Otherwise the original would be kept around and the user might
           experience jumping to a missleading position within the file when
           visiting the file again. With the code below, the position within
           the file will be the position of the first duplicate which is still
           around.
        */
        if (!d->m_editorModel->isDuplicate(e)) {
            QList<IEditor *> duplicates = d->m_editorModel->duplicatesFor(e);
            if (!duplicates.isEmpty())
                d->m_editorModel->makeOriginal(duplicates.first());
        }
    }

    emptyView(view);

    SplitterOrView *splitterOrView = d->m_splitter->findView(view);
    Q_ASSERT(splitterOrView);
    Q_ASSERT(splitterOrView->view() == view);
    SplitterOrView *splitter = d->m_splitter->findSplitter(splitterOrView);
    Q_ASSERT(splitterOrView->hasEditors() == false);
    splitterOrView->hide();
    delete splitterOrView;

    splitter->unsplit();

    SplitterOrView *newCurrent = splitter->findFirstView();
    if (newCurrent) {
        if (IEditor *e = newCurrent->editor())
            activateEditor(newCurrent->view(), e);
        else
            setCurrentView(newCurrent);
    }
}

QList<IEditor*>
    EditorManager::editorsForDocuments(QList<IDocument*> documents) const
{
    const QList<IEditor *> editors = openedEditors();
    QSet<IEditor *> found;
    foreach (IDocument *document, documents) {
        foreach (IEditor *editor, editors) {
            if (editor->document() == document && !found.contains(editor))
                    found << editor;
        }
    }
    return found.toList();
}

QList<IDocument *> EditorManager::documentsForEditors(QList<IEditor *> editors) const
{
    QSet<IEditor *> handledEditors;
    QList<IDocument *> documents;
    foreach (IEditor *editor, editors) {
        if (!handledEditors.contains(editor)) {
            documents << editor->document();
            handledEditors.insert(editor);
        }
    }
    return documents;
}

bool EditorManager::closeAllEditors(bool askAboutModifiedEditors)
{
    d->m_editorModel->removeAllRestoredEditors();
    if (closeEditors(openedEditors(), askAboutModifiedEditors)) {
//        d->clearNavigationHistory();
        return true;
    }
    return false;
}

void EditorManager::closeOtherEditors(IEditor *editor)
{
    d->m_editorModel->removeAllRestoredEditors();
    QList<IEditor*> editors = openedEditors();
    editors.removeAll(editor);
    closeEditors(editors, true);
}

void EditorManager::closeOtherEditors()
{
    IEditor *current = currentEditor();
    QTC_ASSERT(current, return);
    closeOtherEditors(current);
}

// SLOT connected to action
void EditorManager::closeEditor()
{
    if (!d->m_currentEditor)
        return;
    addCurrentPositionToNavigationHistory();
    closeEditor(d->m_currentEditor);
}

static void assignAction(QAction *self, QAction *other)
{
    self->setText(other->text());
    self->setIcon(other->icon());
    self->setShortcut(other->shortcut());
    self->setEnabled(other->isEnabled());
}

void EditorManager::addSaveAndCloseEditorActions(QMenu *contextMenu, const QModelIndex &editorIndex)
{
    QTC_ASSERT(contextMenu, return);
    d->m_contextMenuEditorIndex = editorIndex;

    assignAction(d->m_saveCurrentEditorContextAction, ActionManager::command(Constants::SAVE)->action());
    assignAction(d->m_saveAsCurrentEditorContextAction, ActionManager::command(Constants::SAVEAS)->action());
    assignAction(d->m_revertToSavedCurrentEditorContextAction, ActionManager::command(Constants::REVERTTOSAVED)->action());

    IEditor *editor = d->m_contextMenuEditorIndex.data(Qt::UserRole).value<Core::IEditor*>();

    setupSaveActions(editor,
                     d->m_saveCurrentEditorContextAction,
                     d->m_saveAsCurrentEditorContextAction,
                     d->m_revertToSavedCurrentEditorContextAction);

    contextMenu->addAction(d->m_saveCurrentEditorContextAction);
    contextMenu->addAction(d->m_saveAsCurrentEditorContextAction);
    contextMenu->addAction(ActionManager::command(Constants::SAVEALL)->action());
    contextMenu->addAction(d->m_revertToSavedCurrentEditorContextAction);

    contextMenu->addSeparator();

    d->m_closeCurrentEditorContextAction->setText(editorIndex.isValid()
                                                    ? tr("Close \"%1\"").arg(editorIndex.data().toString())
                                                    : tr("Close Editor"));
    d->m_closeOtherEditorsContextAction->setText(editorIndex.isValid()
                                                   ? tr("Close All Except \"%1\"").arg(editorIndex.data().toString())
                                                   : tr("Close Other Editors"));
    d->m_closeCurrentEditorContextAction->setEnabled(editorIndex.isValid());
    d->m_closeOtherEditorsContextAction->setEnabled(editorIndex.isValid());
    d->m_closeAllEditorsContextAction->setEnabled(!openedEditors().isEmpty());
    contextMenu->addAction(d->m_closeCurrentEditorContextAction);
    contextMenu->addAction(d->m_closeAllEditorsContextAction);
    contextMenu->addAction(d->m_closeOtherEditorsContextAction);
}

void EditorManager::addNativeDirActions(QMenu *contextMenu, const QModelIndex &editorIndex)
{
    QTC_ASSERT(contextMenu, return);
    d->m_openGraphicalShellAction->setEnabled(editorIndex.isValid());
    d->m_openTerminalAction->setEnabled(editorIndex.isValid());
    contextMenu->addAction(d->m_openGraphicalShellAction);
    contextMenu->addAction(d->m_openTerminalAction);
}

void EditorManager::saveDocumentFromContextMenu()
{
    IEditor *editor = d->m_contextMenuEditorIndex.data(Qt::UserRole).value<Core::IEditor*>();
    if (editor)
        saveDocument(editor->document());
}

void EditorManager::saveDocumentAsFromContextMenu()
{
    IEditor *editor = d->m_contextMenuEditorIndex.data(Qt::UserRole).value<Core::IEditor*>();
    if (editor)
        saveDocumentAs(editor->document());
}

void EditorManager::revertToSavedFromContextMenu()
{
    IEditor *editor = d->m_contextMenuEditorIndex.data(Qt::UserRole).value<Core::IEditor*>();
    if (editor)
        revertToSaved(editor);
}

void EditorManager::closeEditorFromContextMenu()
{
    closeEditor(d->m_contextMenuEditorIndex);
}

void EditorManager::closeOtherEditorsFromContextMenu()
{
    closeOtherEditors(d->m_contextMenuEditorIndex.data(Qt::UserRole).value<IEditor *>());
}

void EditorManager::showInGraphicalShell()
{
    const QString path = d->m_contextMenuEditorIndex.data(Qt::UserRole + 1).toString();
    Core::FileUtils::showInGraphicalShell(ICore::mainWindow(), path);
}

void EditorManager::openTerminal()
{
    const QString path = QFileInfo(d->m_contextMenuEditorIndex.data(Qt::UserRole + 1).toString()).path();
    Core::FileUtils::openTerminal(path);
}

void EditorManager::closeEditor(Core::IEditor *editor)
{
    if (!editor)
        return;
    closeEditors(QList<IEditor *>() << editor);
}

void EditorManager::closeEditor(const QModelIndex &index)
{
    IEditor *editor = index.data(Qt::UserRole).value<Core::IEditor*>();
    if (editor)
        closeEditor(editor);
    else
        d->m_editorModel->removeEditor(index);
}

bool EditorManager::closeEditors(const QList<IEditor*> &editorsToClose, bool askAboutModifiedEditors)
{
    if (editorsToClose.isEmpty())
        return true;

    SplitterOrView *currentSplitterOrView = this->currentSplitterOrView();

    bool closingFailed = false;
    QList<IEditor*> acceptedEditors;
    //ask all core listeners to check whether the editor can be closed
    const QList<ICoreListener *> listeners =
        ExtensionSystem::PluginManager::getObjects<ICoreListener>();
    foreach (IEditor *editor, editorsToClose) {
        bool editorAccepted = true;
        if (d->m_editorModel->isDuplicate(editor))
            editor = d->m_editorModel->originalForDuplicate(editor);
        foreach (ICoreListener *listener, listeners) {
            if (!listener->editorAboutToClose(editor)) {
                editorAccepted = false;
                closingFailed = true;
                break;
            }
        }
        if (editorAccepted)
            acceptedEditors.append(editor);
    }
    if (acceptedEditors.isEmpty())
        return false;
    //ask whether to save modified files
    if (askAboutModifiedEditors) {
        bool cancelled = false;
        QList<IDocument*> list = DocumentManager::saveModifiedDocuments(documentsForEditors(acceptedEditors), &cancelled);
        if (cancelled)
            return false;
        if (!list.isEmpty()) {
            closingFailed = true;
            QSet<IEditor*> skipSet = editorsForDocuments(list).toSet();
            acceptedEditors = acceptedEditors.toSet().subtract(skipSet).toList();
        }
    }
    if (acceptedEditors.isEmpty())
        return false;

    // add duplicates
    QList<IEditor *> duplicates;
    foreach (IEditor *editor, acceptedEditors)
        duplicates += d->m_editorModel->duplicatesFor(editor);
    acceptedEditors += duplicates;

    QList<EditorView*> closedViews;

    // remove the editors
    foreach (IEditor *editor, acceptedEditors) {
        emit editorAboutToClose(editor);
        if (!editor->document()->fileName().isEmpty()
                && !editor->isTemporary()) {
            QByteArray state = editor->saveState();
            if (!state.isEmpty())
                d->m_editorStates.insert(editor->document()->fileName(), QVariant(state));
        }

        removeEditor(editor);
        if (SplitterOrView *view = d->m_splitter->findView(editor)) {
            if (editor == view->view()->currentEditor())
                closedViews += view->view();
            view->view()->removeEditor(editor);
        }
    }

    foreach (EditorView *view, closedViews) {
        IEditor *newCurrent = view->currentEditor();
        if (!newCurrent)
            newCurrent = pickUnusedEditor();
        if (newCurrent) {
            activateEditor(view, newCurrent, NoActivate);
        } else {
            QModelIndex idx = d->m_editorModel->firstRestoredEditor();
            if (idx.isValid()) {
                activateEditorForIndex(view, idx, NoActivate);
            } else {
                const QList<IEditor *> editors = d->m_editorModel->editors();
                if (!editors.isEmpty())
                    activateEditor(view, editors.last(), NoActivate);
            }
        }
    }

    emit editorsClosed(acceptedEditors);

    foreach (IEditor *editor, acceptedEditors) {
        delete editor;
    }

    if (currentSplitterOrView) {
        if (IEditor *editor = currentSplitterOrView->editor())
            activateEditor(currentSplitterOrView->view(), editor);
    }

    if (!currentEditor()) {
        emit currentEditorChanged(0);
        updateActions();
        updateWindowTitle();
    }

    return !closingFailed;
}

void EditorManager::closeDuplicate(Core::IEditor *editor)
{

    IEditor *original = editor;
    if (d->m_editorModel->isDuplicate(editor))
        original= d->m_editorModel->originalForDuplicate(editor);
    QList<IEditor *> duplicates = d->m_editorModel->duplicatesFor(original);

    if (duplicates.isEmpty()) {
        closeEditor(editor);
        return;
    }

    if (original== editor)
        d->m_editorModel->makeOriginal(duplicates.first());

    SplitterOrView *currentSplitterOrView = this->currentSplitterOrView();

    emit editorAboutToClose(editor);

    if (d->m_splitter->findView(editor)) {
        EditorView *view = d->m_splitter->findView(editor)->view();
        removeEditor(editor);
        view->removeEditor(editor);

        IEditor *newCurrent = view->currentEditor();
        if (!newCurrent)
            newCurrent = pickUnusedEditor();
        if (newCurrent) {
            activateEditor(view, newCurrent, NoActivate);
        } else {
            QModelIndex idx = d->m_editorModel->firstRestoredEditor();
            if (idx.isValid())
                activateEditorForIndex(view, idx, NoActivate);
        }
    }

    emit editorsClosed(QList<IEditor*>() << editor);
    delete editor;
    if (currentSplitterOrView) {
        if (IEditor *currentEditor = currentSplitterOrView->editor())
            activateEditor(currentSplitterOrView->view(), currentEditor);
    }
}

Core::IEditor *EditorManager::pickUnusedEditor() const
{
    foreach (IEditor *editor, openedEditors()) {
        SplitterOrView *view = d->m_splitter->findView(editor);
        if (!view || view->editor() != editor)
            return editor;
    }
    return 0;
}

void EditorManager::activateEditorForIndex(const QModelIndex &index, OpenEditorFlags flags)
{
    activateEditorForIndex(currentEditorView(), index, flags);
}

void EditorManager::activateEditorForIndex(Internal::EditorView *view, const QModelIndex &index, OpenEditorFlags flags)
{
    Q_ASSERT(view);
    IEditor *editor = index.data(Qt::UserRole).value<IEditor*>();
    if (editor)  {
        activateEditor(view, editor, flags);
        return;
    }

    QString fileName = index.data(Qt::UserRole + 1).toString();
    Core::Id id = index.data(Qt::UserRole + 2).value<Core::Id>();
    if (!openEditor(view, fileName, id, flags))
        d->m_editorModel->removeEditor(index);
}

Core::IEditor *EditorManager::placeEditor(Core::Internal::EditorView *view, Core::IEditor *editor)
{
    Q_ASSERT(view && editor);

    if (view->currentEditor() && view->currentEditor()->document() == editor->document())
        editor = view->currentEditor();

    if (!view->hasEditor(editor)) {
        bool duplicateSupported = editor->duplicateSupported();
        if (SplitterOrView *sourceView = d->m_splitter->findView(editor)) {
            if (editor != sourceView->editor() || !duplicateSupported) {
                sourceView->view()->removeEditor(editor);
                view->addEditor(editor);
                view->setCurrentEditor(editor);
                if (!sourceView->editor()) {
                    if (IEditor *replacement = pickUnusedEditor())
                        sourceView->view()->addEditor(replacement);
                }
                return editor;
            } else if (duplicateSupported) {
                editor = duplicateEditor(editor);
                Q_ASSERT(editor);
                d->m_editorModel->makeOriginal(editor);
            }
        }
        view->addEditor(editor);
    }
    return editor;
}

void EditorManager::activateEditor(Core::IEditor *editor, OpenEditorFlags flags)
{
    SplitterOrView *splitterOrView = m_instance->d->m_splitter->findView(editor);
    EditorView *view = (splitterOrView ? splitterOrView->view() : 0);
    // TODO an IEditor doesn't have to belong to a view, which makes this method a bit funny
    if (!view)
        view = m_instance->currentEditorView();
    m_instance->activateEditor(view, editor, flags);
}

Core::IEditor *EditorManager::activateEditor(Core::Internal::EditorView *view, Core::IEditor *editor, OpenEditorFlags flags)
{
    Q_ASSERT(view);

    if (!editor) {
        if (!d->m_currentEditor)
            setCurrentEditor(0, (flags & IgnoreNavigationHistory));
        return 0;
    }

    editor = placeEditor(view, editor);

    if (!(flags & NoActivate)) {
        setCurrentEditor(editor, (flags & IgnoreNavigationHistory));
        if (flags & ModeSwitch)
            switchToPreferedMode();
        if (isVisible())
            editor->widget()->setFocus();
    }
    return editor;
}

Core::IEditor *EditorManager::activateEditorForDocument(Core::Internal::EditorView *view, Core::IDocument *document, OpenEditorFlags flags)
{
    Q_ASSERT(view);
    const QList<IEditor*> editors = editorsForDocument(document);
    if (editors.isEmpty())
        return 0;

    activateEditor(view, editors.first(), flags);
    return editors.first();
}

/* For something that has a 'QStringList mimeTypes' (IEditorFactory
 * or IExternalEditor), find the one best matching the mimetype passed in.
 *  Recurse over the parent classes of the mimetype to find them. */
template <class EditorFactoryLike>
static void mimeTypeFactoryRecursion(const MimeDatabase *db,
                                     const MimeType &mimeType,
                                     const QList<EditorFactoryLike*> &allFactories,
                                     bool firstMatchOnly,
                                     QList<EditorFactoryLike*> *list)
{
    typedef typename QList<EditorFactoryLike*>::const_iterator EditorFactoryLikeListConstIterator;
    // Loop factories to find type
    const QString type = mimeType.type();
    const EditorFactoryLikeListConstIterator fcend = allFactories.constEnd();
    for (EditorFactoryLikeListConstIterator fit = allFactories.constBegin(); fit != fcend; ++fit) {
        // Exclude duplicates when recursing over xml or C++ -> C -> text.
        EditorFactoryLike *factory = *fit;
        if (!list->contains(factory) && factory->mimeTypes().contains(type)) {
            list->push_back(*fit);
            if (firstMatchOnly)
                return;
        }
    }
    // Any parent mime type classes? -> recurse
    QStringList parentTypes = mimeType.subClassesOf();
    if (parentTypes.empty())
        return;
    const QStringList::const_iterator pcend = parentTypes .constEnd();
    for (QStringList::const_iterator pit = parentTypes .constBegin(); pit != pcend; ++pit) {
        if (const MimeType parent = db->findByType(*pit))
            mimeTypeFactoryRecursion(db, parent, allFactories, firstMatchOnly, list);
    }
}

EditorManager::EditorFactoryList
    EditorManager::editorFactories(const MimeType &mimeType, bool bestMatchOnly)
{
    EditorFactoryList rc;
    const EditorFactoryList allFactories = ExtensionSystem::PluginManager::getObjects<IEditorFactory>();
    mimeTypeFactoryRecursion(ICore::mimeDatabase(), mimeType, allFactories, bestMatchOnly, &rc);
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << mimeType.type() << " returns " << rc;
    return rc;
}

EditorManager::ExternalEditorList
        EditorManager::externalEditors(const MimeType &mimeType, bool bestMatchOnly)
{
    ExternalEditorList rc;
    const ExternalEditorList allEditors = ExtensionSystem::PluginManager::getObjects<IExternalEditor>();
    mimeTypeFactoryRecursion(ICore::mimeDatabase(), mimeType, allEditors, bestMatchOnly, &rc);
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << mimeType.type() << " returns " << rc;
    return rc;
}

/* For something that has a 'QString id' (IEditorFactory
 * or IExternalEditor), find the one matching a id. */
template <class EditorFactoryLike>
EditorFactoryLike *findById(const Core::Id &id)
{
    const QList<EditorFactoryLike *> factories = ExtensionSystem::PluginManager::getObjects<EditorFactoryLike>();
    foreach (EditorFactoryLike *efl, factories)
        if (id == efl->id())
            return efl;
    return 0;
}

IEditor *EditorManager::createEditor(const Id &editorId, const QString &fileName)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << editorId.name() << fileName;

    EditorFactoryList factories;
    if (!editorId.isValid()) {
        const QFileInfo fileInfo(fileName);
        // Find by mime type
        MimeType mimeType = ICore::mimeDatabase()->findByFile(fileInfo);
        if (!mimeType) {
            qWarning("%s unable to determine mime type of %s/%s. Falling back to text/plain",
                     Q_FUNC_INFO, fileName.toUtf8().constData(), editorId.name().constData());
            mimeType = ICore::mimeDatabase()->findByType(QLatin1String("text/plain"));
        }
        // open text files > 48 MB in binary editor
        if (fileInfo.size() >  maxTextFileSize() && mimeType.type().startsWith(QLatin1String("text")))
            mimeType = ICore::mimeDatabase()->findByType(QLatin1String("application/octet-stream"));
        factories = editorFactories(mimeType, true);
    } else {
        // Find by editor id
        if (IEditorFactory *factory = findById<IEditorFactory>(editorId))
            factories.push_back(factory);
    }
    if (factories.empty()) {
        qWarning("%s: unable to find an editor factory for the file '%s', editor Id '%s'.",
                 Q_FUNC_INFO, fileName.toUtf8().constData(), editorId.name().constData());
        return 0;
    }

    IEditor *editor = factories.front()->createEditor(m_instance);
    if (editor)
        connect(editor, SIGNAL(changed()), m_instance, SLOT(handleEditorStateChange()));
    if (editor)
        emit m_instance->editorCreated(editor, fileName);
    return editor;
}

void EditorManager::addEditor(IEditor *editor, bool isDuplicate)
{
    if (!editor)
        return;
    ICore::addContextObject(editor);

    d->m_editorModel->addEditor(editor, isDuplicate);
    if (!isDuplicate) {
        const bool isTemporary = editor->isTemporary();
        const bool addWatcher = !isTemporary;
        DocumentManager::addDocument(editor->document(), addWatcher);
        if (!isTemporary)
            DocumentManager::addToRecentFiles(editor->document()->fileName(), editor->id());
    }
    emit editorOpened(editor);
}

// Run the OpenWithDialog and return the editor id
// selected by the user.
Core::Id EditorManager::getOpenWithEditorId(const QString &fileName,
                                           bool *isExternalEditor) const
{
    // Collect editors that can open the file
    MimeType mt = ICore::mimeDatabase()->findByFile(fileName);
    //Unable to determine mime type of fileName. Falling back to text/plain",
    if (!mt)
        mt = ICore::mimeDatabase()->findByType(QLatin1String("text/plain"));
    QList<Id> allEditorIds;
    QStringList allEditorDisplayNames;
    QList<Id> externalEditorIds;
    // Built-in
    const EditorFactoryList editors = editorFactories(mt, false);
    const int size = editors.size();
    for (int i = 0; i < size; i++) {
        allEditorIds.push_back(editors.at(i)->id());
        allEditorDisplayNames.push_back(editors.at(i)->displayName());
    }
    // External editors
    const ExternalEditorList exEditors = externalEditors(mt, false);
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

IEditor *EditorManager::openEditor(const QString &fileName, const Id &editorId,
                                   OpenEditorFlags flags, bool *newEditor)
{
    return m_instance->openEditor(m_instance->currentEditorView(), fileName, editorId, flags, newEditor);
}

static int extractLineNumber(QString *fileName)
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
            fileName->truncate(i);
            return result;
        }
    }
    return -1;
}

// Extract line number suffix. Return the suffix (e.g. ":132") and truncates the filename accordingly.
QString EditorManager::splitLineNumber(QString *fileName)
{
    int i = fileName->length() - 1;
    for (; i >= 0; --i) {
        if (!fileName->at(i).isNumber())
            break;
    }
    if (i == -1)
        return QString();
    const QChar c = fileName->at(i);
    if (c == QLatin1Char(':') || c == QLatin1Char('+')) {
        const QString result = fileName->mid(i + 1);
        bool ok;
        result.toInt(&ok);
        if (result.isEmpty() || ok) {
            fileName->truncate(i);
            return QString(c) + result;
        }
    }
    return QString();
}

static QString autoSaveName(const QString &fileName)
{
    return fileName + QLatin1String(".autosave");
}

IEditor *EditorManager::openEditor(Core::Internal::EditorView *view, const QString &fileName,
                        const Id &editorId, OpenEditorFlags flags, bool *newEditor)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << fileName << editorId.name();

    QString fn = fileName;
    QFileInfo fi(fn);
    int lineNumber = -1;
    if ((flags & EditorManager::CanContainLineNumber) && !fi.exists()) {
        lineNumber = extractLineNumber(&fn);
        if (lineNumber != -1)
            fi.setFile(fn);
    }

    if (fn.isEmpty())
        return 0;

    if (newEditor)
        *newEditor = false;

    const QList<IEditor *> editors = editorsForFileName(fn);
    if (!editors.isEmpty()) {
        IEditor *editor = editors.first();
        if (flags & EditorManager::CanContainLineNumber)
            editor->gotoLine(lineNumber, -1);
        return activateEditor(view, editor, flags);
    }

    QString realFn = autoSaveName(fn);
    QFileInfo rfi(realFn);
    if (!fi.exists() || !rfi.exists() || fi.lastModified() >= rfi.lastModified()) {
        QFile::remove(realFn);
        realFn = fn;
    }

    IEditor *editor = createEditor(editorId, fn);
    // If we could not open the file in the requested editor, fall
    // back to the default editor:
    if (!editor)
        editor = createEditor(Id(), fn);
    if (!editor) // Internal error
        return 0;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QString errorString;
    if (!editor->open(&errorString, fn, realFn)) {
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(ICore::mainWindow(), tr("File Error"), errorString);
        delete editor;
        return 0;
    }
    if (realFn != fn)
        editor->document()->setRestoredFrom(realFn);
    addEditor(editor);

    if (newEditor)
        *newEditor = true;

    IEditor *result = activateEditor(view, editor, flags);
    if (editor == result)
        restoreEditorState(editor);

    if (flags & EditorManager::CanContainLineNumber)
        editor->gotoLine(lineNumber, -1);

    QApplication::restoreOverrideCursor();
    return result;
}

bool EditorManager::openExternalEditor(const QString &fileName, const Core::Id &editorId)
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

QStringList EditorManager::getOpenFileNames() const
{
    QString selectedFilter;
    const QString &fileFilters = ICore::mimeDatabase()->allFiltersString(&selectedFilter);
    return DocumentManager::getOpenFileNames(fileFilters, QString(), &selectedFilter);
}


/// Empty mode == figure out the correct mode from the editor
/// forcePrefered = true, switch to the mode even if the editor is visible in another mode
/// forcePrefered = false, only switch if it is not visible
void EditorManager::switchToPreferedMode()
{
    Id preferedMode;
    // Figure out preferred mode for editor
    if (d->m_currentEditor)
        preferedMode = d->m_currentEditor->preferredModeType();

    if (!preferedMode.isValid())
        preferedMode = Id(Constants::MODE_EDIT_TYPE);

    ModeManager::activateModeType(preferedMode);
}

IEditor *EditorManager::openEditorWithContents(const Id &editorId,
                                        QString *titlePattern,
                                        const QString &contents)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << editorId.name() << titlePattern << contents;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QString title;
    if (titlePattern) {
        const QChar dollar = QLatin1Char('$');

        QString base = *titlePattern;
        if (base.isEmpty())
            base = QLatin1String("unnamed$");
        if (base.contains(dollar)) {
            int i = 1;
            QSet<QString> docnames;
            foreach (IEditor *editor, m_instance->openedEditors()) {
                QString name = editor->document()->fileName();
                if (name.isEmpty())
                    name = editor->displayName();
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

    IEditor *edt = createEditor(editorId, title);
    if (!edt) {
        QApplication::restoreOverrideCursor();
        return 0;
    }

    if (!edt->createNew(contents)) {
        QApplication::restoreOverrideCursor();
        delete edt;
        edt = 0;
        return 0;
    }

    if (title.isEmpty())
        title = edt->displayName();

    edt->setDisplayName(title);
    m_instance->addEditor(edt);
    QApplication::restoreOverrideCursor();
    return edt;
}

bool EditorManager::hasEditor(const QString &fileName) const
{
    return !editorsForFileName(fileName).isEmpty();
}

void EditorManager::restoreEditorState(IEditor *editor)
{
    QTC_ASSERT(editor, return);
    QString fileName = editor->document()->fileName();
    editor->restoreState(d->m_editorStates.value(fileName).toByteArray());
}

bool EditorManager::saveEditor(IEditor *editor)
{
    return saveDocument(editor->document());
}

bool EditorManager::saveDocument(IDocument *documentParam)
{
    IDocument *document = documentParam;
    if (!document && currentEditor())
        document = currentEditor()->document();
    if (!document)
        return false;

    document->checkPermissions();

    const QString &fileName = document->fileName();

    if (fileName.isEmpty())
        return saveDocumentAs(document);

    bool success = false;
    bool isReadOnly;

    // try saving, no matter what isReadOnly tells us
    success = DocumentManager::saveDocument(document, QString(), &isReadOnly);

    if (!success && isReadOnly) {
        MakeWritableResult answer =
                makeFileWritable(document);
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

void EditorManager::autoSave()
{
    QStringList errors;
    // FIXME: the saving should be staggered
    foreach (IEditor *editor, openedEditors()) {
        IDocument *document = editor->document();
        if (!document->isModified() || !document->shouldAutoSave())
            continue;
        if (document->fileName().isEmpty()) // FIXME: save them to a dedicated directory
            continue;
        QString errorString;
        if (!document->autoSave(&errorString, autoSaveName(document->fileName())))
            errors << errorString;
    }
    if (!errors.isEmpty())
        QMessageBox::critical(ICore::mainWindow(), tr("File Error"),
                              errors.join(QLatin1String("\n")));

    // Also save settings while accessing the disk anyway:
    ICore::saveSettings();
}

MakeWritableResult EditorManager::makeFileWritable(IDocument *document)
{
    if (!document)
        return Failed;
    QString directory = QFileInfo(document->fileName()).absolutePath();
    IVersionControl *versionControl = ICore::vcsManager()->findVersionControlForDirectory(directory);
    const QString &fileName = document->fileName();

    switch (DocumentManager::promptReadOnlyFile(fileName, versionControl, ICore::mainWindow(), document->isSaveAsAllowed())) {
    case DocumentManager::RO_OpenVCS:
        if (!versionControl->vcsOpen(fileName)) {
            QMessageBox::warning(ICore::mainWindow(), tr("Cannot Open File"), tr("Cannot open the file for editing with SCC."));
            return Failed;
        }
        document->checkPermissions();
        return OpenedWithVersionControl;
    case DocumentManager::RO_MakeWriteable: {
        const bool permsOk = QFile::setPermissions(fileName, QFile::permissions(fileName) | QFile::WriteUser);
        if (!permsOk) {
            QMessageBox::warning(ICore::mainWindow(), tr("Cannot Set Permissions"),  tr("Cannot set permissions to writable."));
            return Failed;
        }
    }
        document->checkPermissions();
        return MadeWritable;
    case DocumentManager::RO_SaveAs :
        return saveDocumentAs(document) ? SavedAs : Failed;
    case DocumentManager::RO_Cancel:
        break;
    }
    return Failed;
}

bool EditorManager::saveDocumentAs(IDocument *documentParam)
{
    IDocument *document = documentParam;
    if (!document && currentEditor())
        document = currentEditor()->document();
    if (!document)
        return false;

    const QString &filter = ICore::mimeDatabase()->allFiltersString();
    QString selectedFilter =
        ICore::mimeDatabase()->findByFile(QFileInfo(document->fileName())).filterString();
    const QString &absoluteFilePath =
        DocumentManager::getSaveAsFileName(document, filter, &selectedFilter);

    if (absoluteFilePath.isEmpty())
        return false;

    if (absoluteFilePath != document->fileName()) {
        // close existing editors for the new file name
        const QList<IEditor *> existList = editorsForFileName(absoluteFilePath);
        if (!existList.isEmpty())
            closeEditors(existList, false);
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

/* Adds the file name to the recent files if there is at least one non-temporary editor for it */
void EditorManager::addDocumentToRecentFiles(IDocument *document)
{
    bool isTemporary = true;
    Id editorId;
    QList<IEditor *> editors = editorsForDocument(document);
    foreach (IEditor *editor, editors) {
        if (!editor->isTemporary()) {
            editorId = editor->id();
            isTemporary = false;
            break;
        }
    }
    if (!isTemporary)
        DocumentManager::addToRecentFiles(document->fileName(), editorId);
}

void EditorManager::gotoNextDocHistory()
{
    OpenEditorsWindow *dialog = windowPopup();
    if (dialog->isVisible()) {
        dialog->selectNextEditor();
    } else {
        EditorView *view = currentEditorView();
        dialog->setEditors(d->m_view, view, d->m_editorModel);
        dialog->selectNextEditor();
        showPopupOrSelectDocument();
    }
}

void EditorManager::gotoPreviousDocHistory()
{
    OpenEditorsWindow *dialog = windowPopup();
    if (dialog->isVisible()) {
        dialog->selectPreviousEditor();
    } else {
        EditorView *view = currentEditorView();
        dialog->setEditors(d->m_view, view, d->m_editorModel);
        dialog->selectPreviousEditor();
        showPopupOrSelectDocument();
    }
}

void EditorManager::makeCurrentEditorWritable()
{
    if (IEditor* curEditor = currentEditor())
        makeFileWritable(curEditor->document());
}

void EditorManager::vcsOpenCurrentEditor()
{
    IEditor *curEditor = currentEditor();
    if (!curEditor)
        return;

    const QString directory = QFileInfo(curEditor->document()->fileName()).absolutePath();
    IVersionControl *versionControl = ICore::vcsManager()->findVersionControlForDirectory(directory);
    if (!versionControl || !versionControl->supportsOperation(IVersionControl::OpenOperation))
        return;

    if (!versionControl->vcsOpen(curEditor->document()->fileName())) {
        QMessageBox::warning(ICore::mainWindow(), tr("Cannot Open File"),
                             tr("Cannot open the file for editing with VCS."));
    }
}

void EditorManager::updateWindowTitle()
{
    QString windowTitle = tr("Qt Creator");
    const QString dashSep = QLatin1String(" - ");
    if (!d->m_titleAddition.isEmpty())
        windowTitle.prepend(d->m_titleAddition + dashSep);
    IEditor *curEditor = currentEditor();
    if (curEditor) {
        QString editorName = curEditor->displayName();
        if (!editorName.isEmpty())
            windowTitle.prepend(editorName + dashSep);
        QString filePath = QFileInfo(curEditor->document()->fileName()).absoluteFilePath();
        if (!filePath.isEmpty())
            ICore::mainWindow()->setWindowFilePath(filePath);
    } else {
        ICore::mainWindow()->setWindowFilePath(QString());
    }
    ICore::mainWindow()->setWindowTitle(windowTitle);
}

void EditorManager::handleEditorStateChange()
{
    updateActions();
    IEditor *theEditor = qobject_cast<IEditor *>(sender());
    if (!theEditor->document()->isModified())
        theEditor->document()->removeAutoSaveFile();
    IEditor *currEditor = currentEditor();
    if (theEditor == currEditor) {
        updateWindowTitle();
        emit currentEditorStateChanged(currEditor);
    }
}

void EditorManager::updateMakeWritableWarning()
{
    IEditor *curEditor = currentEditor();
    bool ww = curEditor->document()->isModified() && curEditor->document()->isFileReadOnly();
    if (ww != curEditor->document()->hasWriteWarning()) {
        curEditor->document()->setWriteWarning(ww);

        // Do this after setWriteWarning so we don't re-evaluate this part even
        // if we do not really show a warning.
        bool promptVCS = false;
        const QString directory = QFileInfo(curEditor->document()->fileName()).absolutePath();
        IVersionControl *versionControl = ICore::vcsManager()->findVersionControlForDirectory(directory);
        if (versionControl && versionControl->supportsOperation(IVersionControl::OpenOperation)) {
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
                info.setCustomButtonInfo(tr("Open"), this, SLOT(vcsOpenCurrentEditor()));
                curEditor->document()->infoBar()->addInfo(info);
            } else {
                InfoBarEntry info(Id(kMakeWritableWarning),
                                  tr("<b>Warning:</b> You are changing a read-only file."));
                info.setCustomButtonInfo(tr("Make Writable"), this, SLOT(makeCurrentEditorWritable()));
                curEditor->document()->infoBar()->addInfo(info);
            }
        } else {
            curEditor->document()->infoBar()->removeInfo(Id(kMakeWritableWarning));
        }
    }
}

QString EditorManager::fileNameForEditor(IEditor *editor)
{
    QString fileName;

    if (editor) {
        if (!editor->document()->fileName().isEmpty()) {
            QFileInfo fileInfo(editor->document()->fileName());
            fileName = fileInfo.fileName();
        } else {
            fileName = editor->displayName();
        }
    }
    return fileName;
}

void EditorManager::setupSaveActions(IEditor *editor, QAction *saveAction, QAction *saveAsAction, QAction *revertToSavedAction)
{
    saveAction->setEnabled(editor != 0 && editor->document()->isModified());
    saveAsAction->setEnabled(editor != 0 && editor->document()->isSaveAsAllowed());
    revertToSavedAction->setEnabled(editor != 0
                                    && !editor->document()->fileName().isEmpty() && editor->document()->isModified());

    const QString fileName = fileNameForEditor(editor);
    QString quotedName;

    if (!fileName.isEmpty())
        quotedName = QLatin1Char('"') + fileName + QLatin1Char('"');
    if (!quotedName.isEmpty()) {
        saveAction->setText(tr("&Save %1").arg(quotedName));
        saveAsAction->setText(tr("Save %1 &As...").arg(quotedName));
        revertToSavedAction->setText(tr("Revert %1 to Saved").arg(quotedName));
    }
}

void EditorManager::updateActions()
{
    IEditor *curEditor = currentEditor();
    const QString fileName = fileNameForEditor(curEditor);
    int openedCount = openedEditors().count() + d->m_editorModel->restoredEditors().count();

    if (curEditor) {
        if (HostOsInfo::isMacHost())
            window()->setWindowModified(curEditor->document()->isModified());
        updateMakeWritableWarning();
    } else /* curEditor */ if (HostOsInfo::isMacHost()) {
        window()->setWindowModified(false);
    }

    setCloseSplitEnabled(d->m_splitter, d->m_splitter->isSplitter());

    QString quotedName;
    if (!fileName.isEmpty())
        quotedName = QLatin1Char('"') + fileName + QLatin1Char('"');
    setupSaveActions(curEditor, d->m_saveAction, d->m_saveAsAction, d->m_revertToSavedAction);

    d->m_closeCurrentEditorAction->setEnabled(curEditor != 0);
    d->m_closeCurrentEditorAction->setText(tr("Close %1").arg(quotedName));
    d->m_closeAllEditorsAction->setEnabled(openedCount > 0);
    d->m_closeOtherEditorsAction->setEnabled(openedCount > 1);
    d->m_closeOtherEditorsAction->setText((openedCount > 1 ? tr("Close All Except %1").arg(quotedName) : tr("Close Others")));

    d->m_gotoNextDocHistoryAction->setEnabled(d->m_editorModel->rowCount() != 0);
    d->m_gotoPreviousDocHistoryAction->setEnabled(d->m_editorModel->rowCount() != 0);
    EditorView *view  = currentEditorView();
    d->m_goBackAction->setEnabled(view ? view->canGoBack() : false);
    d->m_goForwardAction->setEnabled(view ? view->canGoForward() : false);

    bool hasSplitter = d->m_splitter->isSplitter();
    d->m_removeCurrentSplitAction->setEnabled(hasSplitter);
    d->m_removeAllSplitsAction->setEnabled(hasSplitter);
    d->m_gotoOtherSplitAction->setEnabled(hasSplitter);
}

void EditorManager::setCloseSplitEnabled(SplitterOrView *splitterOrView, bool enable)
{
    if (splitterOrView->isView())
        splitterOrView->view()->setCloseSplitEnabled(enable);
    QSplitter *splitter = splitterOrView->splitter();
    if (splitter) {
        for (int i = 0; i < splitter->count(); ++i) {
            if (SplitterOrView *subSplitterOrView = qobject_cast<SplitterOrView*>(splitter->widget(i)))
                setCloseSplitEnabled(subSplitterOrView, enable);
        }
    }
}

bool EditorManager::hasSplitter() const
{
    return d->m_splitter->isSplitter();
}

QList<IEditor*> EditorManager::visibleEditors() const
{
    QList<IEditor *> editors;
    if (d->m_splitter->isSplitter()) {
        SplitterOrView *firstView = d->m_splitter->findFirstView();
        SplitterOrView *view = firstView;
        if (view) {
            do {
                if (view->editor())
                    editors.append(view->editor());
                view = d->m_splitter->findNextView(view);
            } while (view && view != firstView);
        }
    } else {
        if (d->m_splitter->editor())
            editors.append(d->m_splitter->editor());
    }
    return editors;
}

QList<IEditor*> EditorManager::openedEditors() const
{
    return d->m_editorModel->editors();
}

OpenEditorsModel *EditorManager::openedEditorsModel() const
{
    return d->m_editorModel;
}

void EditorManager::addCurrentPositionToNavigationHistory(IEditor *editor, const QByteArray &saveState)
{
    currentEditorView()->addCurrentPositionToNavigationHistory(editor, saveState);
    updateActions();
}

void EditorManager::cutForwardNavigationHistory()
{
    currentEditorView()->cutForwardNavigationHistory();
    updateActions();
}

void EditorManager::goBackInNavigationHistory()
{
    currentEditorView()->goBackInNavigationHistory();
    updateActions();
    return;
}

void EditorManager::goForwardInNavigationHistory()
{
    currentEditorView()->goForwardInNavigationHistory();
    updateActions();
}

OpenEditorsWindow *EditorManager::windowPopup() const
{
    return d->m_windowPopup;
}

void EditorManager::showPopupOrSelectDocument() const
{
    if (QApplication::keyboardModifiers() == Qt::NoModifier) {
        windowPopup()->selectAndHide();
    } else {
        // EditorManager is invisible when invoked from Design Mode.
        const QPoint p = isVisible() ?
                         mapToGlobal(QPoint(0, 0)) :
                         ICore::mainWindow()->mapToGlobal(QPoint(0, 0));
        windowPopup()->move((width()-d->m_windowPopup->width())/2 + p.x(),
                            (height()-d->m_windowPopup->height())/2 + p.y());
        windowPopup()->setVisible(true);
    }
}

// Save state of all non-teporary editors.
QByteArray EditorManager::saveState() const
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);

    stream << QByteArray("EditorManagerV4");

    QList<IEditor *> editors = openedEditors();
    foreach (IEditor *editor, editors) {
        if (!editor->document()->fileName().isEmpty()
                && !editor->isTemporary()) {
            QByteArray state = editor->saveState();
            if (!state.isEmpty())
                d->m_editorStates.insert(editor->document()->fileName(), QVariant(state));
        }
    }

    stream << d->m_editorStates;

    QList<OpenEditorsModel::Entry> entries = d->m_editorModel->entries();
    int entriesCount = 0;
    foreach (const OpenEditorsModel::Entry &entry, entries) {
        // The editor may be 0 if it was not loaded yet: In that case it is not temporary
        if (!entry.editor || !entry.editor->isTemporary())
            ++entriesCount;
    }

    stream << entriesCount;

    foreach (const OpenEditorsModel::Entry &entry, entries) {
        if (!entry.editor || !entry.editor->isTemporary())
            stream << entry.fileName() << entry.displayName() << entry.id().name();
    }

    stream << d->m_splitter->saveState();

    return bytes;
}

bool EditorManager::restoreState(const QByteArray &state)
{
    closeAllEditors(true);
    removeAllSplits();
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
        QByteArray id;
        stream >> id;

        if (!fileName.isEmpty() && !displayName.isEmpty()) {
            QFileInfo fi(fileName);
            if (!fi.exists())
                continue;
            QFileInfo rfi(autoSaveName(fileName));
            if (rfi.exists() && fi.lastModified() < rfi.lastModified())
                openEditor(fileName, Id::fromName(id));
            else
                d->m_editorModel->addRestoredEditor(fileName, displayName, Id::fromName(id));
        }
    }

    QByteArray splitterstates;
    stream >> splitterstates;
    d->m_splitter->restoreState(splitterstates);

    // splitting and stuff results in focus trouble, that's why we set the focus again after restoration
    if (d->m_currentEditor) {
        d->m_currentEditor->widget()->setFocus();
    } else if (Core::Internal::SplitterOrView *view = currentSplitterOrView()) {
        if (IEditor *e = view->editor())
            e->widget()->setFocus();
        else if (view->view())
            view->view()->setFocus();
    }

    QApplication::restoreOverrideCursor();

    return true;
}

static const char documentStatesKey[] = "EditorManager/DocumentStates";
static const char reloadBehaviorKey[] = "EditorManager/ReloadBehavior";
static const char autoSaveEnabledKey[] = "EditorManager/AutoSaveEnabled";
static const char autoSaveIntervalKey[] = "EditorManager/AutoSaveInterval";

void EditorManager::saveSettings()
{
    SettingsDatabase *settings = ICore::settingsDatabase();
    settings->setValue(QLatin1String(documentStatesKey), d->m_editorStates);
    settings->setValue(QLatin1String(reloadBehaviorKey), d->m_reloadSetting);
    settings->setValue(QLatin1String(autoSaveEnabledKey), d->m_autoSaveEnabled);
    settings->setValue(QLatin1String(autoSaveIntervalKey), d->m_autoSaveInterval);
}

void EditorManager::readSettings()
{
    // Backward compatibility to old locations for these settings
    QSettings *qs = ICore::settings();
    if (qs->contains(QLatin1String(documentStatesKey))) {
        d->m_editorStates = qs->value(QLatin1String(documentStatesKey))
            .value<QMap<QString, QVariant> >();
        qs->remove(QLatin1String(documentStatesKey));
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


void EditorManager::revertToSaved()
{
    revertToSaved(currentEditor());
}

void EditorManager::revertToSaved(Core::IEditor *editor)
{
    if (!editor)
        return;
    const QString fileName =  editor->document()->fileName();
    if (fileName.isEmpty())
        return;
    if (editor->document()->isModified()) {
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
    if (!editor->document()->reload(&errorString, IDocument::FlagReload, IDocument::TypeContents))
        QMessageBox::critical(ICore::mainWindow(), tr("File Error"), errorString);
}

void EditorManager::showEditorStatusBar(const QString &id,
                                      const QString &infoText,
                                      const QString &buttonText,
                                      QObject *object, const char *member)
{

    currentEditorView()->showEditorStatusBar(id, infoText, buttonText, object, member);
}

void EditorManager::hideEditorStatusBar(const QString &id)
{
    currentEditorView()->hideEditorStatusBar(id);
}

void EditorManager::setReloadSetting(IDocument::ReloadSetting behavior)
{
    d->m_reloadSetting = behavior;
}

IDocument::ReloadSetting EditorManager::reloadSetting() const
{
    return d->m_reloadSetting;
}

void EditorManager::setAutoSaveEnabled(bool enabled)
{
    d->m_autoSaveEnabled = enabled;
    updateAutoSave();
}

bool EditorManager::autoSaveEnabled() const
{
    return d->m_autoSaveEnabled;
}

void EditorManager::setAutoSaveInterval(int interval)
{
    d->m_autoSaveInterval = interval;
    updateAutoSave();
}

int EditorManager::autoSaveInterval() const
{
    return d->m_autoSaveInterval;
}

QTextCodec *EditorManager::defaultTextCodec() const
{
    QSettings *settings = Core::ICore::settings();
    if (QTextCodec *candidate = QTextCodec::codecForName(
            settings->value(QLatin1String(Constants::SETTINGS_DEFAULTTEXTENCODING)).toByteArray()))
        return candidate;
    if (QTextCodec *defaultUTF8 = QTextCodec::codecForName("UTF-8"))
        return defaultUTF8;
    return QTextCodec::codecForLocale();
}

Core::IEditor *EditorManager::duplicateEditor(Core::IEditor *editor)
{
    if (!editor->duplicateSupported())
        return 0;

    IEditor *duplicate = editor->duplicate(0);
    duplicate->restoreState(editor->saveState());
    connect(duplicate, SIGNAL(changed()), this, SLOT(handleEditorStateChange()));
    emit editorCreated(duplicate, duplicate->document()->fileName());
    addEditor(duplicate, true);
    return duplicate;
}

void EditorManager::split(Qt::Orientation orientation)
{
    SplitterOrView *view = d->m_currentView;

    if (!view)
        view = d->m_currentEditor ? d->m_splitter->findView(d->m_currentEditor)
                       : d->m_splitter->findFirstView();

    if (view && !view->splitter())
        view->split(orientation);

    updateActions();
}

void EditorManager::split()
{
    split(Qt::Vertical);
}

void EditorManager::splitSideBySide()
{
    split(Qt::Horizontal);
}

void EditorManager::removeCurrentSplit()
{
    SplitterOrView *viewToClose = d->m_currentView;
    if (!viewToClose && d->m_currentEditor)
        viewToClose = d->m_splitter->findView(d->m_currentEditor);

    if (!viewToClose || viewToClose->isSplitter() || viewToClose == d->m_splitter)
        return;

    closeView(viewToClose->view());
    updateActions();
}

void EditorManager::removeAllSplits()
{
    if (!d->m_splitter->isSplitter())
        return;
    IEditor *editor = d->m_currentEditor;
    // trigger update below
    d->m_currentEditor = 0;
    if (editor && d->m_editorModel->isDuplicate(editor))
        d->m_editorModel->makeOriginal(editor);
    d->m_splitter->unsplitAll();
    if (!editor)
        editor = pickUnusedEditor();
    activateEditor(editor);
}

void EditorManager::gotoOtherSplit()
{
    if (d->m_splitter->isSplitter()) {
        SplitterOrView *currentView = d->m_currentView;
        if (!currentView && d->m_currentEditor)
            currentView = d->m_splitter->findView(d->m_currentEditor);
        if (!currentView)
            currentView = d->m_splitter->findFirstView();
        SplitterOrView *view = d->m_splitter->findNextView(currentView);
        if (!view)
            view = d->m_splitter->findFirstView();
        if (view) {
            if (IEditor *editor = view->editor()) {
                setCurrentEditor(editor, true);
                editor->widget()->setFocus();
            } else {
                setCurrentView(view);
            }
        }
    }
}

qint64 EditorManager::maxTextFileSize()
{
    return qint64(3) << 24;
}

void EditorManager::setWindowTitleAddition(const QString &addition)
{
    d->m_titleAddition = addition;
    updateWindowTitle();
}

QString EditorManager::windowTitleAddition() const
{
    return d->m_titleAddition;
}

void EditorManager::updateVariable(const QByteArray &variable)
{
    if (variable == kCurrentDocumentFilePath || variable == kCurrentDocumentPath) {
        QString value;
        IEditor *curEditor = currentEditor();
        if (curEditor) {
            QString fileName = curEditor->document()->fileName();
            if (!fileName.isEmpty()) {
                if (variable == kCurrentDocumentFilePath)
                    value = QFileInfo(fileName).filePath();
                else if (variable == kCurrentDocumentPath)
                    value = QFileInfo(fileName).path();
            }
        }
        VariableManager::instance()->insert(variable, value);
    } else if (variable == kCurrentDocumentXPos) {
        QString value;
        IEditor *curEditor = currentEditor();
        if (curEditor)
            value = QString::number(curEditor->widget()->mapToGlobal(QPoint(0,0)).x());
        VariableManager::instance()->insert(variable, value);
    } else if (variable == kCurrentDocumentYPos) {
        QString value;
        IEditor *curEditor = currentEditor();
        if (curEditor)
            value = QString::number(curEditor->widget()->mapToGlobal(QPoint(0,0)).y());
        VariableManager::instance()->insert(variable, value);
    }
}
