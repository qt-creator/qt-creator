// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidpotentialkit.h"
#include "androidtr.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>

#include <qtsupport/qtversionmanager.h>
#include <qtsupport/baseqtversion.h>

#include <utils/detailswidget.h>
#include <utils/utilsicons.h>

#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QPushButton>

namespace Android::Internal {

class AndroidPotentialKitWidget : public Utils::DetailsWidget
{
public:
    AndroidPotentialKitWidget(QWidget *parent);

private:
    void openOptions();
    void recheck();
};

QString AndroidPotentialKit::displayName() const
{
    return Tr::tr("Configure Android...");
}

void AndroidPotentialKit::executeFromMenu()
{
    Core::ICore::showOptionsDialog(Constants::ANDROID_SETTINGS_ID);
}

QWidget *AndroidPotentialKit::createWidget(QWidget *parent) const
{
    if (!isEnabled())
        return nullptr;
    return new AndroidPotentialKitWidget(parent);
}

bool AndroidPotentialKit::isEnabled() const
{
    const QList<ProjectExplorer::Kit *> kits = ProjectExplorer::KitManager::kits();
    for (const ProjectExplorer::Kit *kit : kits) {
        if (kit->isAutoDetected() && !kit->isSdkProvided()) {
            return false;
        }
    }

    return QtSupport::QtVersionManager::version([](const QtSupport::QtVersion *v) {
        return v->type() == QString::fromLatin1(Constants::ANDROID_QT_TYPE);
    });
}

AndroidPotentialKitWidget::AndroidPotentialKitWidget(QWidget *parent)
    : Utils::DetailsWidget(parent)
{
    setSummaryText(QLatin1String("<b>Android has not been configured. Create Android kits.</b>"));
    setIcon(Utils::Icons::WARNING.icon());
    //detailsWidget->setState(Utils::DetailsWidget::NoSummary);
    auto mainWidget = new QWidget(this);
    setWidget(mainWidget);

    auto layout = new QGridLayout(mainWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    auto label = new QLabel;
    label->setText(Tr::tr("%1 needs additional settings to enable Android support."
                          " You can configure those settings in the Options dialog.")
                       .arg(QGuiApplication::applicationDisplayName()));
    label->setWordWrap(true);
    layout->addWidget(label, 0, 0, 1, 2);

    auto openOptions = new QPushButton;
    openOptions->setText(Core::ICore::msgShowOptionsDialog());
    openOptions->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(openOptions, 1, 1);

    connect(openOptions, &QAbstractButton::clicked,
            this, &AndroidPotentialKitWidget::openOptions);

    connect(AndroidConfigurations::instance(), &AndroidConfigurations::updated,
            this, &AndroidPotentialKitWidget::recheck);
}

void AndroidPotentialKitWidget::openOptions()
{
    Core::ICore::showOptionsDialog(Constants::ANDROID_SETTINGS_ID, this);
}

void AndroidPotentialKitWidget::recheck()
{
    const QList<ProjectExplorer::Kit *> kits = ProjectExplorer::KitManager::kits();
    for (const ProjectExplorer::Kit *kit : kits) {
        if (kit->isAutoDetected() && !kit->isSdkProvided()) {
            setVisible(false);
            return;
        }
    }
}

} // Android::Internal
