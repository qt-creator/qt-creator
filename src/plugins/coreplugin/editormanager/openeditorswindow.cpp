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

#include "openeditorswindow.h"

#include "editormanager.h"
#include "editormanager_p.h"
#include "editorview.h"
#include <coreplugin/idocument.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QFocusEvent>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QScrollBar>

Q_DECLARE_METATYPE(Core::Internal::EditorView*)
Q_DECLARE_METATYPE(Core::IDocument*)

using namespace Core;
using namespace Core::Internal;

enum class Role
{
    Entry = Qt::UserRole,
    View = Qt::UserRole + 1
};

OpenEditorsWindow::OpenEditorsWindow(QWidget *parent) :
    QFrame(parent, Qt::Popup),
    m_emptyIcon(Utils::Icons::EMPTY14.icon()),
    m_editorList(new OpenEditorsTreeWidget(this))
{
    setMinimumSize(300, 200);
    m_editorList->setColumnCount(1);
    m_editorList->header()->hide();
    m_editorList->setIndentation(0);
    m_editorList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_editorList->setTextElideMode(Qt::ElideMiddle);
    if (Utils::HostOsInfo::isMacHost())
        m_editorList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_editorList->installEventFilter(this);

    // We disable the frame on this list view and use a QFrame around it instead.
    // This improves the look with QGTKStyle.
    if (!Utils::HostOsInfo::isMacHost())
        setFrameStyle(m_editorList->frameStyle());
    m_editorList->setFrameStyle(QFrame::NoFrame);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_editorList);

    connect(m_editorList, &QTreeWidget::itemClicked,
            this, &OpenEditorsWindow::editorClicked);
}

void OpenEditorsWindow::selectAndHide()
{
    setVisible(false);
    selectEditor(m_editorList->currentItem());
}

void OpenEditorsWindow::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible)
        setFocus();
}

bool OpenEditorsWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_editorList) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Escape) {
                setVisible(false);
                return true;
            }
            if (ke->key() == Qt::Key_Return
                    || ke->key() == Qt::Key_Enter) {
                selectEditor(m_editorList->currentItem());
                return true;
            }
        } else if (e->type() == QEvent::KeyRelease) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->modifiers() == 0
                    /*HACK this is to overcome some event inconsistencies between platforms*/
                    || (ke->modifiers() == Qt::AltModifier
                    && (ke->key() == Qt::Key_Alt || ke->key() == -1))) {
                selectAndHide();
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
    QTreeWidgetItem *editor = 0;
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
        editor = m_editorList->topLevelItem(index);
        count++;
    }
    if (editor) {
        m_editorList->setCurrentItem(editor);
        ensureCurrentVisible();
    }
}

void OpenEditorsWindow::selectPreviousEditor()
{
    selectUpDown(false);
}

QSize OpenEditorsTreeWidget::sizeHint() const
{
    return QSize(sizeHintForColumn(0) + verticalScrollBar()->width() + frameWidth() * 2,
                 viewportSizeHint().height() + frameWidth() * 2);
}

QSize OpenEditorsWindow::sizeHint() const
{
    return m_editorList->sizeHint() + QSize(frameWidth() * 2, frameWidth() * 2);
}

void OpenEditorsWindow::selectNextEditor()
{
    selectUpDown(true);
}

void OpenEditorsWindow::setEditors(const QList<EditLocation> &globalHistory, EditorView *view)
{
    m_editorList->clear();

    QSet<const DocumentModel::Entry *> entriesDone;
    addHistoryItems(view->editorHistory(), view, entriesDone);

    // add missing editors from the global history
    addHistoryItems(globalHistory, view, entriesDone);

    // add purely suspended editors which are not initialised yet
    addRemainingItems(view, entriesDone);
}


void OpenEditorsWindow::selectEditor(QTreeWidgetItem *item)
{
    if (!item)
        return;
    auto entry = item->data(0, int(Role::Entry)).value<DocumentModel::Entry *>();
    QTC_ASSERT(entry, return);
    auto view = item->data(0, int(Role::View)).value<EditorView *>();
    if (!EditorManagerPrivate::activateEditorForEntry(view, entry))
        delete item;
}

void OpenEditorsWindow::editorClicked(QTreeWidgetItem *item)
{
    selectEditor(item);
    setFocus();
}


void OpenEditorsWindow::ensureCurrentVisible()
{
    m_editorList->scrollTo(m_editorList->currentIndex(), QAbstractItemView::PositionAtCenter);
}

static DocumentModel::Entry *entryForEditLocation(const EditLocation &item)
{
    if (!item.document.isNull())
        return DocumentModel::entryForDocument(item.document);
    return DocumentModel::entryForFilePath(Utils::FileName::fromString(item.fileName));
}

void OpenEditorsWindow::addHistoryItems(const QList<EditLocation> &history, EditorView *view,
                                        QSet<const DocumentModel::Entry *> &entriesDone)
{
    foreach (const EditLocation &hi, history) {
        if (DocumentModel::Entry *entry = entryForEditLocation(hi))
            addItem(entry, entriesDone, view);
    }
}

void OpenEditorsWindow::addRemainingItems(EditorView *view,
                                          QSet<const DocumentModel::Entry *> &entriesDone)
{
    foreach (DocumentModel::Entry *entry, DocumentModel::entries())
        addItem(entry, entriesDone, view);
}

void OpenEditorsWindow::addItem(DocumentModel::Entry *entry,
                                QSet<const DocumentModel::Entry *> &entriesDone,
                                EditorView *view)
{
    if (entriesDone.contains(entry))
        return;
    entriesDone.insert(entry);
    QString title = entry->displayName();
    QTC_ASSERT(!title.isEmpty(), return);
    QTreeWidgetItem *item = new QTreeWidgetItem();
    if (entry->document->isModified())
        title += tr("*");
    item->setIcon(0, !entry->fileName().isEmpty() && entry->document->isFileReadOnly()
                  ? DocumentModel::lockedIcon() : m_emptyIcon);
    item->setText(0, title);
    item->setToolTip(0, entry->fileName().toString());
    item->setData(0, int(Role::Entry), QVariant::fromValue(entry));
    item->setData(0, int(Role::View), QVariant::fromValue(view));
    item->setTextAlignment(0, Qt::AlignLeft);

    m_editorList->addTopLevelItem(item);

    if (m_editorList->topLevelItemCount() == 1)
        m_editorList->setCurrentItem(item);
}
