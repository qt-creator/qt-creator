/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "basetreeview.h"

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
#include <QTimer>

namespace Utils {
namespace Internal {

const char ColumnKey[] = "Columns";

class BaseTreeViewPrivate : public QObject
{
    Q_OBJECT

public:
    explicit BaseTreeViewPrivate(BaseTreeView *parent)
        : q(parent), m_settings(0), m_expectUserChanges(false)
    {}

    bool eventFilter(QObject *, QEvent *event)
    {
        if (event->type() == QEvent::MouseMove) {
            // At this time we don't know which section will get which size.
            // But we know that a resizedSection() will be emitted later.
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
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

    Q_SLOT void handleSectionResized(int logicalIndex, int /*oldSize*/, int newSize)
    {
        if (m_expectUserChanges) {
            m_userHandled[logicalIndex] = newSize;
            saveState();
            m_expectUserChanges = false;
        }
    }

    int suggestedColumnSize(int column) const
    {
        QHeaderView *h = q->header();
        QTC_ASSERT(h, return -1);
        QAbstractItemModel *m = q->model();
        QTC_ASSERT(m, return -1);

        QModelIndex a = q->indexAt(QPoint(1, 1));
        a = a.sibling(a.row(), column);
        QFontMetrics fm = q->fontMetrics();
        int minimum = fm.width(m->headerData(column, Qt::Horizontal).toString());
        const int ind = q->indentation();
        for (int i = 0; i < 100 && a.isValid(); ++i) {
            const QString s = m->data(a).toString();
            int w = fm.width(s) + 10;
            if (column == 0) {
                for (QModelIndex b = a.parent(); b.isValid(); b = b.parent())
                    w += ind;
            }
            if (w > minimum)
                minimum = w;
            a = q->indexBelow(a);
        }
        return minimum;
    }

    Q_SLOT void resizeColumns()
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

    Q_SLOT void rowActivatedHelper(const QModelIndex &index)
    {
        q->rowActivated(index);
    }

    Q_SLOT void rowClickedHelper(const QModelIndex &index)
    {
        q->rowClicked(index);
    }

    Q_SLOT void toggleColumnWidth(int logicalIndex)
    {
        QHeaderView *h = q->header();
        const int currentSize = h->sectionSize(logicalIndex);
        const int suggestedSize = suggestedColumnSize(logicalIndex);
        int targetSize = suggestedSize;
        // We switch to the size suggested by the contents, except
        // when we have that size already, in that case minimize.
        if (currentSize == suggestedSize) {
            QFontMetrics fm = q->fontMetrics();
            int headerSize = fm.width(q->model()->headerData(logicalIndex, Qt::Horizontal).toString());
            int minSize = 10 * fm.width(QLatin1Char('x'));
            targetSize = qMax(minSize, headerSize);
        }
        h->resizeSection(logicalIndex, targetSize);
        m_userHandled.remove(logicalIndex); // Reset.
        saveState();
    }

public:
    BaseTreeView *q;
    QMap<int, int> m_userHandled; // column -> width, "not present" means "automatic"
    QSettings *m_settings;
    QString m_settingsKey;
    bool m_expectUserChanges;
};

class BaseTreeViewDelegate : public QItemDelegate
{
public:
    BaseTreeViewDelegate(QObject *parent): QItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
    {
        Q_UNUSED(option);
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
    setIconSize(QSize(10, 10));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setUniformRowHeights(true);
    setItemDelegate(new BaseTreeViewDelegate(this));

    QHeaderView *h = header();
    h->setDefaultAlignment(Qt::AlignLeft);
    h->setSectionsClickable(true);
    h->viewport()->installEventFilter(d);

    connect(this, SIGNAL(activated(QModelIndex)),
        d, SLOT(rowActivatedHelper(QModelIndex)));
    connect(this, SIGNAL(clicked(QModelIndex)),
        d, SLOT(rowClickedHelper(QModelIndex)));
    connect(h, SIGNAL(sectionClicked(int)),
        d, SLOT(toggleColumnWidth(int)));
    connect(h, SIGNAL(sectionResized(int,int,int)),
        d, SLOT(handleSectionResized(int,int,int)));
}

BaseTreeView::~BaseTreeView()
{
    delete d;
}

void BaseTreeView::setModel(QAbstractItemModel *m)
{
    QAbstractItemModel *oldModel = model();
    const char *sig = "columnAdjustmentRequested()";
    if (oldModel) {
        int index = model()->metaObject()->indexOfSignal(sig);
        if (index != -1)
            disconnect(model(), SIGNAL(columnAdjustmentRequested()), d, SLOT(resizeColumns()));
    }
    TreeView::setModel(m);
    if (m) {
        int index = m->metaObject()->indexOfSignal(sig);
        if (index != -1)
            connect(m, SIGNAL(columnAdjustmentRequested()), d, SLOT(resizeColumns()));
        d->restoreState();
    }
}

void BaseTreeView::mousePressEvent(QMouseEvent *ev)
{
    TreeView::mousePressEvent(ev);
    const QModelIndex mi = indexAt(ev->pos());
    if (!mi.isValid())
        d->toggleColumnWidth(columnAt(ev->x()));
}

void BaseTreeView::setSettings(QSettings *settings, const QByteArray &key)
{
    QTC_ASSERT(!d->m_settings, qDebug() << "DUPLICATED setSettings" << key);
    d->m_settings = settings;
    d->m_settingsKey = QString::fromLatin1(key);
    d->readSettings();
}

QModelIndexList BaseTreeView::activeRows() const
{
    QItemSelectionModel *selection = selectionModel();
    QModelIndexList indices = selection->selectedRows();
    if (indices.isEmpty()) {
        QModelIndex current = selection->currentIndex();
        if (current.isValid())
            indices.append(current);
    }
    return indices;
}

} // namespace Utils

#include "basetreeview.moc"
