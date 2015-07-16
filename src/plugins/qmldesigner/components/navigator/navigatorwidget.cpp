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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "navigatorwidget.h"
#include "navigatorview.h"

#include <QBoxLayout>
#include <QToolButton>
#include <QAbstractItemModel>
#include <QHeaderView>
#include <QtDebug>
#include <utils/fileutils.h>


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

    setStyleSheet(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/stylesheet.css"))));
    m_treeView->setStyleSheet(QString::fromUtf8(Utils::FileReader::fetchQrc(QLatin1String(":/qmldesigner/scrollbar.css"))));
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

    buttons.append(new QToolButton());
    buttons.last()->setIcon(QIcon(QLatin1String(":/navigator/icon/arrowleft.png")));
    buttons.last()->setToolTip(tr("Become last sibling of parent (CTRL + Left)."));
    buttons.last()->setShortcut(QKeySequence(Qt::Key_Left | Qt::CTRL));
    connect(buttons.last(), SIGNAL(clicked()), this, SIGNAL(leftButtonClicked()));
    buttons.append(new QToolButton());
    buttons.last()->setIcon(QIcon(QLatin1String(":/navigator/icon/arrowright.png")));
    buttons.last()->setToolTip(tr("Become child of last sibling (CTRL + Right)."));
    buttons.last()->setShortcut(QKeySequence(Qt::Key_Right | Qt::CTRL));
    connect(buttons.last(), SIGNAL(clicked()), this, SIGNAL(rightButtonClicked()));

    buttons.append(new QToolButton());
    buttons.last()->setIcon(QIcon(QLatin1String(":/navigator/icon/arrowdown.png")));
    buttons.last()->setToolTip(tr("Move down (CTRL + Down)."));
    buttons.last()->setShortcut(QKeySequence(Qt::Key_Down | Qt::CTRL));
    connect(buttons.last(), SIGNAL(clicked()), this, SIGNAL(downButtonClicked()));

    buttons.append(new QToolButton());
    buttons.last()->setIcon(QIcon(QLatin1String(":/navigator/icon/arrowup.png")));
    buttons.last()->setToolTip(tr("Move up (CTRL + Up)."));
    buttons.last()->setShortcut(QKeySequence(Qt::Key_Up | Qt::CTRL));
    connect(buttons.last(), SIGNAL(clicked()), this, SIGNAL(upButtonClicked()));

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
