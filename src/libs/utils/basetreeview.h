/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

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

    void setModel(QAbstractItemModel *model);
    virtual void rowActivated(const QModelIndex &) {}
    void mousePressEvent(QMouseEvent *ev);

public slots:
    void resizeColumnsToContents();
    void setAlwaysResizeColumnsToContents(bool on);
    void reset();

protected slots:
    void setAlternatingRowColorsHelper(bool on) { setAlternatingRowColors(on); }

private slots:
    void rowActivatedHelper(const QModelIndex &index) { rowActivated(index); }
    void headerSectionClicked(int logicalIndex);

private:
    QAction *m_alwaysAdjustColumnsAction;
    QAction *m_adjustColumnsAction;
};

} // namespace Utils

#endif // BASETREEVIEW_H
