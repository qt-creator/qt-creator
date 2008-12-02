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
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QListWidget>

#include "perforceoutputwindow.h"
#include "perforceplugin.h"

using namespace Perforce::Internal;

PerforceOutputWindow::PerforceOutputWindow(PerforcePlugin *p4Plugin)
    : m_p4Plugin(p4Plugin)
{
    m_outputListWidget = new QListWidget;
    m_outputListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_outputListWidget->setFrameStyle(QFrame::NoFrame);

    m_outputListWidget->setWindowTitle(tr("Perforce Output"));

    m_diffAction = new QAction(tr("Diff"), this);
    connect(m_diffAction, SIGNAL(triggered()), this, SLOT(diff()));

    connect(m_outputListWidget, SIGNAL(itemActivated(QListWidgetItem*)),
        this, SLOT(openFiles()));
}

PerforceOutputWindow::~PerforceOutputWindow()
{
    delete m_outputListWidget;
}

bool PerforceOutputWindow::hasFocus()
{
    return m_outputListWidget->hasFocus();
}

bool PerforceOutputWindow::canFocus()
{
    return false;
}

void PerforceOutputWindow::setFocus()
{

}

QWidget *PerforceOutputWindow::outputWidget(QWidget *parent)
{
    m_outputListWidget->setParent(parent);
    return m_outputListWidget;
}

QString PerforceOutputWindow::name() const
{
    return tr("Perforce");
}

void PerforceOutputWindow::clearContents()
{
    m_outputListWidget->clear();
}

void PerforceOutputWindow::visibilityChanged(bool /* b */)
{

}

void PerforceOutputWindow::append(const QString &txt, bool doPopup)
{
    const QStringList lines = txt.split(QLatin1Char('\n'));
    foreach (const QString &s, lines)
        m_outputListWidget->addItem(s);
    m_outputListWidget->scrollToBottom();
    if (doPopup)
        popup();
}

void PerforceOutputWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu(m_outputListWidget);
    menu->addAction(m_diffAction);
    menu->exec(event->globalPos());
    delete menu;
}

void PerforceOutputWindow::diff()
{
    QStringList files;
    foreach (QListWidgetItem *i, m_outputListWidget->selectedItems()) {
        if (m_outputListWidget->row(i) > 0)
            files.append(getFileName(i));
    }
    if (files.count() == 0 && m_outputListWidget->row(m_outputListWidget->currentItem()) > 0)
        files.append(getFileName(m_outputListWidget->currentItem()));

    m_p4Plugin->p4Diff(files);
}

QString PerforceOutputWindow::getFileName(const QListWidgetItem *item)
{
    QString fileName;
    if (!item || item->text().isEmpty())
        return fileName;

    QString line = item->text();
    QRegExp regExp("(/.+)#\\d+\\s-\\s(.+)$");
    regExp.setMinimal(true);
    if (regExp.indexIn(line) > -1 && regExp.numCaptures() >= 1) {
        fileName = regExp.cap(1);
        QString description;
        if (regExp.numCaptures() >= 2)
            description = regExp.cap(2);
    }
    return fileName;
}

void PerforceOutputWindow::openFiles()
{
    QStringList files;
    foreach (QListWidgetItem *i, m_outputListWidget->selectedItems()) {
        if (m_outputListWidget->row(i) > 0)
            files.append(getFileName(i));
    }
    if (files.count() == 0 && m_outputListWidget->row(m_outputListWidget->currentItem()) > 0)
        files.append(getFileName(m_outputListWidget->currentItem()));

    m_p4Plugin->openFiles(files);
}

int PerforceOutputWindow::priorityInStatusBar() const
{
    return -1;
}
