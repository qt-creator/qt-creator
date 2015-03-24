/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "mimetypemagicdialog.h"

#include "icore.h"

#include <utils/headerviewstretcher.h>
#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>

using namespace Core;
using namespace Internal;

static Utils::Internal::MimeMagicRule::Type typeValue(int i)
{
    QTC_ASSERT(i < Utils::Internal::MimeMagicRule::Byte,
               return Utils::Internal::MimeMagicRule::Invalid);
    return Utils::Internal::MimeMagicRule::Type(i + 1/*0==invalid*/);
}

MimeTypeMagicDialog::MimeTypeMagicDialog(QWidget *parent) :
    QDialog(parent),
    m_customRangeStart(0),
    m_customRangeEnd(0),
    m_customPriority(50)
{
    ui.setupUi(this);
    setWindowTitle(tr("Add Magic Header"));
    connect(ui.useRecommendedGroupBox, &QGroupBox::toggled,
            this, &MimeTypeMagicDialog::applyRecommended);
    connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &MimeTypeMagicDialog::validateAccept);
    connect(ui.informationLabel, &QLabel::linkActivated, this, [](const QString &link) {
        QDesktopServices::openUrl(QUrl(link));
    });
    connect(ui.typeSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, [this]() {
        if (ui.useRecommendedGroupBox->isChecked())
            setToRecommendedValues();
    });
    ui.valueLineEdit->setFocus();
}

void MimeTypeMagicDialog::setToRecommendedValues()
{
    ui.startRangeSpinBox->setValue(0);
    ui.endRangeSpinBox->setValue(ui.typeSelector->currentIndex() == 1/*regexp*/ ? 200 : 0);
    ui.prioritySpinBox->setValue(50);
}

void MimeTypeMagicDialog::applyRecommended(bool checked)
{
    if (checked) {
        // save previous custom values
        m_customRangeStart = ui.startRangeSpinBox->value();
        m_customRangeEnd = ui.endRangeSpinBox->value();
        m_customPriority = ui.prioritySpinBox->value();
        setToRecommendedValues();
    } else {
        // restore previous custom values
        ui.startRangeSpinBox->setValue(m_customRangeStart);
        ui.endRangeSpinBox->setValue(m_customRangeEnd);
        ui.prioritySpinBox->setValue(m_customPriority);
    }
    ui.startRangeLabel->setEnabled(!checked);
    ui.startRangeSpinBox->setEnabled(!checked);
    ui.endRangeLabel->setEnabled(!checked);
    ui.endRangeSpinBox->setEnabled(!checked);
    ui.priorityLabel->setEnabled(!checked);
    ui.prioritySpinBox->setEnabled(!checked);
    ui.noteLabel->setEnabled(!checked);
}

void MimeTypeMagicDialog::validateAccept()
{
    QString errorMessage;
    Utils::Internal::MimeMagicRule rule = createRule(&errorMessage);
    if (rule.isValid())
        accept();
    else
        QMessageBox::critical(ICore::dialogParent(), tr("Error"), errorMessage);
}

void MimeTypeMagicDialog::setMagicData(const MagicData &data)
{
    ui.valueLineEdit->setText(QString::fromUtf8(data.m_rule.value()));
    ui.typeSelector->setCurrentIndex(data.m_rule.type() - 1/*0 == invalid*/);
    ui.maskLineEdit->setText(QString::fromLatin1(MagicData::normalizedMask(data.m_rule)));
    ui.useRecommendedGroupBox->setChecked(false); // resets values
    ui.startRangeSpinBox->setValue(data.m_rule.startPos());
    ui.endRangeSpinBox->setValue(data.m_rule.endPos());
    ui.prioritySpinBox->setValue(data.m_priority);
}

MagicData MimeTypeMagicDialog::magicData() const
{
    MagicData data(createRule(), ui.prioritySpinBox->value());
    return data;
}


bool MagicData::operator==(const MagicData &other)
{
    return m_priority == other.m_priority && m_rule == other.m_rule;
}

/*!
    Returns the mask, or an empty string if the mask is the default mask which is set by
    MimeMagicRule when setting an empty mask for string patterns.
 */
QByteArray MagicData::normalizedMask(const Utils::Internal::MimeMagicRule &rule)
{
    // convert mask and see if it is the "default" one (which corresponds to "empty" mask)
    // see MimeMagicRule constructor
    QByteArray mask = rule.mask();
    if (rule.type() == Utils::Internal::MimeMagicRule::String) {
        QByteArray actualMask = QByteArray::fromHex(QByteArray::fromRawData(mask.constData() + 2,
                                                        mask.size() - 2));
        if (actualMask.count(char(-1)) == actualMask.size()) {
            // is the default-filled 0xfffffffff mask
            mask.clear();
        }
    }
    return mask;
}

Utils::Internal::MimeMagicRule MimeTypeMagicDialog::createRule(QString *errorMessage) const
{
    Utils::Internal::MimeMagicRule::Type type = typeValue(ui.typeSelector->currentIndex());
    Utils::Internal::MimeMagicRule rule(type,
                                        ui.valueLineEdit->text().toUtf8(),
                                        ui.startRangeSpinBox->value(),
                                        ui.endRangeSpinBox->value(),
                                        ui.maskLineEdit->text().toLatin1(),
                                        errorMessage);
    if (type == Utils::Internal::MimeMagicRule::Invalid) {
        if (errorMessage)
            *errorMessage = tr("Internal error: Type is invalid");
    }
    return rule;
}
