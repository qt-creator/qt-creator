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

#include "editorsplitter.h"

#include "coreconstants.h"
#include "editormanager.h"
#include "icore.h"
#include "minisplitter.h"
#include "openeditorswindow.h"
#include "stackededitorgroup.h"
#include "uniqueidmanager.h"
#include "actionmanager/actionmanager.h"

#include <utils/qtcassert.h>

#include <QtGui/QHBoxLayout>
#include <QtGui/QMenu>
#include <QtGui/QApplication>

using namespace Core;
using namespace Core::Internal;

EditorSplitter::EditorSplitter(QWidget *parent)
  : QWidget(parent),
    m_curGroup(0)
{
    registerActions();
    createRootGroup();
    updateActions();
}

EditorSplitter::~EditorSplitter()
{
}

void EditorSplitter::registerActions()
{
    QList<int> gc = QList<int>() << Constants::C_GLOBAL_ID;
    const QList<int> editorManagerContext =
            QList<int>() << ICore::instance()->uniqueIDManager()->uniqueIdentifier(Constants::C_EDITORMANAGER);

    ActionManager *am = ICore::instance()->actionManager();
    ActionContainer *mwindow = am->actionContainer(Constants::M_WINDOW);
    Command *cmd;

    //Horizontal Action
    m_horizontalSplitAction = new QAction(tr("Split Left/Right"), this);
    cmd = am->registerAction(m_horizontalSplitAction, Constants::HORIZONTAL, editorManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_horizontalSplitAction, SIGNAL(triggered()),
            this, SLOT(splitHorizontal()));

    //Vertical Action
    m_verticalSplitAction = new QAction(tr("Split Top/Bottom"), this);
    cmd = am->registerAction(m_verticalSplitAction, Constants::VERTICAL, editorManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_verticalSplitAction, SIGNAL(triggered()),
            this, SLOT(splitVertical()));

    //Unsplit Action
    m_unsplitAction = new QAction(tr("Unsplit"), this);
    cmd = am->registerAction(m_unsplitAction, Constants::REMOVE, editorManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_SPLIT);
    connect(m_unsplitAction, SIGNAL(triggered()),
            this, SLOT(unsplit()));

    //Default Layout menu
    ActionContainer *mLayout = am->createMenu("QtCreator.Menu.Window.Layout");
    mwindow->addMenu(mLayout, Constants::G_WINDOW_SPLIT);
    mLayout->menu()->setTitle(tr("Default Splitter Layout"));

    //Set Current As Default
    m_currentAsDefault = new QAction(tr("Save Current as Default"), this);
    cmd = am->registerAction(m_currentAsDefault, Constants::SAVEASDEFAULT, editorManagerContext);
    mLayout->addAction(cmd);
    connect(m_currentAsDefault, SIGNAL(triggered()),
            this, SLOT(saveCurrentLayout()));

    //Restore Default
    m_restoreDefault = new QAction(tr("Restore Default Layout"), this);
    cmd = am->registerAction(m_restoreDefault, Constants::RESTOREDEFAULT, editorManagerContext);
    mLayout->addAction(cmd);
    connect(m_restoreDefault, SIGNAL(triggered()),
            this, SLOT(restoreDefaultLayout()));

    // TODO: The previous and next actions are removed, to be reenabled when they
    // navigate according to code navigation history. And they need different shortcuts on the mac
    // since Alt+Left/Right is jumping wordwise in editors
#if 0
    // Goto Previous Action
    m_gotoPreviousEditorAction = new QAction(QIcon(Constants::ICON_PREV), tr("Previous Document"), this);
    cmd = am->registerAction(m_gotoPreviousEditorAction, Constants::GOTOPREV, editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Left")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_gotoPreviousEditorAction, SIGNAL(triggered()), this, SLOT(gotoPreviousEditor()));

    // Goto Next Action
    m_gotoNextEditorAction = new QAction(QIcon(Constants::ICON_NEXT), tr("Next Document"), this);
    cmd = am->registerAction(m_gotoNextEditorAction, Constants::GOTONEXT, editorManagerContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Alt+Right")));
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE);
    connect(m_gotoNextEditorAction, SIGNAL(triggered()), this, SLOT(gotoNextEditor()));
#endif

    // Previous Group Action
    m_gotoPreviousGroupAction = new QAction(tr("Previous Group"), this);
    cmd = am->registerAction(m_gotoPreviousGroupAction, Constants::GOTOPREVIOUSGROUP, editorManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE_GROUPS);
    connect(m_gotoPreviousGroupAction, SIGNAL(triggered()), this, SLOT(selectPreviousGroup()));

    // Next Group Action
    m_gotoNextGroupAction = new QAction(tr("Next Group"), this);
    cmd = am->registerAction(m_gotoNextGroupAction, Constants::GOTONEXTGROUP, editorManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE_GROUPS);
    connect(m_gotoNextGroupAction, SIGNAL(triggered()), this, SLOT(selectNextGroup()));

    // Move to Previous Group
    m_moveDocToPreviousGroupAction = new QAction(tr("Move Document to Previous Group"), this);
    cmd = am->registerAction(m_moveDocToPreviousGroupAction, "QtCreator.DocumentToPreviousGroup", editorManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE_GROUPS);
    connect(m_moveDocToPreviousGroupAction, SIGNAL(triggered()), this, SLOT(moveDocToPreviousGroup()));

    // Move to Next Group
    m_moveDocToNextGroupAction = new QAction(tr("Move Document to Next Group"), this);
    cmd = am->registerAction(m_moveDocToNextGroupAction, "QtCreator.DocumentToNextGroup", editorManagerContext);
    mwindow->addAction(cmd, Constants::G_WINDOW_NAVIGATE_GROUPS);
    connect(m_moveDocToNextGroupAction, SIGNAL(triggered()), this, SLOT(moveDocToNextGroup()));
}

void EditorSplitter::updateActions()
{
    const bool hasMultipleGroups = (qobject_cast<QSplitter*>(m_root) != 0);
    QTC_ASSERT(currentGroup(), return);
    const bool hasEditors = (currentGroup()->editorCount() != 0);
    m_unsplitAction->setEnabled(hasMultipleGroups);
#if 0
    const bool hasMultipleEditors = (editorCount() > 1);
    m_gotoNextEditorAction->setEnabled(hasMultipleEditors);
    m_gotoPreviousEditorAction->setEnabled(hasMultipleEditors);
#endif
    m_gotoPreviousGroupAction->setEnabled(hasMultipleGroups);
    m_gotoNextGroupAction->setEnabled(hasMultipleGroups);
    m_moveDocToPreviousGroupAction->setEnabled(hasEditors && hasMultipleGroups);
    m_moveDocToNextGroupAction->setEnabled(hasEditors && hasMultipleGroups);
}

int EditorSplitter::editorCount() const
{
    int count = 0;
    foreach (EditorGroup *group, groups()) {
        count += group->editorCount();
    }
    return count;
}

void EditorSplitter::createRootGroup()
{
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setMargin(0);
    l->setSpacing(0);
    EditorGroup *rootGroup = createGroup();
    m_root = rootGroup->widget();
    l->addWidget(m_root);
    setCurrentGroup(rootGroup);
}

void EditorSplitter::splitHorizontal()
{
    split(Qt::Horizontal);
}

void EditorSplitter::splitVertical()
{
    split(Qt::Vertical);
}

void EditorSplitter::gotoNextEditor()
{
    OpenEditorsWindow *dialog = EditorManager::instance()->windowPopup();
    dialog->setMode(OpenEditorsWindow::ListMode);
    dialog->selectNextEditor();
    EditorManager::instance()->showWindowPopup();
}

void EditorSplitter::gotoPreviousEditor()
{
    OpenEditorsWindow *dialog = EditorManager::instance()->windowPopup();
    dialog->setMode(OpenEditorsWindow::ListMode);
    dialog->selectPreviousEditor();
    EditorManager::instance()->showWindowPopup();
}

void EditorSplitter::setCurrentGroup(EditorGroup *group)
{
    if (!group || group == m_curGroup)
        return;
    m_curGroup = group;
    if (m_curGroup->widget()->focusWidget() != QApplication::focusWidget())
        m_curGroup->widget()->setFocus();
    updateActions();
}

QList<EditorGroup*> EditorSplitter::groups() const
{
    QList<EditorGroup*> grps;
    collectGroups(m_root, grps);
    return grps;
}

void EditorSplitter::collectGroups(QWidget *widget, QList<EditorGroup*> &groups) const
{
    EditorGroup *group = qobject_cast<EditorGroup *>(widget);
    if (group) {
        groups += group;
        return;
    }
    QSplitter *splitter = qobject_cast<QSplitter*>(widget);
    QTC_ASSERT(splitter, return);
    collectGroups(splitter->widget(LEFT), groups);
    collectGroups(splitter->widget(RIGHT), groups);
}


EditorGroup *EditorSplitter::currentGroup() const
{
    return m_curGroup;
}

void EditorSplitter::split(Qt::Orientation orientation)
{
    EditorGroup *curGroup = currentGroup();
    IEditor *editor = curGroup->currentEditor();
    split(orientation, 0);
    if (editor) {
        EditorGroup *otherGroup = currentGroup();
        if (curGroup != otherGroup) {
            curGroup->removeEditor(editor);
            otherGroup->addEditor(editor);
        }
    }
    emit editorGroupsChanged();
}

QSplitter *EditorSplitter::split(Qt::Orientation orientation, EditorGroup *group)
{
    EditorGroup *curGroup = group;
    if (!curGroup)
        curGroup = currentGroup();
    if (!curGroup)
        return 0;

    int oldSibling1 = -1;
    int oldSibling2 = -1;
    int idx = 1;
    QWidget *curGroupWidget = curGroup->widget();
    QSplitter *parentSplitter =  qobject_cast<QSplitter*>(curGroupWidget->parentWidget());
    if (parentSplitter) {
        if (parentSplitter->orientation() == Qt::Vertical) {
            oldSibling1 = parentSplitter->widget(LEFT)->height();
            oldSibling2 = parentSplitter->widget(RIGHT)->height();
        } else {
            oldSibling1 = parentSplitter->widget(LEFT)->width();
            oldSibling2 = parentSplitter->widget(RIGHT)->width();
        }
        idx = parentSplitter->indexOf(curGroupWidget);
    }

    QLayout *l = curGroupWidget->parentWidget()->layout();
    curGroupWidget->setParent(0);

    QSplitter *splitter = new MiniSplitter(0);
    splitter->setOrientation(orientation);
    EditorGroup *eg = createGroup();

    splitter->addWidget(curGroupWidget);
    splitter->addWidget(eg->widget());

    if (curGroupWidget == m_root) {
        l->addWidget(splitter);
        m_root = splitter;
    } else {
        parentSplitter->insertWidget(idx, splitter);
    }

    if (parentSplitter)
        parentSplitter->setSizes(QList<int>() << oldSibling1 << oldSibling2);
    if (orientation == Qt::Vertical)
        splitter->setSizes(QList<int>() << curGroupWidget->height()/2
                           << curGroupWidget->height()/2);
    else
        splitter->setSizes(QList<int>() << curGroupWidget->width()/2
                           << curGroupWidget->width()/2);
    setCurrentGroup(eg);
    return splitter;
}

void EditorSplitter::unsplit()
{
    EditorGroup *curGroup = currentGroup();
    if (!curGroup)
        return;
    QWidget *curGroupWidget = curGroup->widget();
    QTC_ASSERT(curGroupWidget, return);
    IEditor *selectedEditor = curGroup->currentEditor();

    QSplitter *parentSplitter = qobject_cast<QSplitter*>(curGroupWidget->parentWidget());
    if (parentSplitter) {
        int oldSibling1 = -1;
        int oldSibling2 = -1;

        EditorGroup *e1 = qobject_cast<EditorGroup *>(parentSplitter->widget(LEFT));
        EditorGroup *e2 = qobject_cast<EditorGroup *>(parentSplitter->widget(RIGHT));

        QWidget *w = parentSplitter->parentWidget();
        QSplitter *grandParentSplitter = qobject_cast<QSplitter*>(w);
        int idx = 0;
        if (grandParentSplitter) {
            idx = grandParentSplitter->indexOf(parentSplitter);
            if (grandParentSplitter->orientation() == Qt::Vertical) {
                oldSibling1 = grandParentSplitter->widget(LEFT)->height();
                oldSibling2 = grandParentSplitter->widget(RIGHT)->height();
            } else {
                oldSibling1 = grandParentSplitter->widget(LEFT)->width();
                oldSibling2 = grandParentSplitter->widget(RIGHT)->width();
            }
        }
        if (e1 && e2) { // we are unsplitting a split that contains of groups directly not one or more additional splits
            e1->moveEditorsFromGroup(e2);
            parentSplitter->setParent(0);
            if (grandParentSplitter) {
                grandParentSplitter->insertWidget(idx, e1->widget());
            } else {
                w->layout()->addWidget(e1->widget());
                m_root = e1->widget();
            }
            setCurrentGroup(e1);
            deleteGroup(e2);
            e2 = 0;
            delete parentSplitter;
            e1->setCurrentEditor(selectedEditor);
        } else if (e1 || e2) {
            parentSplitter->setParent(0);
            QSplitter *s = 0;
            if (e1) {
                s = qobject_cast<QSplitter*>(parentSplitter->widget(RIGHT));
            } else {
                s = qobject_cast<QSplitter*>(parentSplitter->widget(LEFT));
                e1 = e2;
            }

            if (grandParentSplitter) {
                grandParentSplitter->insertWidget(idx, s);
            } else {
                w->layout()->addWidget(s);
                m_root = s;
            }
            EditorGroup *leftMost = groupFarthestOnSide(s, LEFT);
            leftMost->moveEditorsFromGroup(e1);
            leftMost->setCurrentEditor(selectedEditor);
            setCurrentGroup(leftMost);
            deleteGroup(e1);
        }
        if (grandParentSplitter)
            grandParentSplitter->setSizes(QList<int>() << oldSibling1 << oldSibling2);
        emit editorGroupsChanged();
    }
    updateActions();
}

void EditorSplitter::unsplitAll()
{
    QSplitter *rootSplit = qobject_cast<QSplitter *>(m_root);
    if (!rootSplit)
        return;
    // first create and set the new root, then kill the original stuff
    // this way we can avoid unnecessary signals/ context changes
    rootSplit->setParent(0);
    EditorGroup *rootGroup = createGroup();
    QLayout *l = layout();
    l->addWidget(rootGroup->widget());
    m_root = rootGroup->widget();
    setCurrentGroup(rootGroup);
    // kill old hierarchy
    QList<IEditor *> editors;
    unsplitAll(rootSplit->widget(1), editors);
    unsplitAll(rootSplit->widget(0), editors);
    delete rootSplit;
    rootSplit = 0;
    foreach (IEditor *editor, editors) {
        rootGroup->addEditor(editor);
    }
}

void EditorSplitter::unsplitAll(QWidget *node, QList<IEditor*> &editors)
{
    QSplitter *splitter = qobject_cast<QSplitter *>(node);
    if (splitter) {
        unsplitAll(splitter->widget(1), editors);
        unsplitAll(splitter->widget(0), editors);
        delete splitter;
        splitter = 0;
    } else {
        EditorGroup *group = qobject_cast<EditorGroup *>(node);
        editors << group->editors();
        bool blocking = group->widget()->blockSignals(true);
        foreach (IEditor *editor, group->editors()) {
            group->removeEditor(editor);
        }
        group->widget()->blockSignals(blocking);
        deleteGroup(group);
    }
}

EditorGroup *EditorSplitter::groupFarthestOnSide(QWidget *node, Side side) const
{
    QWidget *current = node;
    QSplitter *split = 0;
    while ((split = qobject_cast<QSplitter*>(current))) {
        current = split->widget(side);
    }
    return qobject_cast<EditorGroup *>(current);
}

void EditorSplitter::selectNextGroup()
{
    EditorGroup *curGroup = currentGroup();
    QTC_ASSERT(curGroup, return);
    setCurrentGroup(nextGroup(curGroup, RIGHT));
}

void EditorSplitter::selectPreviousGroup()
{
    EditorGroup *curGroup = currentGroup();
    QTC_ASSERT(curGroup, return);
    setCurrentGroup(nextGroup(curGroup, LEFT));
}

EditorGroup *EditorSplitter::nextGroup(EditorGroup *curGroup, Side side) const
{
    QTC_ASSERT(curGroup, return 0);
    QWidget *curWidget = curGroup->widget();
    QWidget *parent = curWidget->parentWidget();
    while (curWidget != m_root) {
        QSplitter *splitter = qobject_cast<QSplitter *>(parent);
        QTC_ASSERT(splitter, return 0);
        if (splitter->widget(side) != curWidget) {
            curWidget = splitter->widget(side);
            break;
        }
        curWidget = parent;
        parent = curWidget->parentWidget();
    }
    return groupFarthestOnSide(curWidget, side==LEFT ? RIGHT : LEFT);
}

void EditorSplitter::moveDocToAdjacentGroup(Side side)
{
    EditorGroup *curGroup = currentGroup();
    QTC_ASSERT(curGroup, return);
    IEditor *editor = curGroup->currentEditor();
    if (!editor)
        return;
    EditorGroup *next = nextGroup(curGroup, side);
    next->moveEditorFromGroup(curGroup, editor);
    setCurrentGroup(next);
}

void EditorSplitter::moveDocToNextGroup()
{
    moveDocToAdjacentGroup(RIGHT);
}

void EditorSplitter::moveDocToPreviousGroup()
{
    moveDocToAdjacentGroup(LEFT);
}

QWidget *EditorSplitter::recreateGroupTree(QWidget *node)
{
    QSplitter *splitter = qobject_cast<QSplitter *>(node);
    if (!splitter) {
        EditorGroup *group = qobject_cast<EditorGroup *>(node);
        QTC_ASSERT(group, return 0);
        IEditor *currentEditor = group->currentEditor();
        EditorGroup *newGroup = createGroup();
        bool block = newGroup->widget()->blockSignals(true);
        foreach (IEditor *editor, group->editors()) {
            newGroup->addEditor(editor);
        }
        newGroup->setCurrentEditor(currentEditor);
        deleteGroup(group);
        newGroup->widget()->blockSignals(block);
        return newGroup->widget();
    } else {
        QByteArray splitterState = splitter->saveState();
        QWidget *orig0 = splitter->widget(0);
        QWidget *orig1 = splitter->widget(1);
        QWidget *g0 = recreateGroupTree(orig0);
        QWidget *g1 = recreateGroupTree(orig1);
        splitter->insertWidget(0, g0);
        splitter->insertWidget(1, g1);
        splitter->restoreState(splitterState);
        return node;
    }
}

void EditorSplitter::saveCurrentLayout()
{
    QSettings *settings = ICore::instance()->settings();
    settings->setValue("EditorManager/Splitting", saveState());
}

void EditorSplitter::restoreDefaultLayout()
{
    QSettings *settings = ICore::instance()->settings();
    if (settings->contains("EditorManager/Splitting"))
        restoreState(settings->value("EditorManager/Splitting").toByteArray());
}

void EditorSplitter::saveSettings(QSettings * /*settings*/) const
{
}

void EditorSplitter::readSettings(QSettings * /*settings*/)
{
    restoreDefaultLayout();
}

QByteArray EditorSplitter::saveState() const
{
    //todo: versioning
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    saveState(m_root, stream);
    return bytes;
}

bool EditorSplitter::restoreState(const QByteArray &state)
{
    unsplitAll();
    //todo: versioning
    QDataStream stream(state);
    EditorGroup *curGroup =
            restoreState(qobject_cast<EditorGroup *>(m_root), stream);
    setCurrentGroup(curGroup);
    return true;
}

void EditorSplitter::saveState(QWidget *current, QDataStream &stream) const
{
    QSplitter *splitter = qobject_cast<QSplitter *>(current);
    quint8 type;
    if (splitter) {
        type = 0;
        stream << type;
        stream << splitter->saveState();
        saveState(splitter->widget(0), stream);
        saveState(splitter->widget(1), stream);
    } else {
        EditorGroup *group = qobject_cast<EditorGroup *>(current);
        QTC_ASSERT(group, /**/);
        if (group != currentGroup())
            type = 1;
        else
            type = 2;
        stream << type;
    }
}

EditorGroup *EditorSplitter::restoreState(EditorGroup *current,
    QDataStream &stream)
{
    EditorGroup *curGroup = 0;
    EditorGroup *group;
    quint8 type;
    stream >> type;
    if (type == 0) {
        QSplitter *splitter = split(Qt::Horizontal, current);
        QByteArray splitterState;
        stream >> splitterState;
        splitter->restoreState(splitterState);
        group = restoreState(qobject_cast<EditorGroup *>(splitter->widget(0)),
            stream);
        if (group)
            curGroup = group;
        group = restoreState(qobject_cast<EditorGroup *>(splitter->widget(1)),
            stream);
        if (group)
            curGroup = group;
    } else {
        if (type == 2)
            return current;
    }
    return curGroup;
}

QMap<QString,EditorGroup *> EditorSplitter::pathGroupMap()
{
    QMap<QString,EditorGroup *> map;
    fillPathGroupMap(m_root, "", map);
    return map;
}

void EditorSplitter::fillPathGroupMap(QWidget *current, QString currentPath,
                      QMap<QString,EditorGroup *> &map)
{
    EditorGroup *group = qobject_cast<EditorGroup *>(current);
    if (group) {
        map.insert(currentPath, group);
    } else {
        QSplitter *splitter = qobject_cast<QSplitter *>(current);
        QTC_ASSERT(splitter, return);
        fillPathGroupMap(splitter->widget(0), currentPath+"0", map);
        fillPathGroupMap(splitter->widget(1), currentPath+"1", map);
    }
}

EditorGroup *EditorSplitter::createGroup()
{
    EditorGroup *group = new StackedEditorGroup(this);
    connect(group, SIGNAL(closeRequested(Core::IEditor *)),
            this, SIGNAL(closeRequested(Core::IEditor *)));
    connect(group, SIGNAL(editorRemoved(Core::IEditor *)),
            this, SLOT(updateActions()));
    connect(group, SIGNAL(editorAdded(Core::IEditor *)),
            this, SLOT(updateActions()));
    ICore::instance()->addContextObject(group->contextObject());
    return group;
}

void EditorSplitter::deleteGroup(EditorGroup *group)
{
    ICore::instance()->removeContextObject(group->contextObject());
    delete group;
}
