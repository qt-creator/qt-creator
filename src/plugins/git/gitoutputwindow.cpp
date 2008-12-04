/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "gitoutputwindow.h"

#include <QtCore/QTextCodec>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QListWidget>

using namespace Git::Internal;

GitOutputWindow::GitOutputWindow()
    : Core::IOutputPane()
{
    m_outputListWidget = new QListWidget;
    m_outputListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_outputListWidget->setFrameStyle(QFrame::NoFrame);

    m_outputListWidget->setWindowTitle(tr("Git Output"));
}

GitOutputWindow::~GitOutputWindow()
{
    delete m_outputListWidget;
}

QWidget *GitOutputWindow::outputWidget(QWidget *parent)
{
    m_outputListWidget->setParent(parent);
    return m_outputListWidget;
}

QString GitOutputWindow::name() const
{
    return tr("Git");
}

void GitOutputWindow::clearContents()
{
    m_outputListWidget->clear();
}

void GitOutputWindow::visibilityChanged(bool b)
{
    if (b)
        m_outputListWidget->setFocus();
}

bool GitOutputWindow::hasFocus()
{
    return m_outputListWidget->hasFocus();
}

bool GitOutputWindow::canFocus()
{
    return false;
}

void GitOutputWindow::setFocus()
{
}

void GitOutputWindow::setText(const QString &text)
{
    clearContents();
    append(text);
}

void GitOutputWindow::append(const QString &text)
{
    const QStringList lines = text.split(QLatin1Char('\n'));
    foreach (const QString &s, lines)
        m_outputListWidget->addItem(s);
    m_outputListWidget->scrollToBottom();
    popup();
}

void GitOutputWindow::setData(const QByteArray &data)
{
    setText(QTextCodec::codecForLocale()->toUnicode(data));
}

void GitOutputWindow::appendData(const QByteArray &data)
{
    append(QTextCodec::codecForLocale()->toUnicode(data));
}

int GitOutputWindow::priorityInStatusBar() const
{
    return -1;
}
