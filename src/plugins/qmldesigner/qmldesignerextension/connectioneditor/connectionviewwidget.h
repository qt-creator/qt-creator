/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef CONNECTIONVIEWWIDGET_H
#define CONNECTIONVIEWWIDGET_H

#include <QFrame>
#include <QAbstractItemView>

QT_BEGIN_NAMESPACE
class QToolButton;
class QTableView;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Ui { class ConnectionViewWidget; }

namespace Internal {

class BindingModel;
class ConnectionModel;
class DynamicPropertiesModel;

class ConnectionViewWidget : public QFrame
{
    Q_OBJECT

public:

    enum TabStatus {
        ConnectionTab,
        BindingTab,
        DynamicPropertiesTab,
        InvalidTab
    };

    explicit ConnectionViewWidget(QWidget *parent = 0);
    ~ConnectionViewWidget();

    void setBindingModel(BindingModel *model);
    void setConnectionModel(ConnectionModel *model);
    void setDynamicPropertiesModelModel(DynamicPropertiesModel *model);

    QList<QToolButton*> createToolBarWidgets();

    void setEnabledAddButton(bool enabled);
    void setEnabledRemoveButton(bool enabled);

    TabStatus currentTab() const;

    void resetItemViews();
    void invalidateButtonStatus();

    QTableView *connectionTableView() const;
    QTableView *bindingTableView() const;
    QTableView *dynamicPropertiesTableView() const;

public slots:
    void bindingTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void connectionTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void dynamicPropertiesTableViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);

signals:
    void setEnabledAddButtonChanged(bool);
    void setEnabledRemoveButtonChanged(bool);

private slots:
    void handleTabChanged(int i);
    void removeButtonClicked();
    void addButtonClicked();

private:
    Ui::ConnectionViewWidget *ui;
};

} // namespace Internal

} // namespace QmlDesigner

#endif // CONNECTIONVIEWWIDGET_H
