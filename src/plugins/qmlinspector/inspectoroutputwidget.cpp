/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#include "inspectoroutputwidget.h"
#include <coreplugin/coreconstants.h>

#include <QtGui/QTextEdit>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QIcon>

using namespace Qml;

InspectorOutputWidget::InspectorOutputWidget(QWidget *parent)
    : QTextEdit(parent)
{
    setWindowTitle(tr("Output"));

    m_clearContents = new QAction(QString(tr("Clear")), this);
    m_clearContents->setIcon(QIcon(Core::Constants::ICON_CLEAR));
    connect(m_clearContents, SIGNAL(triggered()), SLOT(clear()));
}

InspectorOutputWidget::~InspectorOutputWidget()
{

}

void InspectorOutputWidget::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu(e->globalPos());

    menu->addSeparator();
    menu->addAction(m_clearContents);
    menu->exec(e->globalPos());
}

void InspectorOutputWidget::addOutput(RunControl *, const QString &text)
{
    insertPlainText(text);
    moveCursor(QTextCursor::End);
}

void InspectorOutputWidget::addOutputInline(RunControl *, const QString &text)
{
    insertPlainText(text);
    moveCursor(QTextCursor::End);
}

void InspectorOutputWidget::addErrorOutput(RunControl *, const QString &text)
{
    append(text);
    moveCursor(QTextCursor::End);
}

void InspectorOutputWidget::addInspectorStatus(const QString &text)
{
    setTextColor(Qt::darkGreen);
    append(text);
    moveCursor(QTextCursor::End);
    setTextColor(Qt::black);
}

