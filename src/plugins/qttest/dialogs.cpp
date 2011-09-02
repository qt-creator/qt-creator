/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "dialogs.h"
#include "qsystem.h"

#include <QTimer>
#include <QAbstractItemView>
#include <QTableWidgetItem>
#include <QResizeEvent>
#include <QScrollBar>
#include <QPushButton>

SelectDlg::SelectDlg(const QString &title, const QString &helpText,
    QAbstractItemView::SelectionMode mode, int width, int height,
    const QStringList &columns, const QByteArray &colWidths, QWidget *parent) : QDialog(parent)
{
    m_colWidths = colWidths;

    setupUi(this);
    setModal(true);
    setWindowTitle(title);
    instructLabel->setText(helpText);
    groupsList->setSelectionMode(mode);
    resize(width, height);

    groupsList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    groupsList->setColumnCount(columns.count());
    groupsList->setHorizontalHeaderLabels(columns);
    groupsList->setSelectionBehavior(QAbstractItemView::SelectRows);
    groupsList->setSortingEnabled(true);
    groupsList->sortByColumn(0, Qt::AscendingOrder);
    groupsList->setAlternatingRowColors(true);
    groupsList->verticalHeader()->hide();
    groupsList->horizontalHeader()->show();

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    groupsList->setWordWrap(true);
    connect(groupsList, SIGNAL(itemSelectionChanged()),
        this, SLOT(onItemSelectionChanged()), Qt::DirectConnection);
    QTimer::singleShot(0, this, SLOT(resizeEvent()));
}

void SelectDlg::resizeEvent()
{
    resizeEvent(0);
}

void SelectDlg::addSelectableItems(const QStringList &items)
{
    foreach (const QString &item, items)
        addSelectableItem(item);
}

void SelectDlg::addSelectableItem(const QString &item)
{
    groupsList->setSortingEnabled(false);
    if (groupsList->columnCount() > 1) {
        int row = groupsList->rowCount();
        groupsList->insertRow(row);
        QStringList tmp = item.split(QLatin1Char('|'));
        if (tmp.count() > groupsList->columnCount()) {
            for (int i = 0; i < (groupsList->columnCount() - 1); ++i)
                groupsList->setItem(row, i, new QTableWidgetItem(tmp[i]));

            QString s;
            for (int i = groupsList->columnCount(); i < tmp.count(); ++i) {
                if (!s.isEmpty()) s+= QLatin1Char('|');
                s += tmp[i];
            }
            groupsList->setItem(row, (groupsList->columnCount() - 1), new QTableWidgetItem(s));
        } else if (tmp.count() < groupsList->columnCount()) {
            for (int i = 0; i < tmp.count(); ++i)
                groupsList->setItem(row, i, new QTableWidgetItem(tmp[i]));
            for (int i = tmp.count(); i < groupsList->columnCount(); ++i)
                groupsList->setItem(row, i, new QTableWidgetItem(QString()));
        } else {
            for (int i = 0; i < tmp.count(); ++i)
                groupsList->setItem(row, i, new QTableWidgetItem(tmp[i]));
        }
    } else {
        // In single column mode we accept no duplicates
        if (groupsList->findItems(item, Qt::MatchCaseSensitive).count() <= 0) {
            int row = groupsList->rowCount();
            groupsList->insertRow(row);
            groupsList->setItem(row, 0, new QTableWidgetItem(item));
        }
    }
    groupsList->setSortingEnabled(true);
}

QStringList SelectDlg::selectedItems()
{
    QStringList ret;
    QTableWidgetItem *item;
    for (int row = 0; row < groupsList->rowCount(); ++row) {
        item = groupsList->item(row, 0);
        if (item && item->isSelected()) {
            QString S = item->text();
            for (int col = 1; col < groupsList->columnCount(); ++col) {
                S+= QLatin1Char('|');
                item = groupsList->item(row, col);
                if (item)
                    S += item->text();
            }
            ret.append(S);
        }
    }
    return ret;
}

void SelectDlg::setColumnWidth(int col, int width)
{
    groupsList->setColumnWidth(col, width);
}

void SelectDlg::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    int fixedSize = 4;
    const int extraSize = 80;
    int total = 0;

    if (groupsList->verticalScrollBar()->isVisible())
        fixedSize += groupsList->verticalScrollBar()->width();

    for (int i = 0; i < groupsList->columnCount(); ++i) {
        if (i < m_colWidths.count()) {
            if (static_cast<int>(m_colWidths[i]) > 0) {
                setColumnWidth(i, m_colWidths[i]);
                total += m_colWidths[i];
            }
        } else if (i == 0) {
            int extra = groupsList->columnCount() - 1;
            int colWidth = (groupsList->width() - fixedSize) - (extraSize * extra);
            setColumnWidth(0, colWidth);
        } else {
            setColumnWidth(i, extraSize);
        }
    }

    if (m_colWidths.count() > 0) {
        for (int i = 0; i < groupsList->columnCount(); ++i) {
            if ((i < m_colWidths.count()) && (static_cast<int>(m_colWidths[i]) == 0)) {
                int colWidth = (groupsList->width() - fixedSize) - total;
                setColumnWidth(i, colWidth);
            }
        }
    }

    groupsList->resizeRowsToContents();
}

void SelectDlg::resizeContents()
{
    groupsList->resizeRowsToContents();
}

void SelectDlg::onItemSelectionChanged()
{
    QList<QTableWidgetItem *> items = groupsList->selectedItems();
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(items.count() > 0);
}

//***********************************

NewTestFunctionDlg::NewTestFunctionDlg(const QString &testCase, QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    setModal(true);
    testcaseLabel->setText(testCase);

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(testFunctionName,SIGNAL(textChanged(QString)),
        this, SLOT(onChanged()), Qt::DirectConnection);

    resizeEvent(0);
}

void NewTestFunctionDlg::onChanged()
{
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!testFunctionName->text().isEmpty());
}
