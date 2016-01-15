/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "androidpotentialkit.h"
#include "androidconstants.h"
#include "androidconfigurations.h"

#include <utils/detailswidget.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtversionmanager.h>
#include <qtsupport/baseqtversion.h>

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

using namespace Android;
using namespace Android::Internal;

QString AndroidPotentialKit::displayName() const
{
    return tr("Configure Android...");
}

void AndroidPotentialKit::executeFromMenu()
{
    Core::ICore::showOptionsDialog(Constants::ANDROID_SETTINGS_ID);
}

QWidget *AndroidPotentialKit::createWidget(QWidget *parent) const
{
    if (!isEnabled())
        return 0;
    return new AndroidPotentialKitWidget(parent);
}

bool AndroidPotentialKit::isEnabled() const
{
    QList<ProjectExplorer::Kit *> kits = ProjectExplorer::KitManager::kits();
    foreach (ProjectExplorer::Kit *kit, kits) {
        Core::Id deviceId = ProjectExplorer::DeviceKitInformation::deviceId(kit);
        if (kit->isAutoDetected()
                && deviceId == Core::Id(Constants::ANDROID_DEVICE_ID)
                && !kit->isSdkProvided()) {
            return false;
        }
    }

    bool found = false;
    foreach (QtSupport::BaseQtVersion *version, QtSupport::QtVersionManager::validVersions()) {
        if (version->type() == QLatin1String(Constants::ANDROIDQT)) {
            found = true;
            break;
        }
    }

    return found;
}

AndroidPotentialKitWidget::AndroidPotentialKitWidget(QWidget *parent)
    : Utils::DetailsWidget(parent)
{
    setSummaryText(QLatin1String("<b>Android has not been configured. Create Android kits.</b>"));
    setIcon(Core::Icons::WARNING.icon());
    //detailsWidget->setState(Utils::DetailsWidget::NoSummary);
    QWidget *mainWidget = new QWidget(this);
    setWidget(mainWidget);

    QGridLayout *layout = new QGridLayout(mainWidget);
    layout->setMargin(0);
    QLabel *label = new QLabel;
    label->setText(tr("Qt Creator needs additional settings to enable Android support."
                      " You can configure those settings in the Options dialog."));
    label->setWordWrap(true);
    layout->addWidget(label, 0, 0, 1, 2);

    QPushButton *openOptions = new QPushButton;
    openOptions->setText(Core::ICore::msgShowOptionsDialog());
    openOptions->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(openOptions, 1, 1);

    connect(openOptions, SIGNAL(clicked()),
            this, SLOT(openOptions()));

    connect(AndroidConfigurations::instance(), SIGNAL(updated()),
            this, SLOT(recheck()));
}

void AndroidPotentialKitWidget::openOptions()
{
    Core::ICore::showOptionsDialog(Constants::ANDROID_SETTINGS_ID, this);
}

void AndroidPotentialKitWidget::recheck()
{
    QList<ProjectExplorer::Kit *> kits = ProjectExplorer::KitManager::kits();
    foreach (ProjectExplorer::Kit *kit, kits) {
        Core::Id deviceId = ProjectExplorer::DeviceKitInformation::deviceId(kit);
        if (kit->isAutoDetected()
                && deviceId == Core::Id(Constants::ANDROID_DEVICE_ID)
                && !kit->isSdkProvided()) {
            setVisible(false);
            return;
        }
    }
}
