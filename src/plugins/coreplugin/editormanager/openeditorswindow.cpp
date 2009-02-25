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

#include "openeditorswindow.h"
#include "editorgroup.h"
#include "editormanager.h"

#include <QtGui/QHeaderView>

Q_DECLARE_METATYPE(Core::IEditor *)

using namespace Core;
using namespace Core::Internal;

const int OpenEditorsWindow::WIDTH = 300;
const int OpenEditorsWindow::HEIGHT = 200;
const int OpenEditorsWindow::MARGIN = 4;

OpenEditorsWindow::OpenEditorsWindow(QWidget *parent) :
    QWidget(parent, Qt::Popup),
    m_editorList(new QTreeWidget(this)),
    m_mode(HistoryMode),
    m_current(0)
{
    resize(QSize(WIDTH, HEIGHT));
    m_editorList->setColumnCount(1);
    m_editorList->header()->hide();
    m_editorList->setIndentation(0);
    m_editorList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorList->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_editorList->setTextElideMode(Qt::ElideMiddle);
#ifdef Q_WS_MAC
    m_editorList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
#endif
    m_editorList->installEventFilter(this);
    m_editorList->setGeometry(MARGIN, MARGIN, WIDTH-2*MARGIN, HEIGHT-2*MARGIN);

    connect(m_editorList, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
            this, SLOT(editorClicked(QTreeWidgetItem*)));

    m_autoHide.setSingleShot(true);
    connect(&m_autoHide, SIGNAL(timeout()), this, SLOT(selectAndHide()));
    EditorManager *em = EditorManager::instance();
    connect(em, SIGNAL(editorOpened(Core::IEditor *)),
            this, SLOT(updateEditorList()));
    connect(em, SIGNAL(editorsClosed(QList<Core::IEditor *>)),
            this, SLOT(updateEditorList()));
    connect(em, SIGNAL(editorGroupsChanged()),
            this, SLOT(updateEditorList()));
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateEditorList()));
}

void OpenEditorsWindow::selectAndHide()
{
    selectEditor(m_editorList->currentItem());
    setVisible(false);
}

void OpenEditorsWindow::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible) {
        updateEditorList(m_current);
        m_autoHide.start(600);
        setFocus();
    }
}

bool OpenEditorsWindow::isCentering()
{
    if (m_mode == OpenEditorsWindow::HistoryMode || m_editorList->topLevelItemCount() < 3)
        return false;
    int internalMargin = m_editorList->viewport()->mapTo(m_editorList, QPoint(0,0)).y();
    QRect rect0 = m_editorList->visualItemRect(m_editorList->topLevelItem(0));
    QRect rect1 = m_editorList->visualItemRect(m_editorList->topLevelItem(m_editorList->topLevelItemCount()-1));
    int height = rect1.y() + rect1.height() - rect0.y();
    height += 2*internalMargin + 2*MARGIN;
    if (height > HEIGHT)
        return true;
    return false;
}

void OpenEditorsWindow::setMode(Mode mode)
{
    m_mode = mode;
    updateEditorList(m_current);
}

bool OpenEditorsWindow::event(QEvent *e) {
    if (e->type() == QEvent::KeyRelease) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        m_autoHide.stop();
        if (ke->modifiers() == 0
            /*HACK this is to overcome some event inconsistencies between platforms*/
            || (ke->modifiers() == Qt::AltModifier && (ke->key() == Qt::Key_Alt || ke->key() == -1))) {
            selectAndHide();
        }
    }
    return QWidget::event(e);
}

bool OpenEditorsWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_editorList) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Escape) {
                m_current = EditorManager::instance()->currentEditor();
                updateSelectedEditor();
                setVisible(false);
                return true;
            }
            if (ke->key() == Qt::Key_Return) {
                selectEditor(m_editorList->currentItem());
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, e);
}

void OpenEditorsWindow::focusInEvent(QFocusEvent *)
{
    m_editorList->setFocus();
}

void OpenEditorsWindow::selectUpDown(bool up)
{
    int itemCount = m_editorList->topLevelItemCount();
    if (itemCount < 2)
        return;
    int index = m_editorList->indexOfTopLevelItem(m_editorList->currentItem());
    if (index < 0)
        return;
    IEditor *editor = 0;
    int count = 0;
    while (!editor && count < itemCount) {
        if (up) {
            index--;
            if (index < 0)
                index = itemCount-1;
        } else {
            index++;
            if (index >= itemCount)
                index = 0;
        }
        editor = m_editorList->topLevelItem(index)
            ->data(0, Qt::UserRole).value<IEditor *>();
        count++;
    }
    if (editor)
        updateEditorList(editor);
}

void OpenEditorsWindow::selectPreviousEditor()
{
    selectUpDown(m_mode == ListMode);
}

void OpenEditorsWindow::selectNextEditor()
{
    selectUpDown(m_mode != ListMode);
}

void OpenEditorsWindow::updateEditorList(IEditor *editor)
{
    if (!editor)
        editor = EditorManager::instance()->currentEditor();
    m_current = editor;
    if (m_mode == ListMode)
        updateList();
    else if (m_mode == HistoryMode)
        updateHistory();
}

void OpenEditorsWindow::updateHistory()
{
    EditorManager *em = EditorManager::instance();
    QList<IEditor *> history = em->editorHistory();
    int oldNum = m_editorList->topLevelItemCount();
    int num = history.count();
    int common = qMin(oldNum, num);
    int selectedIndex = -1;
    QTreeWidgetItem *item;
    for (int i = 0; i < common; ++i) {
        item = m_editorList->topLevelItem(i);
        updateItem(item, history.at(i));
        if (history.at(i) == m_current)
            selectedIndex = i;
    }
    for (int i = common; i < num; ++i) {
        item = new QTreeWidgetItem(QStringList() << "");
        updateItem(item, history.at(i));
        m_editorList->addTopLevelItem(item);
        if (history.at(i) == m_current)
            selectedIndex = i;
    }
    for (int i = oldNum-1; i >= common; --i) {
        delete m_editorList->takeTopLevelItem(i);
    }
    if (isCentering())
        centerOnItem(selectedIndex);
    updateSelectedEditor();
}

void OpenEditorsWindow::updateList()
{
    EditorManager *em = EditorManager::instance();
    QList<EditorGroup *> groups = em->editorGroups();
    int oldNum = m_editorList->topLevelItemCount();
    int curItem = 0;
    int selectedIndex = -1;
    QTreeWidgetItem *item;
    for (int i = 0; i < groups.count(); ++i) {
        if (groups.count() > 1) {
            if (curItem < oldNum) {
                item = m_editorList->topLevelItem(curItem);
            } else {
                item = new QTreeWidgetItem(QStringList()<<"");
                m_editorList->addTopLevelItem(item);
            }
            curItem++;
            item->setText(0, tr("---Group %1---").arg(i));
            item->setFlags(0);
            item->setData(0, Qt::UserRole, QVariant());
        }
        foreach (IEditor *editor, groups.at(i)->editors()) {
            if (curItem < oldNum) {
                item = m_editorList->topLevelItem(curItem);
            } else {
                item = new QTreeWidgetItem(QStringList()<<"");
                m_editorList->addTopLevelItem(item);
            }
            updateItem(item, editor);
            if (editor == m_current) {
                m_editorList->setCurrentItem(item);
                selectedIndex = curItem;
            }
            curItem++;
        }
    }
    for (int i = oldNum-1; i >= curItem; --i) {
        delete m_editorList->takeTopLevelItem(i);
    }
    if (isCentering())
        centerOnItem(selectedIndex);
    if (m_current == 0 && m_editorList->currentItem())
        m_editorList->currentItem()->setSelected(false);
    m_editorList->scrollTo(m_editorList->currentIndex(), QAbstractItemView::PositionAtCenter);
}

void OpenEditorsWindow::centerOnItem(int selectedIndex)
{
    if (selectedIndex >= 0) {
        QTreeWidgetItem *item;
        int num = m_editorList->topLevelItemCount();
        int rotate = selectedIndex-(num-1)/2;
        for (int i = 0; i < rotate; ++i) {
            item = m_editorList->takeTopLevelItem(0);
            m_editorList->addTopLevelItem(item);
        }
        rotate = -rotate;
        for (int i = 0; i < rotate; ++i) {
            item = m_editorList->takeTopLevelItem(num-1);
            m_editorList->insertTopLevelItem(0, item);
        }
    }
}

void OpenEditorsWindow::updateItem(QTreeWidgetItem *item, IEditor *editor)
{
    static const QIcon lockedIcon(QLatin1String(":/core/images/locked.png"));
    static const QIcon emptyIcon(QLatin1String(":/core/images/empty14.png"));

    QString title = editor->displayName();
    if (editor->file()->isModified())
        title += tr("*");
    item->setIcon(0, editor->file()->isReadOnly() ? lockedIcon : emptyIcon);
    item->setText(0, title);
    item->setToolTip(0, editor->file()->fileName());
    item->setData(0, Qt::UserRole, QVariant::fromValue(editor));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setTextAlignment(0, Qt::AlignLeft);
}

void OpenEditorsWindow::selectEditor(QTreeWidgetItem *item)
{
    IEditor *editor = 0;
    if (item)
        editor = item->data(0, Qt::UserRole).value<IEditor*>();
    EditorManager::instance()->setCurrentEditor(editor);
}

void OpenEditorsWindow::editorClicked(QTreeWidgetItem *item)
{
    selectEditor(item);
    setFocus();
}

void OpenEditorsWindow::updateSelectedEditor()
{
    if (m_current == 0 && m_editorList->currentItem()) {
        m_editorList->currentItem()->setSelected(false);
        return;
    }
    int num = m_editorList->topLevelItemCount();
    for (int i = 0; i < num; ++i) {
        IEditor *editor = m_editorList->topLevelItem(i)
                                  ->data(0, Qt::UserRole).value<IEditor *>();
        if (editor == m_current) {
            m_editorList->setCurrentItem(m_editorList->topLevelItem(i));
            break;
        }
    }
    m_editorList->scrollTo(m_editorList->currentIndex(), QAbstractItemView::PositionAtCenter);
}

void OpenEditorsWindow::setSelectedEditor(IEditor *editor)
{
    updateEditorList(editor);
}
