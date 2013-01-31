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

#include <QBoxLayout>
#include <QTreeView>
#include <QHeaderView>
#include <model.h>

#include "navigatorwidget.h"
#include "navigatorview.h"

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

    setStyleSheet(QLatin1String(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css")));
    m_treeView->setStyleSheet(
            QLatin1String(Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css")));
}

NavigatorWidget::~NavigatorWidget()
{

}

void NavigatorWidget::setTreeModel(QAbstractItemModel* model)
{
    m_treeView->setModel(model);
}

QList<QToolButton *> NavigatorWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;

    buttons << new QToolButton();
    buttons.last()->setIcon(QIcon(":/navigator/icon/arrowleft.png"));
    buttons.last()->setToolTip(tr("Become first sibling of parent (CTRL + Left)"));
    buttons.last()->setShortcut(QKeySequence(Qt::Key_Left | Qt::CTRL));
    connect(buttons.last(), SIGNAL(clicked()), this, SIGNAL(leftButtonClicked()));
    buttons << new QToolButton();
    buttons.last()->setIcon(QIcon(":/navigator/icon/arrowright.png"));
    buttons.last()->setToolTip(tr("Become child of first sibling (CTRL + Right)"));
    buttons.last()->setShortcut(QKeySequence(Qt::Key_Right | Qt::CTRL));
    connect(buttons.last(), SIGNAL(clicked()), this, SIGNAL(rightButtonClicked()));

    buttons << new QToolButton();
    buttons.last()->setIcon(QIcon(":/navigator/icon/arrowdown.png"));
    buttons.last()->setToolTip(tr("Move down (CTRL + Down)"));
    buttons.last()->setShortcut(QKeySequence(Qt::Key_Down | Qt::CTRL));
    connect(buttons.last(), SIGNAL(clicked()), this, SIGNAL(downButtonClicked()));

    buttons << new QToolButton();
    buttons.last()->setIcon(QIcon(":/navigator/icon/arrowup.png"));
    buttons.last()->setToolTip(tr("Move up (CTRL + Up)"));
    buttons.last()->setShortcut(QKeySequence(Qt::Key_Up | Qt::CTRL));
    connect(buttons.last(), SIGNAL(clicked()), this, SIGNAL(upButtonClicked()));

    return buttons;
}

QString NavigatorWidget::contextHelpId() const
{
    if (!navigatorView())
        return QString();

    QList<ModelNode> nodes = navigatorView()->selectedModelNodes();
    QString helpId;
    if (!nodes.isEmpty()) {
        helpId = nodes.first().type();
        helpId.replace("QtQuick", "QML");
    }

    return helpId;
}

NavigatorView *NavigatorWidget::navigatorView() const
{
    return m_navigatorView.data();
}

}
