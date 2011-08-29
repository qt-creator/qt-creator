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

#ifndef DIALOGS_H
#define DIALOGS_H

#include "ui_selectdlg.h"
#include "ui_newtestfunctiondlg.h"

class SelectDlg : public QDialog, public Ui::SelectDlg
{
    Q_OBJECT

public:
    SelectDlg(const QString &title, const QString &helpText,
        QAbstractItemView::SelectionMode mode, int width, int height,
        const QStringList &columns, const QByteArray &colWidths, QWidget *parent = 0);
    void addSelectableItem(const QString &item);
    void addSelectableItems(const QStringList &items);
    QStringList selectedItems();
    void resizeContents();

private slots:
    void onItemSelectionChanged();
    void resizeEvent();

private:
    void setColumnWidth(int col, int width);
    void resizeEvent(QResizeEvent *event);

    QByteArray m_colWidths;
};

class NewTestFunctionDlg : public QDialog, public Ui::NewTestFunctionDlg
{
    Q_OBJECT

public:
    explicit NewTestFunctionDlg(const QString &testCase, QWidget *parent = 0);

private slots:
    void onChanged();
};

#endif  // DIALOGS_H
