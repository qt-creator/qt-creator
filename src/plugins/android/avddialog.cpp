/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "avddialog.h"
#include "androidsdkmanager.h"

#include <utils/algorithm.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

#include <QKeyEvent>
#include <QMessageBox>
#include <QToolTip>

using namespace Android;
using namespace Android::Internal;

AvdDialog::AvdDialog(int minApiLevel, AndroidSdkManager *sdkManager, const QString &targetArch,
                     QWidget *parent) :
    QDialog(parent),
    m_sdkManager(sdkManager),
    m_minApiLevel(minApiLevel),
    m_allowedNameChars(QLatin1String("[a-z|A-Z|0-9|._-]*"))
{
    QTC_CHECK(m_sdkManager);
    m_avdDialog.setupUi(this);
    m_hideTipTimer.setInterval(2000);
    m_hideTipTimer.setSingleShot(true);

    if (targetArch.isEmpty()) {
        m_avdDialog.abiComboBox->addItems(QStringList({"armeabi-v7a", "armeabi", "x86", "mips",
                                                       "arm64-v8a", "x86_64", "mips64"}));
    } else {
        m_avdDialog.abiComboBox->addItems(QStringList(targetArch));
    }

    QRegExpValidator *v = new QRegExpValidator(m_allowedNameChars, this);
    m_avdDialog.nameLineEdit->setValidator(v);
    m_avdDialog.nameLineEdit->installEventFilter(this);

    m_avdDialog.warningIcon->setPixmap(Utils::Icons::WARNING.pixmap());

    updateApiLevelComboBox();

    connect(m_avdDialog.abiComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AvdDialog::updateApiLevelComboBox);

    connect(&m_hideTipTimer, &QTimer::timeout,
            this, [](){Utils::ToolTip::hide();});
}

bool AvdDialog::isValid() const
{
    return !name().isEmpty() && sdkPlatform() && sdkPlatform()->isValid() && !abi().isEmpty();
}

CreateAvdInfo AvdDialog::gatherCreateAVDInfo(QWidget *parent, AndroidSdkManager *sdkManager,
                                             int minApiLevel, QString targetArch)
{
    CreateAvdInfo result;
    AvdDialog d(minApiLevel, sdkManager, targetArch, parent);
    if (d.exec() != QDialog::Accepted || !d.isValid())
        return result;

    result.sdkPlatform = d.sdkPlatform();
    result.name = d.name();
    result.abi = d.abi();
    result.sdcardSize = d.sdcardSize();
    return result;
}

const SdkPlatform* AvdDialog::sdkPlatform() const
{
    return m_avdDialog.targetComboBox->currentData().value<SdkPlatform*>();
}

QString AvdDialog::name() const
{
    return m_avdDialog.nameLineEdit->text();
}

QString AvdDialog::abi() const
{
    return m_avdDialog.abiComboBox->currentText();
}

int AvdDialog::sdcardSize() const
{
    return m_avdDialog.sizeSpinBox->value();
}

void AvdDialog::updateApiLevelComboBox()
{
    SdkPlatformList filteredList;
    const SdkPlatformList platforms = m_sdkManager->filteredSdkPlatforms(m_minApiLevel);

    QString selectedAbi = abi();
    auto hasAbi = [selectedAbi](const SystemImage *image) {
        return image && image->isValid() && (image->abiName() == selectedAbi);
    };

    filteredList = Utils::filtered(platforms, [hasAbi](const SdkPlatform *platform) {
        return platform && Utils::anyOf(platform->systemImages(), hasAbi);
    });

    m_avdDialog.targetComboBox->clear();
    for (SdkPlatform *platform: filteredList) {
        m_avdDialog.targetComboBox->addItem(platform->displayText(),
                                            QVariant::fromValue<SdkPlatform *>(platform));
        m_avdDialog.targetComboBox->setItemData(m_avdDialog.targetComboBox->count() - 1,
                                                platform->descriptionText(), Qt::ToolTipRole);
    }

    if (platforms.isEmpty()) {
        m_avdDialog.warningIcon->setVisible(true);
        m_avdDialog.warningText->setVisible(true);
        m_avdDialog.warningText->setText(tr("Cannot create a new AVD. No sufficiently recent Android SDK available.\n"
                                            "Install an SDK of at least API version %1.")
                                         .arg(m_minApiLevel));
    } else if (filteredList.isEmpty()) {
        m_avdDialog.warningIcon->setVisible(true);
        m_avdDialog.warningText->setVisible(true);
        m_avdDialog.warningText->setText(tr("Cannot create a AVD for ABI %1. Install an image for it.")
                                         .arg(abi()));
    } else {
        m_avdDialog.warningIcon->setVisible(false);
        m_avdDialog.warningText->setVisible(false);
    }
}

bool AvdDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_avdDialog.nameLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        const QString key = ke->text();
        if (!key.isEmpty() && !m_allowedNameChars.exactMatch(key)) {
            QPoint position = m_avdDialog.nameLineEdit->parentWidget()->mapToGlobal(m_avdDialog.nameLineEdit->geometry().bottomLeft());
            position -= Utils::ToolTip::offsetFromPosition();
            Utils::ToolTip::show(position, tr("Allowed characters are: a-z A-Z 0-9 and . _ -"), m_avdDialog.nameLineEdit);
            m_hideTipTimer.start();
        } else {
            m_hideTipTimer.stop();
            Utils::ToolTip::hide();
        }
    }
    return QDialog::eventFilter(obj, event);
}
