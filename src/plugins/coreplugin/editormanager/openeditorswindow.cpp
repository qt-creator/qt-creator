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

#include "openeditorswindow.h"

#include "documentmodel.h"
#include "editormanager.h"
#include "editormanager_p.h"
#include "editorview.h"
#include <coreplugin/idocument.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QFocusEvent>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QScrollBar>

Q_DECLARE_METATYPE(Core::Internal::EditorView*)
Q_DECLARE_METATYPE(Core::IDocument*)

using namespace Core;
using namespace Core::Internal;

OpenEditorsWindow::OpenEditorsWindow(QWidget *parent) :
    QFrame(parent, Qt::Popup),
    m_emptyIcon(QLatin1String(":/core/images/empty14.png")),
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

    connect(m_editorList, SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this, SLOT(editorClicked(QTreeWidgetItem*)));
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

    QSet<IDocument*> documentsDone;
    addHistoryItems(view->editorHistory(), view, documentsDone);

    // add missing editors from the global history
    addHistoryItems(globalHistory, view, documentsDone);

    // add purely restored editors which are not initialised yet
    addRestoredItems();
}


void OpenEditorsWindow::selectEditor(QTreeWidgetItem *item)
{
    if (!item)
        return;
    if (IDocument *document = item->data(0, Qt::UserRole).value<IDocument*>()) {
        EditorView *view = item->data(0, Qt::UserRole+1).value<EditorView*>();
        EditorManagerPrivate::activateEditorForDocument(view, document);
    } else {
        if (!EditorManager::openEditor(
                    item->toolTip(0), item->data(0, Qt::UserRole+2).value<Id>())) {
            DocumentModel::removeDocument(item->toolTip(0));
            delete item;
        }
    }
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


void OpenEditorsWindow::addHistoryItems(const QList<EditLocation> &history, EditorView *view,
                                        QSet<IDocument *> &documentsDone)
{
    foreach (const EditLocation &hi, history) {
        if (hi.document.isNull() || documentsDone.contains(hi.document))
            continue;
        documentsDone.insert(hi.document.data());
        DocumentModel::Entry *entry = DocumentModel::entryForDocument(hi.document);
        QString title = entry ? entry->displayName() : hi.document->displayName();
        QTC_ASSERT(!title.isEmpty(), continue);
        QTreeWidgetItem *item = new QTreeWidgetItem();
        if (hi.document->isModified())
            title += tr("*");
        item->setIcon(0, !hi.document->filePath().isEmpty() && hi.document->isFileReadOnly()
                      ? DocumentModel::lockedIcon() : m_emptyIcon);
        item->setText(0, title);
        item->setToolTip(0, hi.document->filePath().toString());
        item->setData(0, Qt::UserRole, QVariant::fromValue(hi.document.data()));
        item->setData(0, Qt::UserRole+1, QVariant::fromValue(view));
        item->setTextAlignment(0, Qt::AlignLeft);

        m_editorList->addTopLevelItem(item);

        if (m_editorList->topLevelItemCount() == 1)
            m_editorList->setCurrentItem(item);
    }
}

void OpenEditorsWindow::addRestoredItems()
{
    foreach (DocumentModel::Entry *entry, DocumentModel::entries()) {
        if (!entry->isRestored)
            continue;
        QTreeWidgetItem *item = new QTreeWidgetItem();
        QString title = entry->displayName();
        item->setIcon(0, m_emptyIcon);
        item->setText(0, title);
        item->setToolTip(0, entry->fileName().toString());
        item->setData(0, Qt::UserRole+2, QVariant::fromValue(entry->id()));
        item->setTextAlignment(0, Qt::AlignLeft);

        m_editorList->addTopLevelItem(item);
    }
}
