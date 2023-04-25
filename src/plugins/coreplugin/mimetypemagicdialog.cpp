// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mimetypemagicdialog.h"

#include "coreplugintr.h"
#include "icore.h"

#include <utils/headerviewstretcher.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

using namespace Utils;

namespace Core::Internal {

static MimeMagicRule::Type typeValue(int i)
{
    QTC_ASSERT(i < MimeMagicRule::Byte, return MimeMagicRule::Invalid);
    return MimeMagicRule::Type(i + 1/*0==invalid*/);
}

MimeTypeMagicDialog::MimeTypeMagicDialog(QWidget *parent) :
    QDialog(parent)
{
    resize(582, 419);
    setWindowTitle(Tr::tr("Add Magic Header"));

    auto informationLabel = new QLabel;
    informationLabel->setText(Tr::tr("<html><head/><body><p>MIME magic data is interpreted as defined "
         "by the Shared MIME-info Database specification from "
         "<a href=\"http://standards.freedesktop.org/shared-mime-info-spec/shared-mime-info-spec-latest.html\">"
         "freedesktop.org</a>.<hr/></p></body></html>"));  // FIXME: Simplify for translators
    informationLabel->setWordWrap(true);

    m_valueLineEdit = new QLineEdit;

    m_typeSelector = new QComboBox;
    m_typeSelector->addItem(Tr::tr("String"));
    m_typeSelector->addItem(Tr::tr("RegExp"));
    m_typeSelector->addItem(Tr::tr("Host16"));
    m_typeSelector->addItem(Tr::tr("Host32"));
    m_typeSelector->addItem(Tr::tr("Big16"));
    m_typeSelector->addItem(Tr::tr("Big32"));
    m_typeSelector->addItem(Tr::tr("Little16"));
    m_typeSelector->addItem(Tr::tr("Little32"));
    m_typeSelector->addItem(Tr::tr("Byte"));

    m_maskLineEdit = new QLineEdit;

    m_useRecommendedGroupBox = new QGroupBox(Tr::tr("Use Recommended"));
    m_useRecommendedGroupBox->setCheckable(true);

    m_noteLabel = new QLabel(Tr::tr("<html><head/><body><p><span style=\" font-style:italic;\">"
                            "Note: Wide range values might impact performance when opening "
                            "files.</span></p></body></html>"));
    m_noteLabel->setTextFormat(Qt::RichText);

    m_startRangeLabel = new QLabel(Tr::tr("Range start:"));

    m_endRangeLabel = new QLabel(Tr::tr("Range end:"));

    m_priorityLabel = new QLabel(Tr::tr("Priority:"));

    m_prioritySpinBox = new QSpinBox(m_useRecommendedGroupBox);
    m_prioritySpinBox->setMinimum(1);
    m_prioritySpinBox->setValue(50);

    m_startRangeSpinBox = new QSpinBox(m_useRecommendedGroupBox);
    m_startRangeSpinBox->setMaximum(9999);

    m_endRangeSpinBox = new QSpinBox(m_useRecommendedGroupBox);
    m_endRangeSpinBox->setMaximum(9999);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        Form {
            m_startRangeLabel, m_startRangeSpinBox, br,
            m_endRangeLabel, m_endRangeSpinBox, br,
            m_priorityLabel,  m_prioritySpinBox, br,
        },
        m_noteLabel
    }.attachTo(m_useRecommendedGroupBox);

    Column {
        informationLabel,
        Form {
            Tr::tr("Value:"), m_valueLineEdit, br,
            Tr::tr("Type:"), m_typeSelector, st, br,
            Tr::tr("Mask:"), m_maskLineEdit, br
        },
        m_useRecommendedGroupBox,
        st,
        buttonBox
    }.attachTo(this);

    connect(m_useRecommendedGroupBox, &QGroupBox::toggled,
            this, &MimeTypeMagicDialog::applyRecommended);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &MimeTypeMagicDialog::validateAccept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    connect(informationLabel, &QLabel::linkActivated, this, [](const QString &link) {
        QDesktopServices::openUrl(QUrl(link));
    });
    connect(m_typeSelector, &QComboBox::activated, this, [this] {
        if (m_useRecommendedGroupBox->isChecked())
            setToRecommendedValues();
    });
    applyRecommended(m_useRecommendedGroupBox->isChecked());
    m_valueLineEdit->setFocus();
}

void MimeTypeMagicDialog::setToRecommendedValues()
{
    m_startRangeSpinBox->setValue(0);
    m_endRangeSpinBox->setValue(m_typeSelector->currentIndex() == 1/*regexp*/ ? 200 : 0);
    m_prioritySpinBox->setValue(50);
}

void MimeTypeMagicDialog::applyRecommended(bool checked)
{
    if (checked) {
        // save previous custom values
        m_customRangeStart = m_startRangeSpinBox->value();
        m_customRangeEnd = m_endRangeSpinBox->value();
        m_customPriority = m_prioritySpinBox->value();
        setToRecommendedValues();
    } else {
        // restore previous custom values
        m_startRangeSpinBox->setValue(m_customRangeStart);
        m_endRangeSpinBox->setValue(m_customRangeEnd);
        m_prioritySpinBox->setValue(m_customPriority);
    }
    m_startRangeLabel->setEnabled(!checked);
    m_startRangeSpinBox->setEnabled(!checked);
    m_endRangeLabel->setEnabled(!checked);
    m_endRangeSpinBox->setEnabled(!checked);
    m_priorityLabel->setEnabled(!checked);
    m_prioritySpinBox->setEnabled(!checked);
    m_noteLabel->setEnabled(!checked);
}

void MimeTypeMagicDialog::validateAccept()
{
    QString errorMessage;
    MimeMagicRule rule = createRule(&errorMessage);
    if (rule.isValid())
        accept();
    else
        QMessageBox::critical(ICore::dialogParent(), Tr::tr("Error"), errorMessage);
}

void MimeTypeMagicDialog::setMagicData(const MagicData &data)
{
    m_valueLineEdit->setText(QString::fromUtf8(data.m_rule.value()));
    m_typeSelector->setCurrentIndex(data.m_rule.type() - 1/*0 == invalid*/);
    m_maskLineEdit->setText(QString::fromLatin1(MagicData::normalizedMask(data.m_rule)));
    m_useRecommendedGroupBox->setChecked(false); // resets values
    m_startRangeSpinBox->setValue(data.m_rule.startPos());
    m_endRangeSpinBox->setValue(data.m_rule.endPos());
    m_prioritySpinBox->setValue(data.m_priority);
}

MagicData MimeTypeMagicDialog::magicData() const
{
    MagicData data(createRule(), m_prioritySpinBox->value());
    return data;
}

bool MagicData::operator==(const MagicData &other) const
{
    return m_priority == other.m_priority && m_rule == other.m_rule;
}

/*!
    Returns the mask, or an empty string if the mask is the default mask which is set by
    MimeMagicRule when setting an empty mask for string patterns.
 */
QByteArray MagicData::normalizedMask(const MimeMagicRule &rule)
{
    // convert mask and see if it is the "default" one (which corresponds to "empty" mask)
    // see MimeMagicRule constructor
    QByteArray mask = rule.mask();
    if (rule.type() == MimeMagicRule::String) {
        QByteArray actualMask = QByteArray::fromHex(QByteArray::fromRawData(mask.constData() + 2,
                                                        mask.size() - 2));
        if (actualMask.count(char(-1)) == actualMask.size()) {
            // is the default-filled 0xfffffffff mask
            mask.clear();
        }
    }
    return mask;
}

MimeMagicRule MimeTypeMagicDialog::createRule(QString *errorMessage) const
{
    MimeMagicRule::Type type = typeValue(m_typeSelector->currentIndex());
    MimeMagicRule rule(type,
                       m_valueLineEdit->text().toUtf8(),
                       m_startRangeSpinBox->value(),
                       m_endRangeSpinBox->value(),
                       m_maskLineEdit->text().toLatin1(),
                       errorMessage);
    if (type == MimeMagicRule::Invalid) {
        if (errorMessage)
            *errorMessage = Tr::tr("Internal error: Type is invalid");
    }
    return rule;
}

} // Core::Internal
