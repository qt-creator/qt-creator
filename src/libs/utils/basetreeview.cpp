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

#include "basetreeview.h"

#include "progressindicator.h"
#include "treemodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFontMetrics>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QMouseEvent>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QTimer>

namespace Utils {
namespace Internal {

const char ColumnKey[] = "Columns";

class BaseTreeViewPrivate : public QObject
{
public:
    explicit BaseTreeViewPrivate(BaseTreeView *parent)
        : q(parent)
    {
        m_settingsTimer.setSingleShot(true);
        connect(&m_settingsTimer, &QTimer::timeout,
                this, &BaseTreeViewPrivate::doSaveState);
        connect(q->header(), &QHeaderView::sectionResized, this, [this](int logicalIndex, int oldSize, int newSize) {
            if (m_processingSpans || m_spanColumn < 0)
                return;

            QHeaderView *h = q->header();
            QTC_ASSERT(h, return);

            // Last non-hidden column.
            int count = h->count();
            while (count > 0 && h->isSectionHidden(count - 1))
                --count;

            if (count == 0)
                return;

            int column = logicalIndex;
            if (oldSize < newSize)
            {
                // Protect against sizing past the next section.
                while (column + 1 < count && h->sectionSize(column + 1) == h->minimumSectionSize())
                    ++column;
            }

            if (logicalIndex >= m_spanColumn) {
                // Resize after the span column.
                column = column + 1;
            } else {
                // Resize the span column or before it.
                column = m_spanColumn;
            }

            rebalanceColumns(column, false);
        });
        connect(q->header(), &QHeaderView::geometriesChanged, this, QOverload<>::of(&BaseTreeViewPrivate::rebalanceColumns));
    }

    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::MouseMove) {
            // At this time we don't know which section will get which size.
            // But we know that a resizedSection() will be emitted later.
            const auto *me = static_cast<QMouseEvent *>(event);
            if (me->buttons() & Qt::LeftButton)
                m_expectUserChanges = true;
        }
        return false;
    }

    void readSettings()
    {
        // This only reads setting, does not restore state.
        // Storage format is a flat list of column numbers and width.
        // Columns not mentioned are resized to content////
        m_userHandled.clear();
        if (m_settings && !m_settingsKey.isEmpty()) {
            m_settings->beginGroup(m_settingsKey);
            QVariantList l = m_settings->value(QLatin1String(ColumnKey)).toList();
            QTC_ASSERT(l.size() % 2 == 0, qDebug() << m_settingsKey; l.append(0));
            for (int i = 0; i < l.size(); i += 2) {
                int column = l.at(i).toInt();
                int width = l.at(i + 1).toInt();
                QTC_ASSERT(column >= 0 && column < 20, qDebug() << m_settingsKey << column; continue);
                QTC_ASSERT(width > 0 && width < 10000, qDebug() << m_settingsKey << width; continue);
                m_userHandled[column] = width;
            }
            m_settings->endGroup();
        }
    }

    void restoreState()
    {
        if (m_settings && !m_settingsKey.isEmpty()) {
            QHeaderView *h = q->header();
            for (auto it = m_userHandled.constBegin(), et = m_userHandled.constEnd(); it != et; ++it) {
                const int column = it.key();
                const int targetSize = it.value();
                const int currentSize = h->sectionSize(column);
                if (targetSize > 0 && targetSize != currentSize)
                    h->resizeSection(column, targetSize);
            }
        }
    }

    void saveState()
    {
        m_settingsTimer.start(2000); // Once per 2 secs should be enough.
    }

    void doSaveState()
    {
        m_settingsTimer.stop();
        if (m_settings && !m_settingsKey.isEmpty()) {
            m_settings->beginGroup(m_settingsKey);
            QVariantList l;
            for (auto it = m_userHandled.constBegin(), et = m_userHandled.constEnd(); it != et; ++it) {
                const int column = it.key();
                const int width = it.value();
                QTC_ASSERT(column >= 0 && column < q->model()->columnCount(), continue);
                QTC_ASSERT(width > 0 && width < 10000, continue);
                l.append(column);
                l.append(width);
            }
            m_settings->setValue(QLatin1String(ColumnKey), l);
            m_settings->endGroup();
        }
    }

    void handleSectionResized(int logicalIndex, int /*oldSize*/, int newSize)
    {
        if (m_expectUserChanges) {
            m_userHandled[logicalIndex] = newSize;
            saveState();
            m_expectUserChanges = false;
        }
    }


    void considerItems(int column, QModelIndex start, int *minimum, bool single) const
    {
        QModelIndex a = start;
        a = a.sibling(a.row(), column);
        QFontMetrics fm = q->fontMetrics();
        const int ind = q->indentation();
        QAbstractItemModel *m = q->model();
        for (int i = 0; i < 100 && a.isValid(); ++i) {
            const QString s = m->data(a).toString();
            int w = fm.horizontalAdvance(s) + 10;
            if (column == 0) {
                for (QModelIndex b = a.parent(); b.isValid(); b = b.parent())
                    w += ind;
            }
            if (w > *minimum)
                *minimum = w;
            if (single)
                break;
            a = q->indexBelow(a);
        }
    }

    int suggestedColumnSize(int column) const
    {
        QHeaderView *h = q->header();
        QTC_ASSERT(h, return -1);
        QAbstractItemModel *m = q->model();
        QTC_ASSERT(m, return -1);

        QFontMetrics fm = q->fontMetrics();
        int minimum = fm.horizontalAdvance(m->headerData(column, Qt::Horizontal).toString())
            + 2 * fm.horizontalAdvance(QLatin1Char('m'));
        considerItems(column, q->indexAt(QPoint(1, 1)), &minimum, false);

        const QVariant extraIndices = m->data(QModelIndex(), BaseTreeView::ExtraIndicesForColumnWidth);
        const QList<QModelIndex> values = extraIndices.value<QModelIndexList>();
        for (const QModelIndex &a : values)
            considerItems(column, a, &minimum, true);

        return minimum;
    }

    void resizeColumns()
    {
        QHeaderView *h = q->header();
        QTC_ASSERT(h, return);

        if (m_settings && !m_settingsKey.isEmpty()) {
            for (int i = 0, n = h->count(); i != n; ++i) {
                int targetSize;
                if (m_userHandled.contains(i))
                    targetSize = m_userHandled.value(i);
                else
                    targetSize = suggestedColumnSize(i);
                const int currentSize = h->sectionSize(i);
                if (targetSize > 0 && targetSize != currentSize)
                    h->resizeSection(i, targetSize);
            }
        }
    }

    void setSpanColumn(int column)
    {
        if (column == m_spanColumn)
            return;

        m_spanColumn = column;
        if (m_spanColumn >= 0)
            q->header()->setStretchLastSection(false);
        rebalanceColumns();
    }

    void toggleColumnWidth(int logicalIndex)
    {
        QHeaderView *h = q->header();
        const int currentSize = h->sectionSize(logicalIndex);
        const int suggestedSize = suggestedColumnSize(logicalIndex);
        int targetSize = suggestedSize;
        // We switch to the size suggested by the contents, except
        // when we have that size already, in that case minimize.
        if (currentSize == suggestedSize) {
            QFontMetrics fm = q->fontMetrics();
            int headerSize = fm.horizontalAdvance(q->model()->headerData(logicalIndex, Qt::Horizontal).toString());
            int minSize = 10 * fm.horizontalAdvance(QLatin1Char('x'));
            targetSize = qMax(minSize, headerSize);
        }

        // Prevent rebalance as part of this resize.
        m_processingSpans = true;
        h->resizeSection(logicalIndex, targetSize);
        m_processingSpans = false;

        // Now trigger a rebalance so it resizes the span column. (if set)
        rebalanceColumns();

        m_userHandled.remove(logicalIndex); // Reset.
        saveState();
    }

    void rebalanceColumns()
    {
        rebalanceColumns(m_spanColumn, true);
    }

    void rebalanceColumns(int column, bool allowResizePrevious)
    {
        if (m_spanColumn < 0 || column < 0 || m_processingSpans)
            return;

        QHeaderView *h = q->header();
        QTC_ASSERT(h, return);

        int count = h->count();
        if (column >= count)
            return;

        // Start with the target column, and resize other columns as necessary.
        int totalSize = q->viewport()->width();
        if (tryRebalanceColumns(column, totalSize))
            return;

        for (int i = allowResizePrevious ? 0 : column + 1; i < count; ++i) {
            if (i != column && tryRebalanceColumns(i, totalSize))
                return;
        }
    }

    bool tryRebalanceColumns(int column, int totalSize)
    {
        QHeaderView *h = q->header();

        int count = h->count();
        int otherColumnTotal = 0;
        for (int i = 0; i < count; ++i) {
            if (i != column)
                otherColumnTotal += h->sectionSize(i);
        }

        if (otherColumnTotal < totalSize) {
            m_processingSpans = true;
            h->resizeSection(column, totalSize - otherColumnTotal);
            m_processingSpans = false;
        } else
            return false;

        // Make sure this didn't go over the total size.
        int totalColumnSize = 0;
        for (int i = 0; i < count; ++i)
            totalColumnSize += h->sectionSize(i);
        return totalColumnSize == totalSize;
    }

public:
    BaseTreeView *q;
    QMap<int, int> m_userHandled; // column -> width, "not present" means "automatic"
    QSettings *m_settings = nullptr;
    QTimer m_settingsTimer;
    QString m_settingsKey;
    bool m_expectUserChanges = false;
    ProgressIndicator *m_progressIndicator = nullptr;
    int m_spanColumn = -1;
    bool m_processingSpans = false;
};

class BaseTreeViewDelegate : public QItemDelegate
{
public:
    BaseTreeViewDelegate(QObject *parent): QItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        Q_UNUSED(option)
        QLabel *label = new QLabel(parent);
        label->setAutoFillBackground(true);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse
            | Qt::LinksAccessibleByMouse);
        label->setText(index.data().toString());
        return label;
    }
};

} // namespace Internal

using namespace Internal;

BaseTreeView::BaseTreeView(QWidget *parent)
    : TreeView(parent), d(new BaseTreeViewPrivate(this))
{
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setFrameStyle(QFrame::NoFrame);
    setRootIsDecorated(false);
    setIconSize(QSize(16, 16));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setUniformRowHeights(true);
    setItemDelegate(new BaseTreeViewDelegate(this));
    setAlternatingRowColors(false);

    QHeaderView *h = header();
    h->setDefaultAlignment(Qt::AlignLeft);
    h->setSectionsClickable(true);
    h->viewport()->installEventFilter(d);

    connect(this, &QAbstractItemView::activated,
            this, &BaseTreeView::rowActivated);
    connect(this, &QAbstractItemView::clicked,
            this, &BaseTreeView::rowClicked);

    connect(h, &QHeaderView::sectionClicked,
            d, &BaseTreeViewPrivate::toggleColumnWidth);
    connect(h, &QHeaderView::sectionResized,
            d, &BaseTreeViewPrivate::handleSectionResized);
}

BaseTreeView::~BaseTreeView()
{
    delete d;
}

void BaseTreeView::setModel(QAbstractItemModel *m)
{
    if (auto oldModel = qobject_cast<BaseTreeModel *>(model())) {
        disconnect(oldModel, &BaseTreeModel::requestExpansion, this, &BaseTreeView::expand);
        disconnect(oldModel, &BaseTreeModel::requestCollapse, this, &BaseTreeView::collapse);
    }

    TreeView::setModel(m);

    if (m) {
        if (auto newModel = qobject_cast<BaseTreeModel *>(m)) {
            connect(newModel, &BaseTreeModel::requestExpansion, this, &BaseTreeView::expand);
            connect(newModel, &BaseTreeModel::requestCollapse, this, &BaseTreeView::collapse);
        }
        d->restoreState();

        QVariant delegateBlob = m->data(QModelIndex(), ItemDelegateRole);
        if (delegateBlob.isValid()) {
            auto delegate = delegateBlob.value<QAbstractItemDelegate *>();
            delegate->setParent(this);
            setItemDelegate(delegate);
        }
    }
}

void BaseTreeView::mousePressEvent(QMouseEvent *ev)
{
    ItemViewEvent ive(ev, this);
    QTC_ASSERT(model(), return);
    if (!model()->setData(ive.index(), QVariant::fromValue(ive), ItemViewEventRole))
        TreeView::mousePressEvent(ev);
// Resizing columns by clicking on the empty space seems to be controversial.
// Let's try without for a while.
//    const QModelIndex mi = indexAt(ev->pos());
//    if (!mi.isValid())
//        d->toggleColumnWidth(columnAt(ev->x()));
}

void BaseTreeView::mouseMoveEvent(QMouseEvent *ev)
{
    ItemViewEvent ive(ev, this);
    QTC_ASSERT(model(), return);
    if (!model()->setData(ive.index(), QVariant::fromValue(ive), ItemViewEventRole))
        TreeView::mouseMoveEvent(ev);
}

void BaseTreeView::mouseReleaseEvent(QMouseEvent *ev)
{
    ItemViewEvent ive(ev, this);
    QTC_ASSERT(model(), return);
    if (!model()->setData(ive.index(), QVariant::fromValue(ive), ItemViewEventRole))
        TreeView::mouseReleaseEvent(ev);
}

void BaseTreeView::contextMenuEvent(QContextMenuEvent *ev)
{
    ItemViewEvent ive(ev, this);
    if (!model()->setData(ive.index(), QVariant::fromValue(ive), ItemViewEventRole))
        TreeView::contextMenuEvent(ev);
}

void BaseTreeView::keyPressEvent(QKeyEvent *ev)
{
    ItemViewEvent ive(ev, this);
    if (!model()->setData(ive.index(), QVariant::fromValue(ive), ItemViewEventRole))
        TreeView::keyPressEvent(ev);
}

void BaseTreeView::dragEnterEvent(QDragEnterEvent *ev)
{
    ItemViewEvent ive(ev, this);
    if (!model()->setData(ive.index(), QVariant::fromValue(ive), ItemViewEventRole))
        TreeView::dragEnterEvent(ev);
}

void BaseTreeView::dropEvent(QDropEvent *ev)
{
    ItemViewEvent ive(ev, this);
    if (!model()->setData(ive.index(), QVariant::fromValue(ive), ItemViewEventRole))
        TreeView::dropEvent(ev);
}

void BaseTreeView::dragMoveEvent(QDragMoveEvent *ev)
{
    ItemViewEvent ive(ev, this);
    if (!model()->setData(ive.index(), QVariant::fromValue(ive), ItemViewEventRole))
        TreeView::dragMoveEvent(ev);
}

void BaseTreeView::mouseDoubleClickEvent(QMouseEvent *ev)
{
    ItemViewEvent ive(ev, this);
    if (!model()->setData(ive.index(), QVariant::fromValue(ive), ItemViewEventRole))
        TreeView::mouseDoubleClickEvent(ev);
}

void BaseTreeView::resizeEvent(QResizeEvent *ev)
{
    TreeView::resizeEvent(ev);
    d->rebalanceColumns();
}

void BaseTreeView::showEvent(QShowEvent *ev)
{
    emit aboutToShow();
    TreeView::showEvent(ev);
}

/*!
    Shows a round spinning progress indicator on top of the tree view.
    Creates a progress indicator widget if necessary.
    \sa hideProgressIndicator()
 */
void BaseTreeView::showProgressIndicator()
{
    if (!d->m_progressIndicator) {
        d->m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large);
        d->m_progressIndicator->attachToWidget(this);
    }
    d->m_progressIndicator->show();
}

/*!
    Hides the round spinning progress indicator that was shown with
    BaseTreeView::showProgressIndicator(). Note that the progress indicator widget is not
    destroyed.
    \sa showProgressIndicator()
 */
void BaseTreeView::hideProgressIndicator()
{
    if (d->m_progressIndicator)
        d->m_progressIndicator->hide();
}

void BaseTreeView::rowActivated(const QModelIndex &index)
{
    model()->setData(index, QVariant(), ItemActivatedRole);
}

void BaseTreeView::rowClicked(const QModelIndex &index)
{
    model()->setData(index, QVariant(), ItemClickedRole);
}

void BaseTreeView::resizeColumns()
{
    d->resizeColumns();
}

int BaseTreeView::spanColumn() const
{
    return d->m_spanColumn;
}

void BaseTreeView::setSpanColumn(int column)
{
    d->setSpanColumn(column);
}

void BaseTreeView::refreshSpanColumn()
{
    d->rebalanceColumns();
}

void BaseTreeView::setSettings(QSettings *settings, const QByteArray &key)
{
    QTC_ASSERT(!d->m_settings, qDebug() << "DUPLICATED setSettings" << key);
    d->m_settings = settings;
    d->m_settingsKey = QString::fromLatin1(key);
    d->readSettings();
}

ItemViewEvent::ItemViewEvent(QEvent *ev, QAbstractItemView *view)
    : m_event(ev), m_view(view)
{
    QItemSelectionModel *selection = view->selectionModel();
    switch (ev->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
        m_pos = static_cast<QMouseEvent *>(ev)->pos();
        m_index = view->indexAt(m_pos);
        break;
    case QEvent::ContextMenu:
        m_pos = static_cast<QContextMenuEvent *>(ev)->pos();
        m_index = view->indexAt(m_pos);
        break;
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::Drop:
        m_pos = static_cast<QDropEvent *>(ev)->pos();
        m_index = view->indexAt(m_pos);
        break;
    default:
        m_index = selection ? selection->currentIndex() : QModelIndex();
        break;
    }

    if (selection) {
        m_selectedRows = selection->selectedRows();
        if (m_selectedRows.isEmpty()) {
            QModelIndex current = selection->currentIndex();
            if (current.isValid())
                m_selectedRows.append(current);
        }
    }

    auto fixIndex = [view](QModelIndex idx) {
        QAbstractItemModel *model = view->model();
        while (auto proxy = qobject_cast<QSortFilterProxyModel *>(model)) {
            idx = proxy->mapToSource(idx);
            model = proxy->sourceModel();
        }
        return idx;
    };

    m_sourceModelIndex = fixIndex(m_index);
    m_selectedRows = Utils::transform(m_selectedRows, fixIndex);
}

QModelIndexList ItemViewEvent::currentOrSelectedRows() const
{
    return m_selectedRows.isEmpty() ? QModelIndexList() << m_sourceModelIndex : m_selectedRows;
}

} // namespace Utils
