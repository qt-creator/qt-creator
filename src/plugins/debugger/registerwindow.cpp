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

#include "registerwindow.h"
#include "registerhandler.h"

#include "debuggeractions.h"
#include "debuggeragents.h"
#include "debuggerconstants.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QFileInfoList>

#include <QtGui/QAction>
#include <QtGui/QHeaderView>
#include <QtGui/QItemDelegate>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtGui/QToolButton>


namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// RegisterDelegate
//
///////////////////////////////////////////////////////////////////////

class RegisterDelegate : public QItemDelegate
{
public:
    RegisterDelegate(DebuggerManager *manager, QObject *parent)
        : QItemDelegate(parent), m_manager(manager)
    {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &) const
    {
        QLineEdit *lineEdit = new QLineEdit(parent);
        lineEdit->setAlignment(Qt::AlignRight);
        return lineEdit;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
        QTC_ASSERT(lineEdit, return);
        lineEdit->setText(index.model()->data(index, Qt::DisplayRole).toString());
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
        const QModelIndex &index) const
    {
        Q_UNUSED(model);
        //qDebug() << "SET MODEL DATA";
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
        QTC_ASSERT(lineEdit, return);
        QString value = lineEdit->text();
        //model->setData(index, value, Qt::EditRole);
        if (index.column() == 1)
            m_manager->setRegisterValue(index.row(), value);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
        const QModelIndex &) const
    {
        editor->setGeometry(option.rect);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const
    {
        if (index.column() == 1) {
            bool paintRed = index.data(RegisterChangedRole).toBool();
            QPen oldPen = painter->pen();
            if (paintRed)
                painter->setPen(QColor(200, 0, 0));
            // FIXME: performance? this changes only on real font changes.
            QFontMetrics fm(option.font);
            int charWidth = fm.width(QLatin1Char('x'));
            for (int i = '1'; i <= '9'; ++i)
                charWidth = qMax(charWidth, fm.width(QLatin1Char(i)));
            for (int i = 'a'; i <= 'f'; ++i)
                charWidth = qMax(charWidth, fm.width(QLatin1Char(i)));
            QString str = index.data(Qt::DisplayRole).toString();
            int x = option.rect.x();
            for (int i = 0; i < str.size(); ++i) {
                QRect r = option.rect;
                r.setX(x);
                r.setWidth(charWidth);
                x += charWidth;
                painter->drawText(r, Qt::AlignHCenter, QString(str.at(i)));
            }
            if (paintRed)
                painter->setPen(oldPen);
        } else {
            QItemDelegate::paint(painter, option, index);
        }
    }

private:
    DebuggerManager *m_manager;
};


///////////////////////////////////////////////////////////////////////
//
// RegisterWindow
//
///////////////////////////////////////////////////////////////////////

RegisterWindow::RegisterWindow(DebuggerManager *manager)
  : m_manager(manager), m_alwaysResizeColumnsToContents(true)
{
    QAction *act = theDebuggerAction(UseAlternatingRowColors);
    setWindowTitle(tr("Registers"));
    setAlternatingRowColors(act->isChecked());
    setRootIsDecorated(false);
    setItemDelegate(new RegisterDelegate(m_manager, this));

    connect(act, SIGNAL(toggled(bool)),
        this, SLOT(setAlternatingRowColorsHelper(bool)));
}

void RegisterWindow::resizeEvent(QResizeEvent *ev)
{
    QTreeView::resizeEvent(ev);
}

void RegisterWindow::contextMenuEvent(QContextMenuEvent *ev)
{
    QMenu menu;

    QAction *actReload = menu.addAction(tr("Reload register listing"));
    menu.addSeparator();

    QModelIndex idx = indexAt(ev->pos());
    QString address = model()->data(idx, RegisterAddressRole).toString();
    QAction *actShowMemory = menu.addAction(QString());
    if (address.isEmpty()) {
        actShowMemory->setText(tr("Open memory editor"));
        actShowMemory->setEnabled(false);
    } else {
        actShowMemory->setText(tr("Open memory editor at %1").arg(address));
    }
    menu.addSeparator();

    int base = model()->data(QModelIndex(), RegisterNumberBaseRole).toInt();
    QAction *act16 = menu.addAction(tr("Hexadecimal"));
    act16->setCheckable(true);
    act16->setChecked(base == 16);
    QAction *act10 = menu.addAction(tr("Decimal"));
    act10->setCheckable(true);
    act10->setChecked(base == 10);
    QAction *act8 = menu.addAction(tr("Octal"));
    act8->setCheckable(true);
    act8->setChecked(base == 8);
    QAction *act2 = menu.addAction(tr("Binary"));
    act2->setCheckable(true);
    act2->setChecked(base == 2);
    menu.addSeparator();

    QAction *actAdjust = menu.addAction(tr("Adjust column widths to contents"));
    QAction *actAlwaysAdjust =
        menu.addAction(tr("Always adjust column widths to contents"));
    actAlwaysAdjust->setCheckable(true);
    actAlwaysAdjust->setChecked(m_alwaysResizeColumnsToContents);
    menu.addSeparator();

    menu.addAction(theDebuggerAction(SettingsDialog));

    QAction *act = menu.exec(ev->globalPos());
 
    if (act == actAdjust)
        resizeColumnsToContents();
    else if (act == actAlwaysAdjust)
        setAlwaysResizeColumnsToContents(!m_alwaysResizeColumnsToContents);
    else if (act == actReload)
        m_manager->reloadRegisters();
    else if (act == actShowMemory)
        (void) new MemoryViewAgent(m_manager, address);
    else if (act) {
        base = (act == act10 ? 10 : act == act8 ? 8 : act == act2 ? 2 : 16);
        QMetaObject::invokeMethod(model(), "setNumberBase", Q_ARG(int, base));
    }
}

void RegisterWindow::resizeColumnsToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

void RegisterWindow::setAlwaysResizeColumnsToContents(bool on)
{
    m_alwaysResizeColumnsToContents = on;
    QHeaderView::ResizeMode mode = on
        ? QHeaderView::ResizeToContents : QHeaderView::Interactive;
    header()->setResizeMode(0, mode);
    header()->setResizeMode(1, mode);
}


void RegisterWindow::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    setAlwaysResizeColumnsToContents(true);
}

} // namespace Internal
} // namespace Debugger
