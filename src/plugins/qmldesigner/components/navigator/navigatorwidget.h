// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/icontext.h>

#include <QFrame>
#include <QPointer>

#include "navigatortreeview.h"

QT_FORWARD_DECLARE_CLASS(QToolBar)
QT_FORWARD_DECLARE_CLASS(QToolButton)
QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)

namespace QmlDesigner {

class NavigatorView;
class NavigatorSearchWidget;

class NavigatorWidget: public QFrame
{
    Q_OBJECT

public:
    NavigatorWidget(NavigatorView *view);

    void setTreeModel(QAbstractItemModel *model);
    QTreeView *treeView() const;
    QList<QWidget *> createToolBarWidgets();
    QToolBar *createToolBar();
    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    void disableNavigator();
    void enableNavigator();

    void setDragType(const QByteArray &type);
    QByteArray dragType() const;

    void clearSearch();

signals:
    void leftButtonClicked();
    void rightButtonClicked();
    void upButtonClicked();
    void downButtonClicked();
    void filterToggled(bool);
    void reverseOrderToggled(bool);
    void textFilterChanged(const QString &name);

protected:
    void dragEnterEvent(QDragEnterEvent *dragEnterEvent) override;
    void dropEvent(QDropEvent *dropEvent) override;

private:
    NavigatorView *navigatorView() const;

    NavigatorTreeView *m_treeView;
    QPointer<NavigatorView> m_navigatorView;
    QByteArray m_dragType;
    NavigatorSearchWidget *m_searchWidget;
};

}
