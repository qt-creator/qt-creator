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

#include "basewindow.h"

#include "debuggeractions.h"
#include "debuggercore.h"

#include <aggregation/aggregate.h>
#include <coreplugin/findplaceholder.h>
#include <find/treeviewfind.h>
#include <utils/savedaction.h>

#include <QMenu>
#include <QVBoxLayout>

namespace Debugger {
namespace Internal {

BaseTreeView::BaseTreeView(QWidget *parent)
    : Utils::BaseTreeView(parent)
{
    QAction *act = debuggerCore()->action(UseAlternatingRowColors);
    setAlternatingRowColors(act->isChecked());
    connect(act, SIGNAL(toggled(bool)),
            SLOT(setAlternatingRowColorsHelper(bool)));
}

void BaseTreeView::addBaseContextActions(QMenu *menu)
{
    Utils::BaseTreeView::addBaseContextActions(menu);
    menu->addAction(debuggerCore()->action(SettingsDialog));
}

BaseWindow::BaseWindow(QTreeView *treeView, QWidget *parent)
    : QWidget(parent), m_treeView(treeView)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    vbox->setSpacing(0);
    vbox->addWidget(m_treeView);
    vbox->addWidget(new Core::FindToolBarPlaceHolder(this));

    Aggregation::Aggregate *agg = new Aggregation::Aggregate;
    agg->add(m_treeView);
    agg->add(new Find::TreeViewFind(m_treeView));
}

} // namespace Internal
} // namespace Debugger
