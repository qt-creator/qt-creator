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

#include "navigatorwidget.h"
#include "navigatorview.h"
#include "qmldesignerconstants.h"
#include "qmldesignericons.h"
#include <designersettings.h>
#include <theme.h>

#include <QBoxLayout>
#include <QToolButton>
#include <QAbstractItemModel>
#include <QMenu>
#include <QHeaderView>
#include <QtDebug>
#include <utils/fileutils.h>
#include <utils/utilsicons.h>

namespace QmlDesigner {

NavigatorWidget::NavigatorWidget(NavigatorView *view) :
        QFrame(),
        m_treeView(new NavigatorTreeView),
        m_navigatorView(view)
{
    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->setDefaultDropAction(Qt::LinkAction);
    m_treeView->setHeaderHidden(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(m_treeView);

    setLayout(layout);

    setWindowTitle(tr("Navigator", "Title of navigator view"));

#ifndef QMLDESIGNER_TEST
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/stylesheet.css")))));
    m_treeView->setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css")))));
#endif
}

void NavigatorWidget::setTreeModel(QAbstractItemModel* model)
{
    m_treeView->setModel(model);
}

QTreeView *NavigatorWidget::treeView() const
{
    return m_treeView;
}

QList<QToolButton *> NavigatorWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;


    auto button = new QToolButton();
    button->setIcon(Icons::ARROW_LEFT.icon());
    button->setToolTip(tr("Become last sibling of parent (CTRL + Left)."));
    button->setShortcut(QKeySequence(Qt::Key_Left | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::leftButtonClicked);
    buttons.append(button);

    button = new QToolButton();
    button->setIcon(Icons::ARROW_RIGHT.icon());
    button->setToolTip(tr("Become child of last sibling (CTRL + Right)."));
    button->setShortcut(QKeySequence(Qt::Key_Right | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::rightButtonClicked);
    buttons.append(button);

    button = new QToolButton();
    button->setIcon(Icons::ARROW_DOWN.icon());
    button->setToolTip(tr("Move down (CTRL + Down)."));
    button->setShortcut(QKeySequence(Qt::Key_Down | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::downButtonClicked);
    buttons.append(button);

    button = new QToolButton();
    button->setIcon(Icons::ARROW_UP.icon());
    button->setToolTip(tr("Move up (CTRL + Up)."));
    button->setShortcut(QKeySequence(Qt::Key_Up | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::upButtonClicked);
    buttons.append(button);

    auto filter = new QToolButton;
    filter->setIcon(Utils::Icons::FILTER.icon());
    filter->setToolTip(tr("Filter Tree"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty("noArrow", true);
    auto filterMenu = new QMenu(filter);
    auto objectAction = new QAction(tr("Show only visible items."), nullptr);
    objectAction->setCheckable(true);
    objectAction->setChecked(
                DesignerSettings::getValue(DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS).toBool());
    connect(objectAction, &QAction::toggled, this, &NavigatorWidget::filterToggled);
    filterMenu->addAction(objectAction);
    filter->setMenu(filterMenu);
    buttons.append(filter);

    return buttons;
}

QString NavigatorWidget::contextHelpId() const
{
    if (navigatorView())
        return  navigatorView()->contextHelpId();

    return QString();
}

NavigatorView *NavigatorWidget::navigatorView() const
{
    return m_navigatorView.data();
}

}
