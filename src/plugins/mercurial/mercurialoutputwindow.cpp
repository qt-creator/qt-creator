/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#include "mercurialoutputwindow.h"

#include <QtGui/QListWidget>
#include <QtCore/QDebug>
#include <QtCore/QTextCodec>

using namespace Mercurial::Internal;

MercurialOutputWindow::MercurialOutputWindow()
{
    outputListWidgets = new QListWidget;
    outputListWidgets->setWindowTitle(tr("Mercurial Output"));
    outputListWidgets->setFrameStyle(QFrame::NoFrame);
    outputListWidgets->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

MercurialOutputWindow::~MercurialOutputWindow()
{
    delete outputListWidgets;
    outputListWidgets = 0;
}

QWidget *MercurialOutputWindow::outputWidget(QWidget *parent)
{
    outputListWidgets->setParent(parent);
    return outputListWidgets;
}

QList<QWidget*> MercurialOutputWindow::toolBarWidgets() const
{
    return QList<QWidget *>();
}

QString MercurialOutputWindow::name() const
{
    return tr("Mercurial");
}

int MercurialOutputWindow::priorityInStatusBar() const
{
    return -1;
}

void MercurialOutputWindow::clearContents()
{
    outputListWidgets->clear();
}

void MercurialOutputWindow::visibilityChanged(bool visible)
{
    if (visible)
        outputListWidgets->setFocus();
}

void MercurialOutputWindow::setFocus()
{
}

bool MercurialOutputWindow::hasFocus()
{
    return outputListWidgets->hasFocus();
}

bool MercurialOutputWindow::canFocus()
{
    return false;
}

bool MercurialOutputWindow::canNavigate()
{
    return false;
}

bool MercurialOutputWindow::canNext()
{
    return false;
}

bool MercurialOutputWindow::canPrevious()
{
    return false;
}

void MercurialOutputWindow::goToNext()
{
}

void MercurialOutputWindow::goToPrev()
{
}

void MercurialOutputWindow::append(const QString &text)
{
    outputListWidgets->addItems(text.split(QLatin1Char('\n')));
    outputListWidgets->scrollToBottom();
    popup(true);
}

void MercurialOutputWindow::append(const QByteArray &array)
{
    append(QTextCodec::codecForLocale()->toUnicode(array));
}
