// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "versionselector.h"

#include "clearcasetr.h"

#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QRegularExpression>
#include <QTextEdit>
#include <QTextStream>

namespace ClearCase::Internal {

VersionSelector::VersionSelector(const QString &fileName, const QString &message, QWidget *parent) :
    QDialog(parent)
{
    resize(413, 435);
    setWindowTitle(Tr::tr("Confirm Version to Check Out"));

    auto headerLabel = new QLabel(Tr::tr("Multiple versions of \"%1\" can be checked out. "
                                     "Select the version to check out:").arg(fileName));
    headerLabel->setWordWrap(true);
    headerLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

    auto loadedRadioButton = new QRadioButton(Tr::tr("&Loaded version"));
    loadedRadioButton->setChecked(true);

    auto loadedLabel = new QLabel;
    auto loadedCreatedByLabel = new QLabel;
    auto loadedCreatedOnLabel = new QLabel;
    auto updatedLabel = new QLabel;
    auto updatedCreatedByLabel = new QLabel;
    auto updatedCreatedOnLabel = new QLabel;

    auto updatedText = new QPlainTextEdit;
    updatedText->setReadOnly(true);

    m_updatedRadioButton = new QRadioButton(Tr::tr("Version after &update"));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    auto loadedText = new QTextEdit;
    loadedText->setHtml("<html><head/><body><p><b>"
                            + Tr::tr("Note: You will not be able to check in this file without merging "
                                 "the changes (not supported by the plugin)")
                            + "</b></p></body></html>");

    using namespace Layouting;

    Column {
        headerLabel,
        Form {
            loadedRadioButton, loadedLabel, br,
            Tr::tr("Created by:"), loadedCreatedByLabel, br,
            Tr::tr("Created on:"),  loadedCreatedOnLabel, br,
            Span(2, loadedText),
        },
        Form {
            m_updatedRadioButton, updatedLabel, br,
            Tr::tr("Created by:"), updatedCreatedByLabel, br,
            Tr::tr("Created on:"),  updatedCreatedOnLabel, br,
            Span(2, updatedText)
        },
        buttonBox,
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_stream = new QTextStream(message.toLocal8Bit(), QIODevice::ReadOnly | QIODevice::Text);
    QString line;
    while (!m_stream->atEnd() && !line.contains(QLatin1String("1) Loaded version")))
        line = m_stream->readLine();
    if (!readValues())
        return;
    loadedLabel->setText(m_versionID);
    loadedCreatedByLabel->setText(m_createdBy);
    loadedCreatedOnLabel->setText(m_createdOn);
    loadedText->insertPlainText(m_message + QLatin1Char(' '));

    line = m_stream->readLine(); // 2) Version after update
    if (!readValues())
        return;
    updatedLabel->setText(m_versionID);
    updatedCreatedByLabel->setText(m_createdBy);
    updatedCreatedOnLabel->setText(m_createdOn);
    updatedText->setPlainText(m_message);
}

VersionSelector::~VersionSelector()
{
    delete m_stream;
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
    return m_updatedRadioButton->isChecked();
}

} // ClearCase::Internal
