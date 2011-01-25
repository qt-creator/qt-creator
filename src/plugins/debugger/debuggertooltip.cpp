/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggertooltip.h"

#include <utils/qtcassert.h>

#include <QtCore/QtDebug>
#include <QtCore/QPointer>

#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QHeaderView>
#include <QtGui/QScrollBar>
#include <QtGui/QTreeView>
#include <QtGui/QSortFilterProxyModel>

namespace Debugger {
namespace Internal {

class ToolTipWidget : public QTreeView
{
    Q_OBJECT

public:
    ToolTipWidget(QWidget *parent);

    QSize sizeHint() const { return m_size; }

    void done();
    void run(const QPoint &point, const QModelIndex &index);
    int computeHeight(const QModelIndex &index) const;
    Q_SLOT void computeSize();

    void leaveEvent(QEvent *ev);

private:
    QSize m_size;
};

static QPointer<ToolTipWidget> theToolTipWidget;


ToolTipWidget::ToolTipWidget(QWidget *parent)
    : QTreeView(parent)
{
    setWindowFlags(Qt::ToolTip | Qt::WindowStaysOnTopHint);
    setFocusPolicy(Qt::NoFocus);

    header()->hide();

    setUniformRowHeights(true);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(computeSize()),
        Qt::QueuedConnection);
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(computeSize()),
        Qt::QueuedConnection);
}

int ToolTipWidget::computeHeight(const QModelIndex &index) const
{
    int s = rowHeight(index);
    for (int i = 0; i < model()->rowCount(index); ++i)
        s += computeHeight(model()->index(i, 0, index));
    return s;
}

void ToolTipWidget::computeSize()
{
    int columns = 0;
    for (int i = 0; i < 3; ++i) {
        resizeColumnToContents(i);
        columns += sizeHintForColumn(i);
    }
    int rows = computeHeight(QModelIndex());

    // Fit tooltip to screen, showing/hiding scrollbars as needed.
    // Add a bit of space to account for tooltip border, and not
    // touch the border of the screen.
    QPoint pos(x(), y());
    QRect desktopRect = QApplication::desktop()->availableGeometry(pos);
    const int maxWidth = desktopRect.right() - pos.x() - 5 - 5;
    const int maxHeight = desktopRect.bottom() - pos.y() - 5 - 5;

    if (columns > maxWidth)
        rows += horizontalScrollBar()->height();

    if (rows > maxHeight) {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        rows = maxHeight;
        columns += verticalScrollBar()->width();
    } else {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    if (columns > maxWidth) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        columns = maxWidth;
    } else {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    m_size = QSize(columns + 5, rows + 5);
    setMinimumSize(m_size);
    setMaximumSize(m_size);
}

void ToolTipWidget::done()
{
    deleteLater();
}

void ToolTipWidget::run(const QPoint &point, const QModelIndex &index)
{
    QAbstractItemModel *model = const_cast<QAbstractItemModel *>(index.model());
    move(point);
    setModel(model);
    // Track changes in filter models.
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(computeSize()), Qt::QueuedConnection);
    computeSize();
    setRootIsDecorated(model->hasChildren(index));
}

void ToolTipWidget::leaveEvent(QEvent *ev)
{
    Q_UNUSED(ev);
    if (QApplication::keyboardModifiers() == Qt::NoModifier)
        hide();
}

void showDebuggerToolTip(const QPoint &point, const QModelIndex &index)
{
    if (index.model()) {
        if (!theToolTipWidget)
            theToolTipWidget = new ToolTipWidget(0);
        theToolTipWidget->run(point, index);
        theToolTipWidget->show();
    } else if (theToolTipWidget) {
        theToolTipWidget->done();
        theToolTipWidget = 0;
    }
}

// Model for tooltips filtering a local variable using the locals model.
class ToolTipRowFilterModel : public QSortFilterProxyModel
{
public:
    // Index points the variable to be filtered.
    explicit ToolTipRowFilterModel(QAbstractItemModel *model, int row, QObject *parent = 0);
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    const int m_row;
};

ToolTipRowFilterModel::ToolTipRowFilterModel(QAbstractItemModel *model, int row, QObject *parent) :
    QSortFilterProxyModel(parent), m_row(row)
{
    setSourceModel(model);
}

bool ToolTipRowFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // Match on row for top level, else pass through.
    return sourceParent.isValid() || sourceRow == m_row;
}

// Show tooltip filtering a row of a source model.
void showDebuggerToolTip(const QPoint &point, QAbstractItemModel *model, int row)
{
    // Create a filter model parented on the widget to display column
    ToolTipRowFilterModel *filterModel = new ToolTipRowFilterModel(model, row);
    showDebuggerToolTip(point, filterModel->index(0, 0));
    QTC_ASSERT(theToolTipWidget, return; )
    filterModel->setParent(theToolTipWidget);
}

void hideDebuggerToolTip(int delay)
{
    Q_UNUSED(delay)
    if (theToolTipWidget)
        theToolTipWidget->done();
}

} // namespace Internal
} // namespace Debugger

#include "debuggertooltip.moc"
