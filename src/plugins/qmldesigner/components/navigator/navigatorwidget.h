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

#pragma once

#include <coreplugin/icontext.h>

#include <QFrame>
#include <QPointer>

#include "navigatortreeview.h"

QT_FORWARD_DECLARE_CLASS(QToolButton)
QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)

namespace QmlDesigner {

class NavigatorView;

class NavigatorWidget: public QFrame
{
    Q_OBJECT
public:
    NavigatorWidget(NavigatorView *view);

    void setTreeModel(QAbstractItemModel *model);
    QTreeView *treeView() const;
    QList<QToolButton *> createToolBarWidgets();
    void contextHelpId(const Core::IContext::HelpIdCallback &callback) const;

    void disableNavigator();
    void enableNavigator();

signals:
    void leftButtonClicked();
    void rightButtonClicked();
    void upButtonClicked();
    void downButtonClicked();
    void filterToggled(bool);

private: // functions
    NavigatorView *navigatorView() const;

private: // variables
    NavigatorTreeView *m_treeView;
    QPointer<NavigatorView> m_navigatorView;
};

}
