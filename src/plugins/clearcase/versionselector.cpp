// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "versionselector.h"

#include "ui_versionselector.h"

#include <QRegularExpression>
#include <QTextStream>

namespace ClearCase {
namespace Internal {

VersionSelector::VersionSelector(const QString &fileName, const QString &message, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VersionSelector)
{
    ui->setupUi(this);
    ui->headerLabel->setText(ui->headerLabel->text().arg(fileName));
    ui->loadedText->setHtml("<html><head/><body><p><b>"
                            + tr("Note: You will not be able to check in this file without merging "
                                 "the changes (not supported by the plugin)")
                            + "</b></p></body></html>");
    m_stream = new QTextStream(message.toLocal8Bit(), QIODevice::ReadOnly | QIODevice::Text);
    QString line;
    while (!m_stream->atEnd() && !line.contains(QLatin1String("1) Loaded version")))
        line = m_stream->readLine();
    if (!readValues())
        return;
    ui->loadedLabel->setText(m_versionID);
    ui->loadedCreatedByLabel->setText(m_createdBy);
    ui->loadedCreatedOnLabel->setText(m_createdOn);
    ui->loadedText->insertPlainText(m_message + QLatin1Char(' '));

    line = m_stream->readLine(); // 2) Version after update
    if (!readValues())
        return;
    ui->updatedLabel->setText(m_versionID);
    ui->updatedCreatedByLabel->setText(m_createdBy);
    ui->updatedCreatedOnLabel->setText(m_createdOn);
    ui->updatedText->setPlainText(m_message);
}

VersionSelector::~VersionSelector()
{
    delete m_stream;
    delete ui;
}

bool VersionSelector::readValues()
{
    QString line;
    line = m_stream->readLine();
    const QRegularExpression id("Version ID: (.*)");
    const QRegularExpressionMatch idMatch = id.match(line);
    if (!idMatch.hasMatch())
        return false;
    m_versionID = idMatch.captured(1);

    line = m_stream->readLine();
    const QRegularExpression owner("Created by: (.*)");
    const QRegularExpressionMatch ownerMatch = owner.match(line);
    if (!ownerMatch.hasMatch())
        return false;
    m_createdBy = ownerMatch.captured(1);

    line = m_stream->readLine();
    const QRegularExpression dateTimeRE("Created on: (.*)");
    const QRegularExpressionMatch dateTimeMatch = dateTimeRE.match(line);
    if (!dateTimeMatch.hasMatch())
        return false;
    m_createdOn = dateTimeMatch.captured(1);

    QStringList messageLines;
    do
    {
        line = m_stream->readLine().trimmed();
        if (line.isEmpty())
            break;
        messageLines << line;
    } while (!m_stream->atEnd());
    m_message = messageLines.join(QLatin1Char(' '));
    return true;
}

bool VersionSelector::isUpdate() const
{
    return (ui->updatedRadioButton->isChecked());
}

} // namespace Internal
} // namespace ClearCase
