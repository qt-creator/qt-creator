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

#include "completionwidget.h"
#include "completionsupport.h"
#include "icompletioncollector.h"

#include <texteditor/itexteditable.h>
#include <utils/qtcassert.h>

#include <QtCore/QEvent>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QVBoxLayout>

#include <limits.h>

using namespace TextEditor;
using namespace TextEditor::Internal;

#define NUMBER_OF_VISIBLE_ITEMS 10

class AutoCompletionModel : public QAbstractListModel
{
public:
    AutoCompletionModel(QObject *parent, const QList<CompletionItem> &items);

    inline const CompletionItem &itemAt(const QModelIndex &index) const
    { return m_items.at(index.row()); }

    void setItems(const QList<CompletionItem> &items);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    QList<CompletionItem> m_items;
};

AutoCompletionModel::AutoCompletionModel(QObject *parent, const QList<CompletionItem> &items)
    : QAbstractListModel(parent)
{
    m_items = items;
}

void AutoCompletionModel::setItems(const QList<CompletionItem> &items)
{
    m_items = items;
    reset();
}

int AutoCompletionModel::rowCount(const QModelIndex &) const
{
    return m_items.count();
}

QVariant AutoCompletionModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= m_items.count())
        return QVariant();

    if (role == Qt::DisplayRole) {
        return itemAt(index).m_text;
    } else if (role == Qt::DecorationRole) {
        return itemAt(index).m_icon;
    } else if (role == Qt::ToolTipRole) {
        return itemAt(index).m_details;
    }

    return QVariant();
}

CompletionWidget::CompletionWidget(CompletionSupport *support, ITextEditable *editor)
    : QListView(),
      m_blockFocusOut(false),
      m_editor(editor),
      m_editorWidget(editor->widget()),
      m_model(0),
      m_support(support)
{
    QTC_ASSERT(m_editorWidget, return);

    setUniformItemSizes(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);

    connect(this, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(completionActivated(const QModelIndex &)));

    // We disable the frame on this list view and use a QFrame around it instead.
    // This fixes the missing frame on Mac and improves the look with QGTKStyle.
    m_popupFrame = new QFrame(0, Qt::Popup);
    m_popupFrame->setFrameStyle(frameStyle());
    setFrameStyle(QFrame::NoFrame);
    setParent(m_popupFrame);
    m_popupFrame->setObjectName("m_popupFrame");
    m_popupFrame->setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout *layout = new QVBoxLayout(m_popupFrame);
    layout->setMargin(0);
    layout->addWidget(this);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_popupFrame->setMinimumSize(1, 1);
    setMinimumSize(1, 1);
}

bool CompletionWidget::event(QEvent *e)
{
    if (m_blockFocusOut)
        return QListView::event(e);

    bool forwardKeys = true;
    if (e->type() == QEvent::FocusOut) {
        closeList();
        return true;
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_Escape:
            closeList();
            return true;
        case Qt::Key_Right:
        case Qt::Key_Left:
            break;
        case Qt::Key_Tab:
        case Qt::Key_Return:
            //independently from style, accept current entry if return is pressed
            closeList(currentIndex());
            return true;
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Enter:
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            forwardKeys = false;
            break;
        default:
            break;
        }

        if (forwardKeys) {
            m_blockFocusOut = true;
            QApplication::sendEvent(m_editorWidget, e);
            m_blockFocusOut = false;

            // Have the completion support update the list of items
            m_support->autoComplete(m_editor, false);

            return true;
        }
    }
    return QListView::event(e);
}

void CompletionWidget::keyboardSearch(const QString &search)
{
    Q_UNUSED(search);
}

void CompletionWidget::closeList(const QModelIndex &index)
{
    m_blockFocusOut = true;
    if (index.isValid())
        emit itemSelected(m_model->itemAt(index));

    close();
    if (m_popupFrame) {
        m_popupFrame->close();
        m_popupFrame = 0;
    }

    emit completionListClosed();

    m_blockFocusOut = false;
}

void CompletionWidget::setCompletionItems(const QList<TextEditor::CompletionItem> &completionItems)
{
    if (!m_model) {
        m_model = new AutoCompletionModel(this, completionItems);
        setModel(m_model);
    } else {
        m_model->setItems(completionItems);
    }

    // Select the first of the most relevant completion items
    int relevance = INT_MIN;
    int mostRelevantIndex = 0;
    for (int i = 0; i < completionItems.size(); ++i) {
        const CompletionItem &item = completionItems.at(i);
        if (item.m_relevance > relevance) {
            relevance = item.m_relevance;
            mostRelevantIndex = i;
        }
    }

    setCurrentIndex(m_model->index(mostRelevantIndex));
}

void CompletionWidget::showCompletions(int startPos)
{
    updatePositionAndSize(startPos);
    m_popupFrame->show();
    show();
    setFocus();
}

void CompletionWidget::updatePositionAndSize(int startPos)
{
    // Determine size by calculating the space of the visible items
    int visibleItems = m_model->rowCount();
    if (visibleItems > NUMBER_OF_VISIBLE_ITEMS)
        visibleItems = NUMBER_OF_VISIBLE_ITEMS;

    const QStyleOptionViewItem &option = viewOptions();

    QSize shint;
    for (int i = 0; i < visibleItems; ++i) {
        QSize tmp = itemDelegate()->sizeHint(option, m_model->index(i));
        if (shint.width() < tmp.width())
            shint = tmp;
    }

    const int frameWidth = m_popupFrame->frameWidth();
    const int width = shint.width() + frameWidth * 2 + 30;
    const int height = shint.height() * visibleItems + frameWidth * 2;

    // Determine the position, keeping the popup on the screen
    const QRect cursorRect = m_editor->cursorRect(startPos);
    const QDesktopWidget *desktop = QApplication::desktop();
#ifdef Q_OS_MAC
    const QRect screen = desktop->availableGeometry(desktop->screenNumber(m_editorWidget));
#else
    const QRect screen = desktop->screenGeometry(desktop->screenNumber(m_editorWidget));
#endif

    QPoint pos = cursorRect.bottomLeft();
    pos.rx() -= 16 + frameWidth;    // Space for the icons

    if (pos.y() + height > screen.bottom())
        pos.setY(cursorRect.top() - height);

    if (pos.x() + width > screen.right())
        pos.setX(screen.right() - width);

    m_popupFrame->setGeometry(pos.x(), pos.y(), width, height);
}

void CompletionWidget::completionActivated(const QModelIndex &index)
{
    closeList(index);
}
