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

#include "editormanager.h"
#include "editorgroup.h"
#include "editorsplitter.h"
#include "openeditorsview.h"
#include "openeditorswindow.h"
#include "openwithdialog.h"
#include "filemanager.h"
#include "icore.h"
#include "iversioncontrol.h"
#include "mimedatabase.h"
#include "saveitemsdialog.h"
#include "tabpositionindicator.h"
#include "vcsmanager.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/baseview.h>
#include <coreplugin/imode.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QSet>
#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QFileDialog>
#include <QtGui/QLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QSplitter>

using namespace Core;
using namespace Core::Internal;

enum { debugEditorManager=0 };

static inline ExtensionSystem::PluginManager *pluginManager()
{
    return ExtensionSystem::PluginManager::instance();
}

//===================EditorManager=====================

EditorManagerPlaceHolder *EditorManagerPlaceHolder::m_current = 0;

EditorManagerPlaceHolder::EditorManagerPlaceHolder(Core::IMode *mode, QWidget *parent)
    : QWidget(parent), m_mode(mode)
{
    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    connect(Core::ModeManager::instance(), SIGNAL(currentModeChanged(Core::IMode *)),
            this, SLOT(currentModeChanged(Core::IMode *)));
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

struct Core::EditorManagerPrivate {
    struct EditLocation {
        QPointer<IEditor> editor;
        QString fileName;
        QString kind;
        QVariant state;
    };
    explicit EditorManagerPrivate(ICore *core, QWidget *parent);
    ~EditorManagerPrivate();
    Internal::EditorSplitter *m_splitter;
    ICore *m_core;

    bool m_suppressEditorChanges;

    // actions
    QAction *m_revertToSavedAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_closeCurrentEditorAction;
    QAction *m_closeAllEditorsAction;
    QAction *m_gotoNextDocHistoryAction;
    QAction *m_gotoPreviousDocHistoryAction;
    QAction *m_duplicateAction;
    QAction *m_goBackAction;
    QAction *m_goForwardAction;
    QAction *m_openInExternalEditorAction;

    QList<IEditor *> m_editorHistory;
    QList<EditLocation *> m_navigationHistory;
    int currentNavigationHistoryPosition;
    Internal::OpenEditorsWindow *m_windowPopup;
    Core::BaseView *m_openEditorsView;
    Internal::EditorClosingCoreListener *m_coreListener;

    typedef QMap<IEditor *, QList<IEditor *> *> DuplicateMap;
    DuplicateMap m_duplicates;

    QMap<QString, QVariant> m_editorStates;
    Internal::OpenEditorsViewFactory *m_openEditorsFactory;

    QString fileFilters;
    QString selectedFilter;

    QString m_externalEditor;
};

EditorManagerPrivate::EditorManagerPrivate(ICore *core, QWidget *parent) :
    m_splitter(0),
    m_core(core),
    m_suppressEditorChanges(false),
    m_revertToSavedAction(new QAction(EditorManager::tr("Revert to Saved"), parent)),
    m_saveAction(new QAction(parent)),
    m_saveAsAction(new QAction(parent)),
    m_closeCurrentEditorAction(new QAction(EditorManager::tr("Close"), parent)),
    m_closeAllEditorsAction(new QAction(EditorManager::tr("Close All"), parent)),
    m_gotoNextDocHistoryAction(new QAction(EditorManager::tr("Next Document in History"), parent)),
    m_gotoPreviousDocHistoryAction(new QAction(EditorManager::tr("Previous Document in History"), parent)),
    m_duplicateAction(new QAction(EditorManager::tr("Duplicate Document"), parent)),
    m_goBackAction(new QAction(EditorManager::tr("Go back"), parent)),
    m_goForwardAction(new QAction(EditorManager::tr("Go forward"), parent)),
    m_openInExternalEditorAction(new QAction(EditorManager::tr("Open in External Editor"), parent)),
    currentNavigationHistoryPosition(-1),
    m_windowPopup(0),
    m_coreListener(0)
{

}

EditorManagerPrivate::~EditorManagerPrivate()
{
    qDeleteAll(m_navigationHistory);
    m_navigationHistory.clear();
}

EditorManager *EditorManager::m_instance = 0;

static Command *createSeparator(ActionManager *am, QObject *parent,
                                const QString &name,
                                const QList<int> &context)
{
    QAction *tmpaction = new QAction(parent);
    tmpaction->setSeparator(true);
    Command *cmd = am->registerAction(tmpaction, name, context);
    return cmd;
}

EditorManager::EditorManager(ICore *core, QWidget *parent) :
    QWidget(parent),
    m_d(new EditorManagerPrivate(core, parent))
{
    m_instance = this;

    connect(m_d->m_core, SIGNAL(contextAboutToChange(Core::IContext *)),
            this, SLOT(updateCurrentEditorAndGroup(Core::IContext *)));

    const QList<int> gc =  QList<int>() << Constants::C_GLOBAL_ID;
    const QList<int> editManagerContext =
            QList<int>() << m_d->m_core->uniqueIDManager()->uniqueIdentifier(Constants::C_EDITORMANAGER);

    ActionManager *am = m_d->m_core->actionManager();
    ActionContainer *mfile = am->actionContainer(Constants::M_FILE);

    //Revert to saved
    Command *cmd = am->registerAction(m_d->m_revertToSavedAction,
                                       Constants::REVERTTOSAVED, editManagerContext);
    cmd->setAttribute(Command::CA_UpdateText);
    cmd->setDefaultText(tr("Revert File to Saved"));
    mfile->addAction(cmd, Constants::G_FILE_SAVE);
    connect(m_d->m_revertToSavedAction, SIGNAL(triggered()), this, SLOT(revertToSaved()));

    //Save Action
    am->registerAction(m_d->m_saveAction, Constants::SAVE, editManagerContext);
    connect(m_d->m_saveAction, SIGNAL(triggered()), this, SLOT(saveFile()));

    //Save As Action
    am->registerAction(m_d->m_saveAsAction, Constants::SAVEAS, editManagerContext);
    connect(m_d->m_saveAsAction, SIGNAL(triggered()), this, SLOT(saveFileAs()));

    //Window Menu
    ActionContainer *mwindow = am->actionContainer(Constants::M_WINDOW);

    //Window menu separators
    QAction *tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, QLatin1String("QtCreator.Window.Sep.Split"), editManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, QLatin1String("QtCreator.Window.Sep.Close"), editManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_CLOSE);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, QLatin1String("QtCreator.Window.Sep.Navigate"), editManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, QLatin1String("QtCreator.Window.Sep.Navigate.Groups"), editManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE_GROUPS);

    tmpaction = new QAction(this);
    tmpaction->setSeparator(true);
    cmd = am->registerAction(tmpaction, QLatin1String("QtCreator.Window.Sep.Bottom"), editManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_LIST);

    //Close Action
    cmd = am->registerAction(m_d->m_closeCurrentEditorAction, Constants::CLOSE, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+W")));
    cmd->setAttribute(Core::Command::CA_UpdateText);
    cmd->setDefaultText(m_d->m_closeCurrentEditorAction->text());
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(m_d->m_closeCurrentEditorAction, SIGNAL(triggered()), this, SLOT(closeEditor()));

    //Close All Action
    cmd = am->registerAction(m_d->m_closeAllEditorsAction, Constants::CLOSEALL, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+W")));
    mfile->addAction(cmd, Constants::G_FILE_CLOSE);
    connect(m_d->m_closeAllEditorsAction, SIGNAL(triggered()), this, SLOT(closeAllEditors()));

    //Duplicate Action
    cmd = am->registerAction(m_d->m_duplicateAction, Constants::DUPLICATEDOCUMENT, editManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_CLOSE);
    connect(m_d->m_duplicateAction, SIGNAL(triggered()), this, SLOT(duplicateEditor()));

    // Goto Previous In History Action
    cmd = am->registerAction(m_d->m_gotoPreviousDocHistoryAction, Constants::GOTOPREVINHISTORY, editManagerContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Tab")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Tab")));
#endif
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_d->m_gotoPreviousDocHistoryAction, SIGNAL(triggered()), this, SLOT(gotoPreviousDocHistory()));

    // Goto Next In History Action
    cmd = am->registerAction(m_d->m_gotoNextDocHistoryAction, Constants::GOTONEXTINHISTORY, editManagerContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Shift+Tab")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+Tab")));
#endif
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_d->m_gotoNextDocHistoryAction, SIGNAL(triggered()), this, SLOT(gotoNextDocHistory()));

    // Go back in navigation history
    cmd = am->registerAction(m_d->m_goBackAction, Constants::GO_BACK, editManagerContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Left")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Left")));
#endif
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_d->m_goBackAction, SIGNAL(triggered()), this, SLOT(goBackInNavigationHistory()));

    // Go forward in navigation history
    cmd = am->registerAction(m_d->m_goForwardAction, Constants::GO_FORWARD, editManagerContext);
#ifdef Q_WS_MAC
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+Right")));
#else
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Right")));
#endif
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_d->m_goForwardAction, SIGNAL(triggered()), this, SLOT(goForwardInNavigationHistory()));


    ActionContainer *medit = am->actionContainer(Constants::M_EDIT);
    ActionContainer *advancedMenu = am->createMenu(Constants::M_EDIT_ADVANCED);
    medit->addMenu(advancedMenu, Constants::G_EDIT_ADVANCED);
    advancedMenu->menu()->setTitle(tr("&Advanced"));
    advancedMenu->appendGroup(Constants::G_EDIT_FORMAT);
    advancedMenu->appendGroup(Constants::G_EDIT_COLLAPSING);
    advancedMenu->appendGroup(Constants::G_EDIT_FONT);
    advancedMenu->appendGroup(Constants::G_EDIT_EDITOR);

    // Advanced menu separators
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Collapsing"), editManagerContext);
    advancedMenu->addAction(cmd, Constants::G_EDIT_COLLAPSING);
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Font"), editManagerContext);
    advancedMenu->addAction(cmd, Constants::G_EDIT_FONT);
    cmd = createSeparator(am, this, QLatin1String("QtCreator.Edit.Sep.Editor"), editManagerContext);
    advancedMenu->addAction(cmd, Constants::G_EDIT_EDITOR);

    cmd = am->registerAction(m_d->m_openInExternalEditorAction, Constants::OPEN_IN_EXTERNAL_EDITOR, editManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+V,Alt+I")));
    advancedMenu->addAction(cmd, Constants::G_EDIT_EDITOR);
    connect(m_d->m_openInExternalEditorAction, SIGNAL(triggered()), this, SLOT(openInExternalEditor()));


    // other setup
    connect(this, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateActions()));
    connect(this, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateEditorHistory()));
    m_d->m_splitter = new EditorSplitter;
    connect(m_d->m_splitter, SIGNAL(closeRequested(Core::IEditor *)),
            this, SLOT(closeEditor(Core::IEditor *)));
    connect(m_d->m_splitter, SIGNAL(editorGroupsChanged()),
            this, SIGNAL(editorGroupsChanged()));

    QHBoxLayout *l = new QHBoxLayout(this);
    l->setSpacing(0);
    l->setMargin(0);
    l->addWidget(m_d->m_splitter);

    updateActions();

    m_d->m_windowPopup = new OpenEditorsWindow(this);
}

EditorManager::~EditorManager()
{
    if (m_d->m_core) {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        if (m_d->m_coreListener) {
            pm->removeObject(m_d->m_coreListener);
            delete m_d->m_coreListener;
        }
        pm->removeObject(m_d->m_openEditorsFactory);
        delete m_d->m_openEditorsFactory;
    }
    delete m_d;
}

void EditorManager::init()
{
    QList<int> context;
    context << m_d->m_core->uniqueIDManager()->uniqueIdentifier("QtCreator.OpenDocumentsView");

    m_d->m_coreListener = new EditorClosingCoreListener(this);
    
    pluginManager()->addObject(m_d->m_coreListener);

    m_d->m_openEditorsFactory = new OpenEditorsViewFactory();
    pluginManager()->addObject(m_d->m_openEditorsFactory);
}

QSize EditorManager::minimumSizeHint() const
{
    return QSize(400, 300);
}

QString EditorManager::defaultExternalEditor() const
{
#ifdef Q_OS_MAC
    return m_d->m_core->resourcePath()
            +QLatin1String("/runInTerminal.command vi %f %l %c %W %H %x %y");
#elif defined(Q_OS_UNIX)
    return QLatin1String("xterm -geom %Wx%H+%x+%y -e vi %f +%l +\"normal %c|\"");
#elif defined (Q_OS_WIN)
    return QLatin1String("notepad %f");
#else
    return QString();
#endif
}

EditorSplitter *EditorManager::editorSplitter() const
{
    return m_d->m_splitter;
}

void EditorManager::updateEditorHistory()
{
    IEditor *editor = currentEditor();
    if (!editor)
        return;
    m_d->m_editorHistory.removeAll(editor);
    m_d->m_editorHistory.prepend(editor);
}

bool EditorManager::registerEditor(IEditor *editor)
{
    if (editor) {
        if (!hasDuplicate(editor)) {
            m_d->m_core->fileManager()->addFile(editor->file());
            m_d->m_core->fileManager()->addToRecentFiles(editor->file()->fileName());
        }
        m_d->m_editorHistory.removeAll(editor);
        m_d->m_editorHistory.prepend(editor);
        return true;
    }
    return false;
}

bool EditorManager::unregisterEditor(IEditor *editor)
{
    if (editor) {
        if (!hasDuplicate(editor))
            m_d->m_core->fileManager()->removeFile(editor->file());
        m_d->m_editorHistory.removeAll(editor);
        return true;
    }
    return false;
}


void EditorManager::updateCurrentEditorAndGroup(IContext *context)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO;
    EditorGroupContext *groupContext = context ? qobject_cast<EditorGroupContext*>(context) : 0;
    IEditor *editor = context ? qobject_cast<IEditor*>(context) : 0;
    if (groupContext) {
        m_d->m_splitter->setCurrentGroup(groupContext->editorGroup());
        if (groupContext->editorGroup()->editorCount() == 0)
            setCurrentEditor(0);
        updateActions();
    } else if (editor) {
        setCurrentEditor(editor);
    } else {
        updateActions();
    }
    if (debugEditorManager)
        qDebug() << "leaving method" << Q_FUNC_INFO;
}

void EditorManager::setCurrentEditor(IEditor *editor, bool ignoreNavigationHistory)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << currentEditor() << "-->" << editor
                << (m_d->m_suppressEditorChanges?"suppressed":"")
                << "ignore history?" << ignoreNavigationHistory;
    if (m_d->m_suppressEditorChanges)
        return;
    if (editor) {
        bool addToHistory = (!ignoreNavigationHistory && editor != currentEditor());
        if (debugEditorManager)
            qDebug() << Q_FUNC_INFO << (addToHistory ? "adding to history" : "not adding to history");
        if (addToHistory)
            addCurrentPositionToNavigationHistory(true);
        EditorGroup *group = groupOfEditor(editor);
        if (!group)
            return;
        m_d->m_suppressEditorChanges = true;
        m_d->m_splitter->setCurrentGroup(group);
        group->setCurrentEditor(editor);
        m_d->m_suppressEditorChanges = false;
        if (addToHistory)
            addCurrentPositionToNavigationHistory();
    }
    editorChanged(editor);
}

void EditorManager::editorChanged(IEditor *toEditor)
{
    emit currentEditorChanged(toEditor);
}

EditorGroup *EditorManager::groupOfEditor(IEditor *editor) const
{
    foreach (EditorGroup *group, m_d->m_splitter->groups()) {
        if (group->editors().contains(editor))
            return group;
    }
    return 0;
}

QList<IEditor *> EditorManager::editorsForFileName(const QString &filename) const
{
    QList<IEditor *> found;
    QString fixedname = FileManager::fixFileName(filename);
    foreach (IEditor *editor, openedEditors()) {
        if (fixedname == FileManager::fixFileName(editor->file()->fileName()))
            found << editor;
    }
    return found;
}

IEditor *EditorManager::currentEditor() const
{
    return m_d->m_splitter->currentGroup()->currentEditor();
}

EditorGroup *EditorManager::currentEditorGroup() const
{
    return m_d->m_splitter->currentGroup();
}

void EditorManager::duplicateEditor()
{
    IEditor *curEditor = currentEditor();
    if (!curEditor || !curEditor->duplicateSupported())
        return;
    IEditor *editor = curEditor->duplicate(this);
    registerDuplicate(curEditor, editor);
    insertEditor(editor);
}

// SLOT connected to action
// since this is potentially called in the event handler of the editor
// we simply postpone it with a single shot timer
void EditorManager::closeEditor()
{
    static bool postpone = true;
    if (postpone) {
        QTimer::singleShot(0, this, SLOT(closeEditor()));
        postpone = false;
    } else {
        closeEditor(currentEditor());
        postpone = true;
    }

}

void EditorManager::closeEditor(IEditor *editor)
{
    if (!editor)
        editor = currentEditor();
    if (!editor)
        return;
    closeEditors(QList<IEditor *>() << editor);
}

QList<IEditor*>
    EditorManager::editorsForFiles(QList<IFile*> files) const
{
    const QList<IEditor *> editors = openedEditors();
    QSet<IEditor *> found;
    foreach (IFile *file, files) {
        foreach (IEditor *editor, editors) {
            if (editor->file() == file && !found.contains(editor)) {
                if (hasDuplicate(editor)) {
                    foreach (IEditor *duplicate, duplicates(editor)) {
                        found << duplicate;
                    }
                } else {
                    found << editor;
                }
            }
        }
    }
    return found.toList();
}

QList<IFile *> EditorManager::filesForEditors(QList<IEditor *> editors) const
{
    QSet<IEditor *> handledEditors;
    QList<IFile *> files;
    foreach (IEditor *editor, editors) {
        if (!handledEditors.contains(editor)) {
            files << editor->file();
            if (hasDuplicate(editor)) {
                foreach (IEditor *duplicate, duplicates(editor)) {
                    handledEditors << duplicate;
                }
            } else {
                handledEditors.insert(editor);
            }
        }
    }
    return files;
}

bool EditorManager::closeAllEditors(bool askAboutModifiedEditors)
{
    return closeEditors(openedEditors(), askAboutModifiedEditors);
}

bool EditorManager::closeEditors(const QList<IEditor*> editorsToClose, bool askAboutModifiedEditors)
{
    if (editorsToClose.isEmpty())
        return true;
    bool closingFailed = false;
    QList<IEditor*> acceptedEditors;
    //ask all core listeners to check whether the editor can be closed
    const QList<ICoreListener *> listeners =
        pluginManager()->getObjects<ICoreListener>();
    foreach (IEditor *editor, editorsToClose) {
        bool editorAccepted = true;
        foreach (ICoreListener *listener, listeners) {
            if (!listener->editorAboutToClose(editor)) {
                editorAccepted = false;
                closingFailed = false;
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
        QList<IFile*> list = ICore::instance()->fileManager()->
            saveModifiedFiles(filesForEditors(acceptedEditors), &cancelled);
        if (cancelled)
            return false;
        if (!list.isEmpty()) {
            QSet<IEditor*> skipSet = editorsForFiles(list).toSet();
            acceptedEditors = acceptedEditors.toSet().subtract(skipSet).toList();
            closingFailed = false;
        }
    }
    if (acceptedEditors.isEmpty())
        return false;
    bool currentEditorRemoved = false;
    IEditor *current = currentEditor();
    if (current)
        addCurrentPositionToNavigationHistory(true);
    // remove current editor last, for optimization
    if (acceptedEditors.contains(current)) {
        currentEditorRemoved = true;
        acceptedEditors.removeAll(current);
        acceptedEditors.append(current);
    }
    // remove the editors
    foreach (IEditor *editor, acceptedEditors) {
        emit editorAboutToClose(editor);
        if (!editor->file()->fileName().isEmpty()) {
            QByteArray state = editor->saveState();
            if (!state.isEmpty())
                m_d->m_editorStates.insert(editor->file()->fileName(), QVariant(state));
        }
        unregisterEditor(editor);
        if (hasDuplicate(editor))
            unregisterDuplicate(editor);
        m_d->m_core->removeContextObject(editor);
        EditorGroup *group = groupOfEditor(editor);
        const bool suppress = m_d->m_suppressEditorChanges;
        m_d->m_suppressEditorChanges = true;
        if (group)
            group->removeEditor(editor);
        m_d->m_suppressEditorChanges = suppress;
    }
    emit editorsClosed(acceptedEditors);
    foreach (IEditor *editor, acceptedEditors) {
        delete editor;
    }
    if (currentEditorRemoved) {
        if (m_d->m_editorHistory.count() > 0) {
            setCurrentEditor(m_d->m_editorHistory.first(), true);
        } else {
            editorChanged(currentEditor());
        }
    }
    if (currentEditor())
        addCurrentPositionToNavigationHistory();
    updateActions();

    return !closingFailed;
}


/* Find editors for a mimetype, best matching at the front
 * of the list. Recurse over the parent classes of the mimetype to
 * find them. */
static void mimeTypeFactoryRecursion(const MimeDatabase *db,
                                     const MimeType &mimeType,
                                     const QList<IEditorFactory*> &allFactories,
                                     bool firstMatchOnly,
                                     QList<IEditorFactory*> *list)
{
    typedef QList<IEditorFactory*> EditorFactoryList;
    // Loop factories to find type
    const QString type = mimeType.type();
    const EditorFactoryList::const_iterator fcend = allFactories.constEnd();
    for (EditorFactoryList::const_iterator fit = allFactories.constBegin(); fit != fcend; ++fit) {
        // Exclude duplicates when recursing over xml or C++ -> C -> text.
        IEditorFactory *factory = *fit;
        if (!list->contains(factory) && factory->mimeTypes().contains(type)) {
            list->push_back(*fit);
            if (firstMatchOnly)
                return;
            break;
        }
    }
    // Any parent classes? -> recurse
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
    EditorManager::editorFactories(const MimeType &mimeType, bool bestMatchOnly) const
{
    EditorFactoryList rc;
    const EditorFactoryList allFactories = pluginManager()->getObjects<IEditorFactory>();
    mimeTypeFactoryRecursion(m_d->m_core->mimeDatabase(), mimeType, allFactories, bestMatchOnly, &rc);
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << mimeType.type() << " returns " << rc;
    return rc;
}

IEditor *EditorManager::createEditor(const QString &editorKind,
                                     const QString &fileName)
{
    typedef QList<IEditorFactory*> FactoryList;
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << editorKind << fileName;


    EditorFactoryList factories;
    if (editorKind.isEmpty()) {
        // Find by mime type
        const MimeType mimeType = m_d->m_core->mimeDatabase()->findByFile(QFileInfo(fileName));
        if (!mimeType) {
            qWarning("%s unable to determine mime type of %s/%s.",
                     Q_FUNC_INFO, fileName.toUtf8().constData(), editorKind.toUtf8().constData());
            return 0;
        }
        factories = editorFactories(mimeType, true);
    } else {
        // Find by editor kind
        const EditorFactoryList allFactories = pluginManager()->getObjects<IEditorFactory>();
        const EditorFactoryList::const_iterator acend = allFactories.constEnd();
        for (EditorFactoryList::const_iterator ait = allFactories.constBegin(); ait != acend; ++ait) {
            if (editorKind == (*ait)->kind()) {
                factories.push_back(*ait);
                break;
            }
        }
    }
    if (factories.empty()) {
        qWarning("%s: unable to find an editor factory for the file '%s', editor kind '%s'.",
                 Q_FUNC_INFO, fileName.toUtf8().constData(), editorKind.toUtf8().constData());
        return 0;
    }

    IEditor *editor = factories.front()->createEditor(this);
    if (editor)
        connect(editor, SIGNAL(changed()), this, SLOT(updateActions()));
    if (editor)
        emit editorCreated(editor, fileName);
    return editor;
}

void EditorManager::insertEditor(IEditor *editor,
                                 bool ignoreNavigationHistory,
                                 EditorGroup *group)
{
    if (!editor)
        return;
    m_d->m_core->addContextObject(editor);
    registerEditor(editor);
    if (group)
        group->addEditor(editor);
    else
        m_d->m_splitter->currentGroup()->addEditor(editor);

    setCurrentEditor(editor, ignoreNavigationHistory);
    emit editorOpened(editor);
}

// Run the OpenWithDialog and return the editor kind
// selected by the user.
QString EditorManager::getOpenWithEditorKind(const QString &fileName) const
{
    QStringList editorKinds;
    // Collect editors that can open the file
    if (const MimeType mt = m_d->m_core->mimeDatabase()->findByFile(fileName)) {
        const EditorFactoryList editors = editorFactories(mt, false);
        const int size = editors.size();
        for (int i = 0; i < size; i++) {
            editorKinds.push_back(editors.at(i)->kind());
        }
    }
    if (editorKinds.empty())
        return QString();

    // Run dialog.
    OpenWithDialog dialog(fileName, m_d->m_core->mainWindow());
    dialog.setEditors(editorKinds);
    dialog.setCurrentEditor(0);
    if (dialog.exec() != QDialog::Accepted)
        return QString();
    return dialog.editor();
}

static QString formatFileFilters(const Core::ICore *core, QString *selectedFilter)
{
    QString rc;
    // Compile list of filter strings. If we find a glob  matching all files,
    // put it last and set it as default selectedFilter.
    QStringList filters = core->mimeDatabase()->filterStrings();
    filters.sort();
    selectedFilter->clear();
    if (filters.empty())
        return rc;
    const QString filterSeparator = QLatin1String(";;");
    bool hasAllFilter = false;
    const int size = filters.size();
    for (int i = 0; i < size; i++) {
        const QString &filterString = filters.at(i);
        if (filterString.isEmpty()) { // binary editor
            hasAllFilter = true;
        } else {
            if (!rc.isEmpty())
                rc += filterSeparator;
            rc += filterString;
        }
    }
    if (hasAllFilter) {
        // prepend all files filter
        // prepending instead of appending to work around a but in Qt/Mac
        QString allFilesFilter = QLatin1String("All Files (*)");
        if (!rc.isEmpty())
            allFilesFilter += filterSeparator;
        rc.prepend(allFilesFilter);
        *selectedFilter = allFilesFilter;
    } else {
        *selectedFilter = filters.front();
    }
    return rc;
}

IEditor *EditorManager::openEditor(const QString &fileName, const QString &editorKind,
                                   bool ignoreNavigationHistory)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << fileName << editorKind;

    if (fileName.isEmpty())
        return 0;

    const QList<IEditor *> editors = editorsForFileName(fileName);
    if (!editors.isEmpty()) {
        setCurrentEditor(editors.first(), ignoreNavigationHistory);
        return editors.first();
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    IEditor *editor = createEditor(editorKind, fileName);
    if (!editor || !editor->open(fileName)) {
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(m_d->m_core->mainWindow(), tr("Opening File"), tr("Cannot open file %1!").arg(fileName));
        delete editor;
        editor = 0;
        return 0;
    }
    insertEditor(editor, ignoreNavigationHistory);
    restoreEditorState(editor);
    QApplication::restoreOverrideCursor();
    ensureEditorManagerVisible();
    setCurrentEditor(editor);
    return editor;
}

QStringList EditorManager::getOpenFileNames() const
{
    QString dir;
    if (m_d->fileFilters.isEmpty())
        m_d->fileFilters = formatFileFilters(m_d->m_core, &m_d->selectedFilter);

    if (IEditor *curEditor = currentEditor()) {
        const QFileInfo fi(curEditor->file()->fileName());
        dir = fi.absolutePath();
    }

    return QFileDialog::getOpenFileNames(m_d->m_core->mainWindow(), tr("Open File"),
                                         dir, m_d->fileFilters, &m_d->selectedFilter);
}

void EditorManager::ensureEditorManagerVisible()
{
    if (!isVisible())
        m_d->m_core->modeManager()->activateMode(Constants::MODE_EDIT);
}

IEditor *EditorManager::newFile(const QString &editorKind,
                                        QString *titlePattern,
                                        const QString &contents)
{
    if (debugEditorManager)
        qDebug() << Q_FUNC_INFO << editorKind << titlePattern << contents;

    if (editorKind.isEmpty())
        return 0;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    IEditor *edt = createEditor(editorKind);
    if (!edt)
        return 0;

    if (!edt || !edt->createNew(contents)) {
        QApplication::restoreOverrideCursor();
        delete edt;
        edt = 0;
        return 0;
    }

    QString title = edt->displayName();

    if (title.isEmpty() && titlePattern) {
        const QChar dollar = QLatin1Char('$');
        const QChar dot = QLatin1Char('.');

        QString base = *titlePattern;
        if (base.isEmpty())
            base = QLatin1String("unnamed$");
        if (base.contains(dollar)) {
            int i = 1;
            QSet<QString> docnames;
            foreach (IEditor *editor, openedEditors()) {
                QString name = editor->file()->fileName();
                if (name.isEmpty()) {
                    name = editor->displayName();
                    name.remove(QLatin1Char('*'));
                } else {
                    name = QFileInfo(name).completeBaseName();
                }
                docnames << name;
            }

            do {
                title = base;
                title.replace(QString(dollar), QString::number(i++));
            } while (docnames.contains(title));
        } else {
            title = *titlePattern;
        }
    }
    *titlePattern = title;
    edt->setDisplayName(title);
    insertEditor(edt);
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
    QString fileName = editor->file()->fileName();
    if (m_d->m_editorStates.contains(fileName))
        editor->restoreState(m_d->m_editorStates.value(fileName).toByteArray());
}

bool EditorManager::saveEditor(IEditor *editor)
{
    return saveFile(editor);
}

bool EditorManager::saveFile(IEditor *editor)
{
    if (!editor)
        editor = currentEditor();
    if (!editor)
        return false;

    IFile *file = editor->file();
    const QString &fileName = file->fileName();
    if (!fileName.isEmpty() && file->isReadOnly()) {
        MakeWritableResult answer =
                makeEditorWritable(editor);
        if (answer == Failed)
            return false;
        if (answer == SavedAs)
            return true;
    }

    if (file->isReadOnly() || fileName.isEmpty()) {
        return saveFileAs(editor);
    }

    m_d->m_core->fileManager()->blockFileChange(file);
    const bool success = file->save(fileName);
    m_d->m_core->fileManager()->unblockFileChange(file);
    if (success)
        m_d->m_core->fileManager()->addToRecentFiles(editor->file()->fileName());
    return success;
}

EditorManager::ReadOnlyAction
    EditorManager::promptReadOnlyFile(const QString &fileName,
                                      const IVersionControl *versionControl,
                                      QWidget *parent,
                                      bool displaySaveAsButton)
{
    QMessageBox msgBox(QMessageBox::Question, QObject::tr("File is Read Only"),
                       QObject::tr("The file %1 is read only.").arg(fileName),
                       QMessageBox::Cancel, parent);

    QPushButton *sccButton = 0;
    if (versionControl && versionControl->supportsOperation(IVersionControl::OpenOperation))
        sccButton = msgBox.addButton(QObject::tr("Open with VCS (%1)").arg(versionControl->name()), QMessageBox::AcceptRole);

    QPushButton *makeWritableButton =  msgBox.addButton(QObject::tr("Make writable"), QMessageBox::AcceptRole);

    QPushButton *saveAsButton = 0;
    if (displaySaveAsButton)
        msgBox.addButton(QObject::tr("Save as ..."), QMessageBox::ActionRole);

    msgBox.setDefaultButton(sccButton ? sccButton : makeWritableButton);
    msgBox.exec();

    QAbstractButton *clickedButton = msgBox.clickedButton();
    if (clickedButton == sccButton)
        return RO_OpenVCS;
    if (clickedButton == makeWritableButton)
        return RO_MakeWriteable;
    if (clickedButton == saveAsButton)
        return RO_SaveAs;
    return  RO_Cancel;
}


MakeWritableResult
EditorManager::makeEditorWritable(IEditor *editor)
{
    QString directory = QFileInfo(editor->file()->fileName()).absolutePath();
    IVersionControl *versionControl = m_d->m_core->vcsManager()->findVersionControlForDirectory(directory);
    IFile *file = editor->file();
    const QString &fileName = file->fileName();

    switch (promptReadOnlyFile(fileName, versionControl, m_d->m_core->mainWindow(), true)) {
    case RO_OpenVCS:
        if (!versionControl->vcsOpen(fileName)) {
            QMessageBox::warning(m_d->m_core->mainWindow(), tr("Failed!"), tr("Could not open the file for edit with SCC."));
            return Failed;
        }
        return OpenedWithVersionControl;
    case RO_MakeWriteable: {
        const bool permsOk = QFile::setPermissions(fileName, QFile::permissions(fileName) | QFile::WriteUser);
        if (!permsOk) {
            QMessageBox::warning(m_d->m_core->mainWindow(), tr("Failed!"),  tr("Could not set permissions to writable."));
            return Failed;
        }
    }
        return MadeWritable;
    case RO_SaveAs :
        return saveFileAs(editor) ? SavedAs : Failed;
    case RO_Cancel:
        break;
    }
    return Failed;
}

bool EditorManager::saveFileAs(IEditor *editor)
{
    if (!editor)
        editor = currentEditor();
    if (!editor)
        return false;

    QString absoluteFilePath = m_d->m_core->fileManager()->getSaveAsFileName(editor->file());
    if (absoluteFilePath.isEmpty())
        return false;
    if (absoluteFilePath != editor->file()->fileName()) {
        const QList<IEditor *> existList = editorsForFileName(absoluteFilePath);
        if (!existList.isEmpty()) {
            closeEditors(existList, false);
        }
    }

    m_d->m_core->fileManager()->blockFileChange(editor->file());
    const bool success = editor->file()->save(absoluteFilePath);
    m_d->m_core->fileManager()->unblockFileChange(editor->file());

    if (success)
        m_d->m_core->fileManager()->addToRecentFiles(editor->file()->fileName());

    updateActions();
    return success;
}

void EditorManager::gotoNextDocHistory()
{
    OpenEditorsWindow *dialog = windowPopup();
    dialog->setMode(OpenEditorsWindow::HistoryMode);
    dialog->selectNextEditor();
    showWindowPopup();
}

void EditorManager::gotoPreviousDocHistory()
{
    OpenEditorsWindow *dialog = windowPopup();
    dialog->setMode(OpenEditorsWindow::HistoryMode);
    dialog->selectPreviousEditor();
    showWindowPopup();
}

void EditorManager::makeCurrentEditorWritable()
{
    if (IEditor* curEditor = currentEditor())
        makeEditorWritable(curEditor);
}

void EditorManager::updateActions()
{
    QString fName;
    IEditor *curEditor = currentEditor();
    int openedCount = openedEditors().count();
    if (curEditor) {
        if (!curEditor->file()->fileName().isEmpty()) {
            QFileInfo fi(curEditor->file()->fileName());
            fName = fi.fileName();
        } else {
            fName = curEditor->displayName();
        }


        if (curEditor->file()->isModified() && curEditor->file()->isReadOnly()) {
            // we are about to change a read-only file, warn user
            showEditorInfoBar(QLatin1String("Core.EditorManager.MakeWritable"),
                tr("<b>Warning:</b> You are changing a read-only file."),
                tr("Make writable"), this, SLOT(makeCurrentEditorWritable()));
        } else {
            hideEditorInfoBar(QLatin1String("Core.EditorManager.MakeWritable"));
        }
    }

    m_d->m_saveAction->setEnabled(curEditor != 0 && curEditor->file()->isModified());
    m_d->m_saveAsAction->setEnabled(curEditor != 0 && curEditor->file()->isSaveAsAllowed());
    m_d->m_revertToSavedAction->setEnabled(curEditor != 0
        && !curEditor->file()->fileName().isEmpty() && curEditor->file()->isModified());

    m_d->m_saveAsAction->setText(tr("Save %1 As...").arg(fName));
    m_d->m_saveAction->setText(tr("&Save %1").arg(fName));
    m_d->m_revertToSavedAction->setText(tr("Revert %1 to Saved").arg(fName));


    m_d->m_closeCurrentEditorAction->setEnabled(m_d->m_splitter->currentGroup()->editorCount() > 0);
    m_d->m_closeCurrentEditorAction->setText(tr("Close %1").arg(fName));
    m_d->m_closeAllEditorsAction->setEnabled(openedCount > 0);

    m_d->m_gotoNextDocHistoryAction->setEnabled(m_d->m_editorHistory.count() > 0);
    m_d->m_gotoPreviousDocHistoryAction->setEnabled(m_d->m_editorHistory.count() > 0);
    m_d->m_goBackAction->setEnabled(m_d->currentNavigationHistoryPosition > 0);
    m_d->m_goForwardAction->setEnabled(m_d->currentNavigationHistoryPosition < m_d->m_navigationHistory.size()-1);

    m_d->m_duplicateAction->setEnabled(curEditor != 0 && curEditor->duplicateSupported());

    m_d->m_openInExternalEditorAction->setEnabled(curEditor != 0);
}

QList<IEditor*> EditorManager::openedEditors() const
{
    QList<IEditor*> editors;
    const QList<EditorGroup*> groups = m_d->m_splitter->groups();
    foreach (EditorGroup *group, groups) {
        editors += group->editors();
    }
    return editors;
}

QList<EditorGroup *> EditorManager::editorGroups() const
{
    return m_d->m_splitter->groups();
}

QList<IEditor*> EditorManager::editorHistory() const
{
    return m_d->m_editorHistory;
}

void EditorManager::addCurrentPositionToNavigationHistory(bool compress)
{
    IEditor *editor = currentEditor();
    if (!editor)
        return;
    if (!editor->file())
        return;
    QString fileName = editor->file()->fileName();
    QByteArray state = editor->saveState();
    // cut existing
    int firstIndexToRemove;
    if (compress && m_d->currentNavigationHistoryPosition >= 0) {
        EditorManagerPrivate::EditLocation *previousLocation =
                m_d->m_navigationHistory.at(m_d->currentNavigationHistoryPosition);
        if ((previousLocation->editor && editor == previousLocation->editor)
                || (!fileName.isEmpty() && previousLocation->fileName == fileName)) {
            firstIndexToRemove = m_d->currentNavigationHistoryPosition;
        } else {
            firstIndexToRemove = m_d->currentNavigationHistoryPosition+1;
        }
    } else {
        firstIndexToRemove = m_d->currentNavigationHistoryPosition+1;
    }
    if (firstIndexToRemove >= 0) {
        for (int i = m_d->m_navigationHistory.size()-1; i >= firstIndexToRemove; --i) {
            delete m_d->m_navigationHistory.takeLast();
        }
    }
    while (m_d->m_navigationHistory.size() >= 30) {
        delete m_d->m_navigationHistory.takeFirst();
    }
    EditorManagerPrivate::EditLocation *location = new EditorManagerPrivate::EditLocation;
    location->editor = editor;
    location->fileName = editor->file()->fileName();
    location->kind = editor->kind();
    location->state = QVariant(state);
    m_d->m_navigationHistory.append(location);
    m_d->currentNavigationHistoryPosition = m_d->m_navigationHistory.size()-1;
    updateActions();
}

void EditorManager::goBackInNavigationHistory()
{
    while (m_d->currentNavigationHistoryPosition > 0) {
        --m_d->currentNavigationHistoryPosition;
        EditorManagerPrivate::EditLocation *location = m_d->m_navigationHistory.at(m_d->currentNavigationHistoryPosition);
        IEditor *editor;
        if (location->editor) {
            editor = location->editor;
            setCurrentEditor(location->editor, true);
        } else {
            editor = openEditor(location->fileName, location->kind, true);
            if (!editor) {
                delete m_d->m_navigationHistory.takeAt(m_d->currentNavigationHistoryPosition);
                continue;
            }
        }
        editor->restoreState(location->state.toByteArray());
        updateActions();
        ensureEditorManagerVisible();
        return;
    }
}

void EditorManager::goForwardInNavigationHistory()
{
    if (m_d->currentNavigationHistoryPosition >= m_d->m_navigationHistory.size()-1)
        return;
    ++m_d->currentNavigationHistoryPosition;
    EditorManagerPrivate::EditLocation *location = m_d->m_navigationHistory.at(m_d->currentNavigationHistoryPosition);
    IEditor *editor;
    if (location->editor) {
        editor = location->editor;
        setCurrentEditor(location->editor, true);
    } else {
        editor = openEditor(location->fileName, location->kind, true);
        if (!editor) {
            //TODO
            qDebug() << Q_FUNC_INFO << "can't open file" << location->fileName;
            return;
        }
    }
    editor->restoreState(location->state.toByteArray());
    updateActions();
    ensureEditorManagerVisible();
}

OpenEditorsWindow *EditorManager::windowPopup() const
{
    return m_d->m_windowPopup;
}

void EditorManager::showWindowPopup() const
{
    const QPoint p(mapToGlobal(QPoint(0, 0)));
    m_d->m_windowPopup->move((width()-m_d->m_windowPopup->width())/2 + p.x(),
                        (height()-m_d->m_windowPopup->height())/2 + p.y());
    m_d->m_windowPopup->setVisible(true);
}

void EditorManager::registerDuplicate(IEditor *original,
                                      IEditor *duplicate)
{
    QList<IEditor *> *duplicateList;
    if (m_d->m_duplicates.contains(original)) {
        duplicateList = m_d->m_duplicates.value(original);
    } else {
        duplicateList = new QList<IEditor *>;
        duplicateList->append(original);
        m_d->m_duplicates.insert(original, duplicateList);
    }
    duplicateList->append(duplicate);
    m_d->m_duplicates.insert(duplicate, duplicateList);
}

void EditorManager::unregisterDuplicate(IEditor *editor)
{
    if (!m_d->m_duplicates.contains(editor))
        return;
    QList<IEditor *> *duplicateList = m_d->m_duplicates.value(editor);
    duplicateList->removeAll(editor);
    m_d->m_duplicates.remove(editor);
    if (duplicateList->count() < 2) {
        foreach (IEditor *other, *duplicateList) {
            m_d->m_duplicates.remove(other);
        }
        delete duplicateList;
    }
}

bool EditorManager::hasDuplicate(IEditor *editor) const
{
    return m_d->m_duplicates.contains(editor);
}

QList<IEditor *>
        EditorManager::duplicates(IEditor *editor) const
{
    if (m_d->m_duplicates.contains(editor))
        return *m_d->m_duplicates.value(editor);
    return QList<IEditor *>() << editor;
}

QByteArray EditorManager::saveState() const
{
    //todo: versioning
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << m_d->m_splitter->saveState();
    stream << saveOpenEditorList();
    stream << m_d->m_editorStates;
    return bytes;
}

bool EditorManager::restoreState(const QByteArray &state)
{
    closeAllEditors(true);
    //todo: versioning
    QDataStream stream(state);
    QByteArray data;
    QMap<QString, QVariant> editorstates;
    stream >> data;
    const bool success = m_d->m_splitter->restoreState(data);
    if (!success)
        return false;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    bool editorChangesSuppressed = m_d->m_suppressEditorChanges;
    m_d->m_suppressEditorChanges = true;

    stream >> data;
    restoreOpenEditorList(data);
    stream >> editorstates;
    QMapIterator<QString, QVariant> i(editorstates);
    while (i.hasNext()) {
        i.next();
        m_d->m_editorStates.insert(i.key(), i.value());
    }

    m_d->m_suppressEditorChanges = editorChangesSuppressed;
    if (currentEditor())
        setCurrentEditor(currentEditor());// looks like a null-op but is not
    
    QApplication::restoreOverrideCursor();

    return true;
}

void EditorManager::saveSettings(QSettings *settings)
{
    m_d->m_splitter->saveSettings(settings);
    settings->setValue(QLatin1String("EditorManager/DocumentStates"),
                       m_d->m_editorStates);
    settings->setValue(QLatin1String("EditorManager/ExternalEditorCommand"),
                       m_d->m_externalEditor);
}

void EditorManager::readSettings(QSettings *settings)
{
    m_d->m_splitter->readSettings(settings);
    if (settings->contains(QLatin1String("EditorManager/DocumentStates")))
        m_d->m_editorStates = settings->value(QLatin1String("EditorManager/DocumentStates"))
            .value<QMap<QString, QVariant> >();
    if (settings->contains(QLatin1String("EditorManager/ExternalEditorCommand")))
        m_d->m_externalEditor = settings->value(QLatin1String("EditorManager/ExternalEditorCommand")).toString();
}

QByteArray EditorManager::saveOpenEditorList() const
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    QMap<QString, QByteArray> outlist;
    QMapIterator<QString, EditorGroup *> i(m_d->m_splitter->pathGroupMap());
    while (i.hasNext()) {
        i.next();
        outlist.insert(i.key(), i.value()->saveState());
    }
    stream << outlist;
    return bytes;
}

void EditorManager::restoreOpenEditorList(const QByteArray &state)
{
    QDataStream in(state);
    QMap<QString, EditorGroup *> pathGroupMap = m_d->m_splitter->pathGroupMap();
    QMap<QString, QByteArray> inlist;
    in >> inlist;
    QMapIterator<QString, QByteArray> i(inlist);
    while (i.hasNext()) {
        i.next();
        EditorGroup *group = pathGroupMap.value(i.key());
        if (!group)
            continue;
        group->restoreState(i.value());
    }
}

IEditor *EditorManager::restoreEditor(QString fileName, QString editorKind, EditorGroup *group)
{
    IEditor *editor;
    QList<IEditor *> existing =
            editorsForFileName(fileName);
    if (!existing.isEmpty()) {
        IEditor *first = existing.first();
        if (!first->duplicateSupported())
            return 0;
        editor = first->duplicate(this);
        registerDuplicate(first, editor);
    } else {
        editor = createEditor(editorKind, fileName);
        if (!editor || !editor->open(fileName))
            return 0;
    }
    insertEditor(editor, false, group);
    restoreEditorState(editor);
    return editor;
}

void EditorManager::revertToSaved()
{
    IEditor *currEditor = currentEditor();
    if (!currEditor)
        return;
    const QString fileName =  currEditor->file()->fileName();
    if (fileName.isEmpty())
        return;
    if (currEditor->file()->isModified()) {
        QMessageBox msgBox(QMessageBox::Question, tr("Revert to Saved"),
                           tr("You will lose your current changes if you proceed reverting %1.").arg(fileName),
                           QMessageBox::Yes|QMessageBox::No, m_d->m_core->mainWindow());
        msgBox.button(QMessageBox::Yes)->setText(tr("Proceed"));
        msgBox.button(QMessageBox::No)->setText(tr("Cancel"));
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;

    }
    IFile::ReloadBehavior temp = IFile::ReloadAll;
    currEditor->file()->modified(&temp);
}


void EditorManager::showEditorInfoBar(const QString &kind,
                                      const QString &infoText,
                                      const QString &buttonText,
                                      QObject *object, const char *member)
{
    m_d->m_splitter->currentGroup()->showEditorInfoBar(kind, infoText, buttonText, object, member);
}


void EditorManager::hideEditorInfoBar(const QString &kind)
{
    m_d->m_splitter->currentGroup()->hideEditorInfoBar(kind);
}

QString EditorManager::externalEditorHelpText() const
{
    QString help = tr(
            "<table border=1 cellspacing=0 cellpadding=3>"
            "<tr><th>Variable</th><th>Expands to</th></tr>"
            "<tr><td>%f</td><td>file name</td></tr>"
            "<tr><td>%l</td><td>current line number</td></tr>"
            "<tr><td>%c</td><td>current column number</td></tr>"
            "<tr><td>%x</td><td>editor's x position on screen</td></tr>"
            "<tr><td>%y</td><td>editor's y position on screen</td></tr>"
            "<tr><td>%w</td><td>editor's width in pixels</td></tr>"
            "<tr><td>%h</td><td>editor's height in pixels</td></tr>"
            "<tr><td>%W</td><td>editor's width in characters</td></tr>"
            "<tr><td>%H</td><td>editor's height in characters</td></tr>"
            "<tr><td>%%</td><td>%</td></tr>"
            "</table>");
    return help;
}

void EditorManager::openInExternalEditor()
{
    QString command = m_d->m_externalEditor;
    if (command.isEmpty())
        command = defaultExternalEditor();

    if (command.isEmpty())
        return;

    IEditor *editor = currentEditor();
    if (!editor)
        return;
    if (editor->file()->isModified()) {
        bool cancelled = false;
        QList<IFile*> list = ICore::instance()->fileManager()->
                             saveModifiedFiles(QList<IFile*>() << editor->file(), &cancelled);
        if (cancelled)
            return;
    }

    QRect rect = editor->widget()->rect();
    QFont font = editor->widget()->font();
    QFontMetrics fm(font);
    rect.moveTo(editor->widget()->mapToGlobal(QPoint(0,0)));

    QString pre = command;
    QString cmd;
    for (int i = 0; i < pre.size(); ++i) {
        QChar c = pre.at(i);
        if (c == QLatin1Char('%') && i < pre.size()-1) {
            c = pre.at(++i);
            QString s;
            if (c == QLatin1Char('f'))
                s = editor->file()->fileName();
            else if (c == QLatin1Char('l'))
                s = QString::number(editor->currentLine());
            else if (c == QLatin1Char('c'))
                s = QString::number(editor->currentColumn());
            else if (c == QLatin1Char('x'))
                s = QString::number(rect.x());
            else if (c == QLatin1Char('y'))
                s = QString::number(rect.y());
            else if (c == QLatin1Char('w'))
                s = QString::number(rect.width());
            else if (c == QLatin1Char('h'))
                s = QString::number(rect.height());
            else if (c == QLatin1Char('W'))
                s = QString::number(rect.width() / fm.width(QLatin1Char('x')));
            else if (c == QLatin1Char('H'))
                s = QString::number(rect.height() / fm.lineSpacing());
            else if (c == QLatin1Char('%'))
                s = c;
            else {
                s = QLatin1Char('%');
                cmd += c;
            }
            cmd += s;
            continue;

        }
        cmd += c;
    }

    QProcess::startDetached(cmd);
}

void EditorManager::setExternalEditor(const QString &editor)
{
    if (editor.isEmpty() || editor == defaultExternalEditor())
        m_d->m_externalEditor = defaultExternalEditor();
    else
        m_d->m_externalEditor = editor;
}

QString EditorManager::externalEditor() const
{
    if (m_d->m_externalEditor.isEmpty())
        return defaultExternalEditor();
    return m_d->m_externalEditor;
}

//===================EditorClosingCoreListener======================

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
