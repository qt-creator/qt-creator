/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "blackberrypotentialkit.h"

#include "blackberryconfigurationmanager.h"
#include "blackberryapilevelconfiguration.h"
#include "qnxconstants.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QPushButton>

using namespace Qnx::Internal;

QString BlackBerryPotentialKit::displayName() const
{
    return tr("Configure BlackBerry...");
}

void BlackBerryPotentialKit::executeFromMenu()
{
    openSettings();
}

QWidget *BlackBerryPotentialKit::createWidget(QWidget *parent) const
{
    return shouldShow() ? new BlackBerryPotentialKitWidget(parent) : 0;
}

bool BlackBerryPotentialKit::isEnabled() const
{
    return shouldShow();
}

bool BlackBerryPotentialKit::shouldShow()
{
    QList<BlackBerryApiLevelConfiguration *> configs =
            BlackBerryConfigurationManager::instance()->apiLevels();
    if (configs.isEmpty())
        return false; // do not display when we do not have any BlackBerry API Level registered
    foreach (BlackBerryApiLevelConfiguration *config, configs) {
        if (config->isValid() && config->isActive())
            return false; // do not display when there is at least one valid and active API Level
    }
    return true;
}

void BlackBerryPotentialKit::openSettings()
{
    Core::ICore::showOptionsDialog(Qnx::Constants::QNX_BB_CATEGORY,
                                   Qnx::Constants::QNX_BB_SETUP_ID);
}

BlackBerryPotentialKitWidget::BlackBerryPotentialKitWidget(QWidget *parent)
    : Utils::DetailsWidget(parent)
{
    setSummaryText(tr("<b>BlackBerry has not been configured. Create BlackBerry kits.</b>"));
    setIcon(QIcon(QLatin1String(ProjectExplorer::Constants::ICON_WARNING)));
    QWidget *mainWidget = new QWidget(this);
    setWidget(mainWidget);

    QGridLayout *layout = new QGridLayout(mainWidget);
    layout->setMargin(0);
    QLabel *label = new QLabel;
    label->setText(tr("Qt Creator needs additional settings to enable BlackBerry support."
                      " You can configure those settings in the Options dialog."));
    label->setWordWrap(true);
    layout->addWidget(label, 0, 0, 1, 2);

    QPushButton *openOptions = new QPushButton;
    openOptions->setText(Core::ICore::msgShowOptionsDialog());
    openOptions->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(openOptions, 1, 1);

    connect(openOptions, SIGNAL(clicked()), this, SLOT(openOptions()));
    connect(BlackBerryConfigurationManager::instance(), SIGNAL(settingsChanged()),
            this, SLOT(recheck()));
}

void BlackBerryPotentialKitWidget::openOptions()
{
    BlackBerryPotentialKit::openSettings();
}

void BlackBerryPotentialKitWidget::recheck()
{
    setVisible(BlackBerryPotentialKit::shouldShow());
}
