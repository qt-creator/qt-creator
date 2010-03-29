/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
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
#include <QtGui/QScrollBar>

#include <limits.h>

using namespace TextEditor;
using namespace TextEditor::Internal;

#define NUMBER_OF_VISIBLE_ITEMS 10

namespace TextEditor {
namespace Internal {

class AutoCompletionModel : public QAbstractListModel
{
public:
    AutoCompletionModel(QObject *parent);

    inline const CompletionItem &itemAt(const QModelIndex &index) const
    { return m_items.at(index.row()); }

    void setItems(const QList<CompletionItem> &items);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    QList<CompletionItem> m_items;
};

} // namespace Internal
} // namespace TextEditor


AutoCompletionModel::AutoCompletionModel(QObject *parent)
    : QAbstractListModel(parent)
{
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
        return itemAt(index).text;
    } else if (role == Qt::DecorationRole) {
        return itemAt(index).icon;
    } else if (role == Qt::ToolTipRole) {
        return itemAt(index).details;
    }

    return QVariant();
}


CompletionWidget::CompletionWidget(CompletionSupport *support, ITextEditable *editor)
    : QFrame(0, Qt::Popup),
      m_support(support),
      m_editor(editor),
      m_completionListView(new CompletionListView(support, editor, this))
{
    // We disable the frame on this list view and use a QFrame around it instead.
    // This improves the look with QGTKStyle.
#ifndef Q_WS_MAC
    setFrameStyle(m_completionListView->frameStyle());
#endif
    m_completionListView->setFrameStyle(QFrame::NoFrame);

    setObjectName(QLatin1String("m_popupFrame"));
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(1, 1);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    layout->addWidget(m_completionListView);
    setFocusProxy(m_completionListView);

    connect(m_completionListView, SIGNAL(itemSelected(TextEditor::CompletionItem)),
            this, SIGNAL(itemSelected(TextEditor::CompletionItem)));
    connect(m_completionListView, SIGNAL(completionListClosed()),
            this, SIGNAL(completionListClosed()));
    connect(m_completionListView, SIGNAL(activated(QModelIndex)),
            SLOT(closeList(QModelIndex)));
}

CompletionWidget::~CompletionWidget()
{
}

void CompletionWidget::setQuickFix(bool quickFix)
{
    m_completionListView->setQuickFix(quickFix);
}

void CompletionWidget::setCompletionItems(const QList<TextEditor::CompletionItem> &completionitems)
{
    m_completionListView->setCompletionItems(completionitems);
}

void CompletionWidget::closeList(const QModelIndex &index)
{
    m_completionListView->closeList(index);
    close();
}

void CompletionWidget::showCompletions(int startPos)
{
    ensurePolished();
    updatePositionAndSize(startPos);
    show();
    setFocus();
}

void CompletionWidget::updatePositionAndSize(int startPos)
{
    // Determine size by calculating the space of the visible items
    QAbstractItemModel *model = m_completionListView->model();
    int visibleItems = model->rowCount();
    if (visibleItems > NUMBER_OF_VISIBLE_ITEMS)
        visibleItems = NUMBER_OF_VISIBLE_ITEMS;

    const QStyleOptionViewItem &option = m_completionListView->viewOptions();

    QSize shint;
    for (int i = 0; i < visibleItems; ++i) {
        QSize tmp = m_completionListView->itemDelegate()->sizeHint(option, model->index(i, 0));
        if (shint.width() < tmp.width())
            shint = tmp;
    }

    const int fw = frameWidth();
    const int width = shint.width() + fw * 2 + 30;
    const int height = shint.height() * visibleItems + fw * 2;

    // Determine the position, keeping the popup on the screen
    const QRect cursorRect = m_editor->cursorRect(startPos);
    const QDesktopWidget *desktop = QApplication::desktop();

    QWidget *editorWidget = m_editor->widget();

#ifdef Q_WS_MAC
    const QRect screen = desktop->availableGeometry(desktop->screenNumber(editorWidget));
#else
    const QRect screen = desktop->screenGeometry(desktop->screenNumber(editorWidget));
#endif

    QPoint pos = cursorRect.bottomLeft();
    pos.rx() -= 16 + fw;    // Space for the icons

    if (pos.y() + height > screen.bottom())
        pos.setY(cursorRect.top() - height);

    if (pos.x() + width > screen.right())
        pos.setX(screen.right() - width);

    setGeometry(pos.x(), pos.y(), width, height);
}


CompletionListView::CompletionListView(CompletionSupport *support, ITextEditable *editor, CompletionWidget *completionWidget)
    : QListView(completionWidget),
      m_blockFocusOut(false),
      m_quickFix(false),
      m_editor(editor),
      m_editorWidget(editor->widget()),
      m_completionWidget(completionWidget),
      m_model(new AutoCompletionModel(this)),
      m_support(support)
{
    QTC_ASSERT(m_editorWidget, return);

    setAttribute(Qt::WA_MacShowFocusRect, false);
    setUniformItemSizes(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMinimumSize(1, 1);
    setModel(m_model);
#ifdef Q_WS_MAC
    if (horizontalScrollBar())
        horizontalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
    if (verticalScrollBar())
        verticalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
#endif
}

CompletionListView::~CompletionListView()
{
}

bool CompletionListView::event(QEvent *e)
{
    if (m_blockFocusOut)
        return QListView::event(e);

    bool forwardKeys = true;
    if (e->type() == QEvent::FocusOut) {
        QModelIndex index;
#if defined(Q_OS_DARWIN) && ! defined(QT_MAC_USE_COCOA)
        QFocusEvent *fe = static_cast<QFocusEvent *>(e);
        if (fe->reason() == Qt::OtherFocusReason) {
            // Qt/carbon workaround
            // focus out is received before the key press event.
            index = currentIndex();
        }
#endif
        m_completionWidget->closeList(index);
        return true;
    } else if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_N:
        case Qt::Key_P:
            // select next/previous completion
            if (ke->modifiers() == Qt::ControlModifier)
            {
                e->accept();
                int change = (ke->key() == Qt::Key_N) ? 1 : -1;
                int nrows = model()->rowCount();
                int row = currentIndex().row();
                int newRow = (row + change + nrows) % nrows;
                if (newRow == row + change || !ke->isAutoRepeat())
                    setCurrentIndex(m_model->index(newRow));
                return true;
            }
        }
    } else if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key()) {
        case Qt::Key_N:
        case Qt::Key_P:
            // select next/previous completion - so don't pass on to editor
            if (ke->modifiers() == Qt::ControlModifier)
                forwardKeys = false;
            break;

        case Qt::Key_Escape:
            m_completionWidget->closeList();
            return true;

        case Qt::Key_Right:
        case Qt::Key_Left:
            break;

        case Qt::Key_Tab:
        case Qt::Key_Return:
            //independently from style, accept current entry if return is pressed
            if (qApp->focusWidget() == this)
                m_completionWidget->closeList(currentIndex());
            return true;

        case Qt::Key_Up:
            if (!ke->isAutoRepeat()
                && currentIndex().row() == 0) {
                setCurrentIndex(model()->index(model()->rowCount()-1, 0));
                return true;
            }
            forwardKeys = false;
            break;

        case Qt::Key_Down:
            if (!ke->isAutoRepeat()
                && currentIndex().row() == model()->rowCount()-1) {
                setCurrentIndex(model()->index(0, 0));
                return true;
            }
            forwardKeys = false;
            break;

        case Qt::Key_Enter:
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            forwardKeys = false;
            break;

        default:
            // if a key is forwarded, completion widget is re-opened and selected item is reset to first,
            // so only forward keys that insert text and refine the completed item
            forwardKeys = !ke->text().isEmpty();
            break;
        }

        if (forwardKeys && ! m_quickFix) {
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

void CompletionListView::keyboardSearch(const QString &search)
{
    Q_UNUSED(search)
}

void CompletionListView::setQuickFix(bool quickFix)
{
    m_quickFix = quickFix;
}

void CompletionListView::setCompletionItems(const QList<TextEditor::CompletionItem> &completionItems)
{
    m_model->setItems(completionItems);

    // Select the first of the most relevant completion items
    int relevance = INT_MIN;
    int mostRelevantIndex = 0;
    for (int i = 0; i < completionItems.size(); ++i) {
        const CompletionItem &item = completionItems.at(i);
        if (item.relevance > relevance) {
            relevance = item.relevance;
            mostRelevantIndex = i;
        }
    }

    setCurrentIndex(m_model->index(mostRelevantIndex));
}

void CompletionListView::closeList(const QModelIndex &index)
{
    m_blockFocusOut = true;

    if (index.isValid())
        emit itemSelected(m_model->itemAt(index));

    emit completionListClosed();

    m_blockFocusOut = false;
}
