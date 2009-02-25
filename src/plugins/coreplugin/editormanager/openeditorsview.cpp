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

#include "openeditorsview.h"
#include "editorgroup.h"
#include "editormanager.h"
#include "icore.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#ifdef Q_WS_MAC
#include <qmacstyle_mac.h>
#endif

Q_DECLARE_METATYPE(Core::IEditor*)

using namespace Core;
using namespace Core::Internal;

OpenEditorsWidget::OpenEditorsWidget()
{
    m_ui.setupUi(this);
    setWindowTitle(tr("Open Documents"));
    setWindowIcon(QIcon(Constants::ICON_DIR));
    setFocusProxy(m_ui.editorList);
    m_ui.editorList->setColumnCount(1);
    m_ui.editorList->header()->hide();
    m_ui.editorList->setIndentation(0);
    m_ui.editorList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_ui.editorList->setTextElideMode(Qt::ElideMiddle);
    m_ui.editorList->installEventFilter(this);
    m_ui.editorList->setFrameStyle(QFrame::NoFrame);
    m_ui.editorList->setAttribute(Qt::WA_MacShowFocusRect, false);
    EditorManager *em = EditorManager::instance();
    foreach (IEditor *editor, em->openedEditors()) {
        registerEditor(editor);
    }
    connect(em, SIGNAL(editorOpened(Core::IEditor*)),
            this, SLOT(registerEditor(Core::IEditor*)));
    connect(em, SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(unregisterEditors(QList<Core::IEditor*>)));
    connect(em, SIGNAL(editorGroupsChanged()),
            this, SLOT(updateEditorList()));
    connect(em, SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(updateCurrentItem()));
    connect(m_ui.editorList, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
            this, SLOT(selectEditor(QTreeWidgetItem*)));
    updateEditorList();
}

OpenEditorsWidget::~OpenEditorsWidget()
{
    
}

void OpenEditorsWidget::registerEditor(IEditor *editor)
{
    connect(editor, SIGNAL(changed()), this, SLOT(updateEditor()));
    updateEditorList();
}

void OpenEditorsWidget::unregisterEditors(QList<IEditor *> editors)
{
    foreach (IEditor *editor, editors)
        disconnect(editor, SIGNAL(changed()), this, SLOT(updateEditor()));
    updateEditorList();
}

void OpenEditorsWidget::updateEditorList()
{
    EditorManager *em = EditorManager::instance();
    QList<EditorGroup *> groups = em->editorGroups();
    IEditor *curEditor = em->currentEditor();
    int oldNum = m_ui.editorList->topLevelItemCount();
    QTreeWidgetItem *currentItem = 0;
    int currItemIndex = 0;
    for (int i = 0; i < groups.count(); ++i) {
        QTreeWidgetItem *item;
        if (groups.count() > 1) {
            if (currItemIndex < oldNum) {
                item = m_ui.editorList->topLevelItem(currItemIndex);
            } else {
                item = new QTreeWidgetItem(QStringList()<<"");
                m_ui.editorList->addTopLevelItem(item);
            }
            currItemIndex++;
            item->setIcon(0, QIcon());
            item->setText(0, tr("---Group %1---").arg(i));
            item->setFlags(0);
            item->setToolTip(0, "");
            item->setData(0, Qt::UserRole, QVariant());
            item->setTextAlignment(0, Qt::AlignLeft);
        }
        foreach (IEditor *editor, groups.at(i)->editors()) {
            if (currItemIndex < oldNum) {
                item = m_ui.editorList->topLevelItem(currItemIndex);
            } else {
                item = new QTreeWidgetItem(QStringList()<<"");
                m_ui.editorList->addTopLevelItem(item);
            }
            currItemIndex++;
            updateItem(item, editor);
            if (editor == curEditor)
                currentItem = item;
        }
    }
    for (int i = oldNum-1; i >= currItemIndex; --i) {
        delete m_ui.editorList->takeTopLevelItem(i);
    }
    updateCurrentItem(currentItem);
}

void OpenEditorsWidget::updateCurrentItem(QTreeWidgetItem *currentItem)
{
    EditorManager *em = EditorManager::instance();
    IEditor *curEditor = em->currentEditor();
    m_ui.editorList->clearSelection();
    if (!currentItem && curEditor) {
        int count = m_ui.editorList->topLevelItemCount();
        for (int i = 0; i < count; ++i) {
            if (m_ui.editorList->topLevelItem(i)->data(0, Qt::UserRole).value<IEditor *>()
                    == curEditor) {
                currentItem = m_ui.editorList->topLevelItem(i);
                break;
            }
        }
    }
    m_ui.editorList->setCurrentItem(currentItem);
    if (currentItem)
        m_ui.editorList->scrollTo(m_ui.editorList->currentIndex());
}

//todo: this is almost duplicated in openeditorswindow
void OpenEditorsWidget::updateItem(QTreeWidgetItem *item, IEditor *editor)
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

void OpenEditorsWidget::selectEditor(QTreeWidgetItem *item)
{
    if (item == 0)
        item = m_ui.editorList->currentItem();
    if (item == 0)
        return;
    IEditor *editor = item->data(0, Qt::UserRole).value<IEditor*>();
    EditorManager::instance()->setCurrentEditor(editor);
}

void OpenEditorsWidget::updateEditor()
{
    IEditor *editor = qobject_cast<IEditor *>(sender());
    QTC_ASSERT(editor, return);
    int num = m_ui.editorList->topLevelItemCount();
    for (int i = 0; i < num; ++i) {
        QTreeWidgetItem *item = m_ui.editorList->topLevelItem(i);
        if (item->data(0, Qt::UserRole).value<IEditor *>()
                == editor) {
            updateItem(item, editor);
            return;
        }
    }
}

void OpenEditorsWidget::closeEditors()
{
    QList<IFile *> selectedFiles;
    QList<IEditor *> selectedEditors;
    foreach (QTreeWidgetItem *item, m_ui.editorList->selectedItems()) {
        selectedEditors.append(item->data(0, Qt::UserRole).value<IEditor *>());
        selectedFiles.append(item->data(0, Qt::UserRole).value<IEditor *>()->file());
    }
    ICore *core = ICore::instance();
    bool cancelled = false;
    core->fileManager()->saveModifiedFiles(selectedFiles, &cancelled);
    if (cancelled)
        return;
    core->editorManager()->closeEditors(selectedEditors);
    updateEditorList();
}

void OpenEditorsWidget::closeAllEditors()
{
    m_ui.editorList->selectAll();
    closeEditors();
}


bool OpenEditorsWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_ui.editorList) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            switch (keyEvent->key()) {
                case Qt::Key_Return:
                    selectEditor(m_ui.editorList->currentItem());
                    return true;
                case Qt::Key_Delete: //fall through
                case Qt::Key_Backspace:
                    if (keyEvent->modifiers() == Qt::NoModifier) {
                        closeEditors();
                        return true;
                    }
                    break;
                default:
                    break;
            }
        }
        if (event->type() == QEvent::ContextMenu) {
            QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent *>(event);
            QMenu menu;
            menu.addAction(tr("&Select"), this, SLOT(selectEditor()));
            menu.addAction(tr("&Close"), this, SLOT(closeEditors()));
            menu.addAction(tr("Close &All"), this, SLOT(closeAllEditors()));
            if (m_ui.editorList->selectedItems().isEmpty())
                menu.setEnabled(false);
            menu.exec(contextMenuEvent->globalPos());
            return true;
        }
    } else if (obj == m_widget) {
        if (event->type() == QEvent::FocusIn) {
            QFocusEvent *e = static_cast<QFocusEvent *>(event);
            if (e->reason() != Qt::MouseFocusReason) {
                // we should not set the focus in a event filter for a focus event,
                // so do it when the focus event is processed
                QTimer::singleShot(0, this, SLOT(putFocusToEditorList()));
            }
        }
    }
    return false;
}

void OpenEditorsWidget::putFocusToEditorList()
{
    m_ui.editorList->setFocus();
}

NavigationView OpenEditorsViewFactory::createWidget()
{
    NavigationView n;
    n.widget = new OpenEditorsWidget();
    return n;
}

QString OpenEditorsViewFactory::displayName()
{
    return "Open Documents";
}

QKeySequence OpenEditorsViewFactory::activationSequence()
{
    return QKeySequence(Qt::ALT + Qt::Key_O);
}

OpenEditorsViewFactory::OpenEditorsViewFactory()
{
}

OpenEditorsViewFactory::~OpenEditorsViewFactory()
{
}
