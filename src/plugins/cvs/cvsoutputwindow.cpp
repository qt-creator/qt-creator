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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "cvsoutputwindow.h"
#include "cvsplugin.h"

#include <QtGui/QListWidget>
#include <QtCore/QDebug>

using namespace CVS::Internal;

CVSOutputWindow::CVSOutputWindow(CVSPlugin *cvsPlugin)
    : m_cvsPlugin(cvsPlugin)
{
    m_outputListWidget = new QListWidget;
    m_outputListWidget->setFrameStyle(QFrame::NoFrame);
    m_outputListWidget->setWindowTitle(tr("CVS Output"));
    m_outputListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

CVSOutputWindow::~CVSOutputWindow()
{
    delete m_outputListWidget;
}

QWidget *CVSOutputWindow::outputWidget(QWidget *parent)
{
    m_outputListWidget->setParent(parent);
    return m_outputListWidget;
}

QString CVSOutputWindow::name() const
{
    return tr("CVS");
}

void CVSOutputWindow::clearContents()
{
    m_outputListWidget->clear();
}

int CVSOutputWindow::priorityInStatusBar() const
{
    return -1;
}

void CVSOutputWindow::visibilityChanged(bool b)
{
    if (b)
        m_outputListWidget->setFocus();
}

void CVSOutputWindow::append(const QString &txt, bool doPopup)
{
    const QStringList lines = txt.split(QLatin1Char('\n'));
    foreach (const QString &s, lines)
        m_outputListWidget->addItem(s);
    m_outputListWidget->scrollToBottom();

    if (doPopup)
        popup();
}

bool CVSOutputWindow::canFocus()
{
    return false;
}

bool CVSOutputWindow::hasFocus()
{
    return m_outputListWidget->hasFocus();
}

void CVSOutputWindow::setFocus()
{
}

bool CVSOutputWindow::canNext()
{
    return false;
}

bool CVSOutputWindow::canPrevious()
{
    return false;
}

void CVSOutputWindow::goToNext()
{

}

void CVSOutputWindow::goToPrev()
{

}

bool CVSOutputWindow::canNavigate()
{
    return false;
}
