/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASETREEVIEW_H
#define BASETREEVIEW_H

#include "utils_global.h"

#include <QTreeView>

namespace Utils {

class QTCREATOR_UTILS_EXPORT BaseTreeView : public QTreeView
{
    Q_OBJECT

public:
    BaseTreeView(QWidget *parent = 0);

    void setAlwaysAdjustColumnsAction(QAction *action);
    virtual void addBaseContextActions(QMenu *menu);
    bool handleBaseContextAction(QAction *action);
    QModelIndexList activeRows() const;

    void setModel(QAbstractItemModel *model);
    virtual void rowActivated(const QModelIndex &) {}
    virtual void rowClicked(const QModelIndex &) {}
    void mousePressEvent(QMouseEvent *ev);

public slots:
    void resizeColumnsToContents();
    void setAlwaysResizeColumnsToContents(bool on);
    void reset();

protected slots:
    void setAlternatingRowColorsHelper(bool on) { setAlternatingRowColors(on); }

private slots:
    void rowActivatedHelper(const QModelIndex &index) { rowActivated(index); }
    void rowClickedHelper(const QModelIndex &index) { rowClicked(index); }
    void headerSectionClicked(int logicalIndex);

private:
    QAction *m_alwaysAdjustColumnsAction;
    QAction *m_adjustColumnsAction;
};

} // namespace Utils

#endif // BASETREEVIEW_H
