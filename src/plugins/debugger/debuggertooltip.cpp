/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "debuggertooltip.h"

#include <QtCore/QPointer>
#include <QtCore/QtDebug>

#include <QtGui/QApplication>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>


namespace Debugger {
namespace Internal {

class ToolTipWidget : public QTreeView
{
    Q_OBJECT

public:
    ToolTipWidget(QWidget *parent);

    bool eventFilter(QObject *ob, QEvent *ev);
    QSize sizeHint() const { return m_size; }

    void done();
    void run(const QPoint &point, QAbstractItemModel *model,
        const QModelIndex &index, const QString &msg);
    int computeHeight(const QModelIndex &index) const;
    Q_SLOT void computeSize();

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

    qApp->installEventFilter(this);
}

bool ToolTipWidget::eventFilter(QObject *ob, QEvent *ev)
{
    Q_UNUSED(ob)
    switch (ev->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent *>(ev)->key() == Qt::Key_Escape)
            return true;
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent *>(ev)->key() == Qt::Key_Escape) {
            return true;
        }
        break;
    case QEvent::KeyRelease:
        if (static_cast<QKeyEvent *>(ev)->key() == Qt::Key_Escape) {
            done();
            return true;
        }
        break;
    case QEvent::WindowDeactivate:
    case QEvent::FocusOut:
        done();
        break;
    default:
        break;
    }
    return false;
}

int ToolTipWidget::computeHeight(const QModelIndex &index) const
{
    int s = rowHeight(index);
    for (int i = 0; i < model()->rowCount(index); ++i)
        s += computeHeight(model()->index(i, 0, index));
    return s;
}

Q_SLOT void ToolTipWidget::computeSize()
{
    int columns = 0;
    for (int i = 0; i < 3; ++i) {
        resizeColumnToContents(i);
        columns += sizeHintForColumn(i);
    }
    int rows = computeHeight(QModelIndex());
    m_size = QSize(columns + 5, rows + 5);
    setMinimumSize(m_size);
    setMaximumSize(m_size);
}

void ToolTipWidget::done()
{
    qApp->removeEventFilter(this);
    deleteLater();
}

void ToolTipWidget::run(const QPoint &point, QAbstractItemModel *model,
    const QModelIndex &index, const QString & /* msg */)
{
    move(point);
    setModel(model);
    setRootIndex(index.parent());
    computeSize();
    setRootIsDecorated(model->hasChildren(index));
    // FIXME: use something more sensible
    QPalette pal = palette();
    QColor bg = pal.color(QPalette::Base);
    bg.setAlpha(20);
    pal.setColor(QPalette::Base, bg);
    setPalette(pal);
    //viewport()->setPalette(pal);
}

void showDebuggerToolTip(const QPoint &point, QAbstractItemModel *model,
        const QModelIndex &index, const QString &msg)
{
    if (model) {
        if (!theToolTipWidget)
            theToolTipWidget = new ToolTipWidget(0);
        theToolTipWidget->run(point, model, index, msg);
        theToolTipWidget->show();
    } else if (theToolTipWidget) {
        theToolTipWidget->done();
        theToolTipWidget = 0;
    }
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
