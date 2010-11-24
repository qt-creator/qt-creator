/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "maemopublishingresultpagefremantlefree.h"
#include "ui_maemopublishingresultpagefremantlefree.h"

#include <QtGui/QAbstractButton>

namespace Qt4ProjectManager {
namespace Internal {
typedef MaemoPublisherFremantleFree MPFF;

MaemoPublishingResultPageFremantleFree::MaemoPublishingResultPageFremantleFree(MPFF *publisher,
    QWidget *parent) : QWizardPage(parent), m_publisher(publisher),
    ui(new Ui::MaemoPublishingResultPageFremantleFree)
{
    m_lastOutputType = MPFF::StatusOutput;
    ui->setupUi(this);
}

MaemoPublishingResultPageFremantleFree::~MaemoPublishingResultPageFremantleFree()
{
    delete ui;
}

void MaemoPublishingResultPageFremantleFree::initializePage()
{
    cancelButton()->disconnect();
    connect(cancelButton(), SIGNAL(clicked()), SLOT(handleCancelRequest()));
    connect(m_publisher, SIGNAL(finished()), SLOT(handleFinished()));
    connect(m_publisher,
        SIGNAL(progressReport(QString, MaemoPublisherFremantleFree::OutputType)),
        SLOT(handleProgress(QString, MaemoPublisherFremantleFree::OutputType)));
    m_publisher->publish();
}

void MaemoPublishingResultPageFremantleFree::handleFinished()
{
    handleProgress(m_publisher->resultString(), MPFF::StatusOutput);
    m_isComplete = true;
    cancelButton()->setEnabled(false);
    emit completeChanged();
}

void MaemoPublishingResultPageFremantleFree::handleProgress(const QString &text,
    MPFF::OutputType type)
{
    const QString color = QLatin1String(type == MPFF::StatusOutput
        || type == MPFF::ToolStatusOutput ? "blue" : "red");
    ui->progressTextEdit->setTextColor(QColor(color));
    const bool bold = type == MPFF::StatusOutput
            || type == MPFF::ErrorOutput ? true : false;
    QFont font = ui->progressTextEdit->currentFont();
    font.setBold(bold);
    ui->progressTextEdit->setCurrentFont(font);

    if (type == MPFF::StatusOutput || type == MPFF::ErrorOutput
            || m_lastOutputType == MPFF::StatusOutput
            || m_lastOutputType == MPFF::ErrorOutput) {
        ui->progressTextEdit->append(text);
    } else {
        ui->progressTextEdit->insertPlainText(text);
    }
    ui->progressTextEdit->moveCursor(QTextCursor::End);
    m_lastOutputType = type;
}

void MaemoPublishingResultPageFremantleFree::handleCancelRequest()
{
    cancelButton()->setEnabled(false);
    m_publisher->cancel();
}

QAbstractButton *MaemoPublishingResultPageFremantleFree::cancelButton() const
{
    return wizard()->button(QWizard::CancelButton);
}

} // namespace Internal
} // namespace Qt4ProjectManager
