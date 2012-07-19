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

#include "s60deployconfigurationwidget.h"
#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"
#include "symbianidevice.h"

#include <symbianutils/symbiandevicemanager.h>
#include <codadevice.h>

#include <coreplugin/helpmanager.h>

#include "codaruncontrol.h"

#include <utils/detailswidget.h>
#include <utils/ipaddresslineedit.h>
#include <utils/qtcassert.h>
#include <utils/pathchooser.h>

#include <QDir>
#include <QTimer>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QToolButton>
#include <QStyle>
#include <QApplication>
#include <QSpacerItem>
#include <QMessageBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QValidator>

#include <QTcpSocket>

Q_DECLARE_METATYPE(SymbianUtils::SymbianDevice)

static const char STARTING_DRIVE_LETTER = 'C';
static const char LAST_DRIVE_LETTER = 'Z';

static QString formatDriveText(const Qt4ProjectManager::S60DeployConfiguration::DeviceDrive &drive)
{
    const QChar driveLetter = QChar(QLatin1Char(drive.first)).toUpper();
    if (drive.second <= 0)
        return driveLetter + QLatin1Char(':');
    if (drive.second >= 1024)
        return QString::fromLatin1("%1:%2 MB").arg(driveLetter).arg(drive.second);
    return QString::fromLatin1("%1:%2 kB").arg(driveLetter).arg(drive.second);
}

namespace Qt4ProjectManager {
namespace Internal {


S60DeployConfigurationWidget::S60DeployConfigurationWidget(QWidget *parent)
    : ProjectExplorer::DeployConfigurationWidget(parent),
      m_detailsWidget(new Utils::DetailsWidget),
      m_sisFileLabel(new QLabel),
      m_deviceInfoButton(new QToolButton),
      m_installationDriveCombo(new QComboBox()),
      m_silentInstallCheckBox(new QCheckBox(tr("Silent installation")))
{
}

S60DeployConfigurationWidget::~S60DeployConfigurationWidget()
{
}

void S60DeployConfigurationWidget::init(ProjectExplorer::DeployConfiguration *dc)
{
    m_deployConfiguration = qobject_cast<S60DeployConfiguration *>(dc);

    m_detailsWidget->setState(Utils::DetailsWidget::NoSummary);

    QVBoxLayout *mainBoxLayout = new QVBoxLayout();
    mainBoxLayout->setMargin(0);
    setLayout(mainBoxLayout);
    mainBoxLayout->addWidget(m_detailsWidget);
    QWidget *detailsContainer = new QWidget;
    m_detailsWidget->setWidget(detailsContainer);

    QVBoxLayout *detailsBoxLayout = new QVBoxLayout();
    detailsBoxLayout->setMargin(0);
    detailsContainer->setLayout(detailsBoxLayout);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setMargin(0);
    detailsBoxLayout->addLayout(formLayout);
    formLayout->addRow(tr("Installation file:"), m_sisFileLabel);

    // Installation Drive control
    updateInstallationDrives();

    QHBoxLayout *installationBoxLayout = new QHBoxLayout();
    m_installationDriveCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_installationDriveCombo, SIGNAL(activated(int)), this, SLOT(setInstallationDrive(int)));
    QHBoxLayout *installationDriveHBoxLayout = new QHBoxLayout;
    installationDriveHBoxLayout->addWidget(m_installationDriveCombo);
    installationBoxLayout->addLayout(installationDriveHBoxLayout);

    // Non-silent installs are a fallback if one wants to override missing dependencies.
    m_silentInstallCheckBox->setChecked(m_deployConfiguration->silentInstall());
    m_silentInstallCheckBox->setToolTip(tr("Silent installation is an installation mode "
                                           "that does not require user's intervention. "
                                           "In case it fails the non silent installation is launched."));
    connect(m_silentInstallCheckBox, SIGNAL(stateChanged(int)), this, SLOT(silentInstallChanged(int)));
    installationBoxLayout->addWidget(m_silentInstallCheckBox);
    installationBoxLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
    formLayout->addRow(tr("Installation drive:"), installationBoxLayout);

    updateTargetInformation();
    connect(m_deployConfiguration, SIGNAL(targetInformationChanged()),
            this, SLOT(updateTargetInformation()));
    connect(m_deployConfiguration, SIGNAL(availableDeviceDrivesChanged()),
            this, SLOT(updateInstallationDrives()));
}

void S60DeployConfigurationWidget::updateInstallationDrives()
{
    m_installationDriveCombo->clear();
    const QList<S60DeployConfiguration::DeviceDrive> &availableDrives(m_deployConfiguration->availableDeviceDrives());
    int index = 0;
    char currentDrive = QChar::toUpper(static_cast<ushort>(m_deployConfiguration->installationDrive()));
    if (availableDrives.isEmpty()) {
        for (int i = STARTING_DRIVE_LETTER; i <= LAST_DRIVE_LETTER; ++i) {
            const QChar qc = QLatin1Char(static_cast<char>(i));
            m_installationDriveCombo->addItem(qc + QLatin1Char(':'), QVariant(qc));
        }
        index = currentDrive - STARTING_DRIVE_LETTER;
    } else {
        for (int i = 0; i < availableDrives.count(); ++i) {
            const S60DeployConfiguration::DeviceDrive& drive(availableDrives.at(i));
            char driveLetter = QChar::toUpper(static_cast<ushort>(drive.first));
            m_installationDriveCombo->addItem(formatDriveText(drive),
                                              QVariant(QChar(QLatin1Char(driveLetter))));
            if (currentDrive == driveLetter)
                index = i;
        }
    }
    QTC_ASSERT(index >= 0 && index <= m_installationDriveCombo->count(), return);

    m_installationDriveCombo->setCurrentIndex(index);
    setInstallationDrive(index);
}

void S60DeployConfigurationWidget::silentInstallChanged(int state)
{
    m_deployConfiguration->setSilentInstall(state == Qt::Checked);
}

void S60DeployConfigurationWidget::updateTargetInformation()
{
    QString package;
    for (int i = 0; i < m_deployConfiguration->signedPackages().count(); ++i)
        package += m_deployConfiguration->signedPackages()[i] + QLatin1String("\n");
    if (!package.isEmpty())
        package.remove(package.length()-1, 1);
    m_sisFileLabel->setText(QDir::toNativeSeparators(package));
}

void S60DeployConfigurationWidget::setInstallationDrive(int index)
{
    QTC_ASSERT(index >= 0, return);
    QTC_ASSERT(index < m_installationDriveCombo->count(), return);

    QChar driveLetter(m_installationDriveCombo->itemData(index).toChar());
    m_deployConfiguration->setInstallationDrive(driveLetter.toAscii());
}

} // namespace Internal
} // namespace Qt4ProjectManager
