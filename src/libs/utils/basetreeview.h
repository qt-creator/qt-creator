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

#ifndef BASETREEVIEW_H
#define BASETREEVIEW_H

#include "utils_global.h"

#include "itemviews.h"

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {

namespace Internal { class BaseTreeViewPrivate; }

class QTCREATOR_UTILS_EXPORT BaseTreeView : public TreeView
{
    Q_OBJECT

public:
    enum { ExtraIndicesForColumnWidth = 12734 };

    BaseTreeView(QWidget *parent = 0);
    ~BaseTreeView();

    void setSettings(QSettings *settings, const QByteArray &key);
    QModelIndexList activeRows() const;

    virtual void rowActivated(const QModelIndex &) {}
    virtual void rowClicked(const QModelIndex &) {}

    void setModel(QAbstractItemModel *model) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void showEvent(QShowEvent *ev) override;

    void showProgressIndicator();
    void hideProgressIndicator();

signals:
    void aboutToShow();

public slots:
    void setAlternatingRowColorsHelper(bool on) { setAlternatingRowColors(on); }

private:
    Internal::BaseTreeViewPrivate *d;
};

} // namespace Utils

#endif // BASETREEVIEW_H
