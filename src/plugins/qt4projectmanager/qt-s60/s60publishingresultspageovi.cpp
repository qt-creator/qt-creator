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
#include "s60publishingresultspageovi.h"
#include "s60publisherovi.h"
#include "ui_s60publishingresultspageovi.h"

#include <QDesktopServices>
#include <QAbstractButton>
#include <QScrollBar>
#include <QProcess>

namespace Qt4ProjectManager {
namespace Internal {

S60PublishingResultsPageOvi::S60PublishingResultsPageOvi(S60PublisherOvi *publisher, QWidget *parent) :
    QWizardPage(parent),
    ui(new Ui::S60PublishingResultsPageOvi),
  m_publisher(publisher)
{
    ui->setupUi(this);
    connect(m_publisher, SIGNAL(progressReport(QString,QColor)), SLOT(updateResultsPage(QString,QColor)));
}

S60PublishingResultsPageOvi::~S60PublishingResultsPageOvi()
{
    delete ui;
}

void S60PublishingResultsPageOvi::initializePage()
{
    wizard()->setButtonText(QWizard::FinishButton, tr("Open Containing Folder"));
    connect(m_publisher, SIGNAL(finished()), SIGNAL(completeChanged()));
    connect(m_publisher, SIGNAL(finished()), SLOT(packageCreationFinished()));
    connect(wizard()->button(QWizard::FinishButton), SIGNAL(clicked()), SLOT(openFileLocation()));
    m_publisher->buildSis();
}

bool S60PublishingResultsPageOvi::isComplete() const
{
    return m_publisher->hasSucceeded();
}

void S60PublishingResultsPageOvi::packageCreationFinished()
{
    wizard()->setButtonText(QWizard::CancelButton, tr("Close"));
}

void S60PublishingResultsPageOvi::updateResultsPage(const QString& status, QColor c)
{
    const bool atBottom = isScrollbarAtBottom();
    QTextCursor cur(ui->resultsTextBrowser->document());
    QTextCharFormat tcf = cur.charFormat();
    tcf.setForeground(c);
    cur.movePosition(QTextCursor::End);
    cur.insertText(status, tcf);
    if (atBottom)
        scrollToBottom();
}

bool S60PublishingResultsPageOvi::isScrollbarAtBottom() const
{
    QScrollBar *scrollBar = ui->resultsTextBrowser->verticalScrollBar();
    return scrollBar->value() == scrollBar->maximum();
}

void S60PublishingResultsPageOvi::scrollToBottom()
{
    QScrollBar *scrollBar = ui->resultsTextBrowser->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
    // QPlainTextEdit destroys the first calls value in case of multiline
    // text, so make sure that the scroll bar actually gets the value set.
    // Is a noop if the first call succeeded.
    scrollBar->setValue(scrollBar->maximum());
}

void S60PublishingResultsPageOvi::openFileLocation()
{
#ifdef Q_OS_WIN
    QProcess::startDetached(QLatin1String("explorer /select,")+ m_publisher->createdSisFilePath());
#else
    QDesktopServices::openUrl(QUrl(QLatin1String("file:///") + m_publisher->createdSisFileContainingFolder()));
#endif
}

} // namespace Internal
} // namespace Qt4ProjectManager
