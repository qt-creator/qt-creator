// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androidpotentialkit.h"
#include "androidtr.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>

#include <projectexplorer/ipotentialkit.h>
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

using namespace ProjectExplorer;

namespace Android::Internal {

class AndroidPotentialKitWidget : public Utils::DetailsWidget
{
public:
    AndroidPotentialKitWidget(QWidget *parent);

private:
    void openOptions();
    void recheck();
};

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
    const QList<Kit *> kits = KitManager::kits();
    for (const Kit *kit : kits) {
        if (kit->isAutoDetected() && !kit->isSdkProvided()) {
            setVisible(false);
            return;
        }
    }
}

class AndroidPotentialKit final : public IPotentialKit
{
public:
    QString displayName() const final
    {
        return Tr::tr("Configure Android...");
    }

    void executeFromMenu() final
    {
        Core::ICore::showOptionsDialog(Constants::ANDROID_SETTINGS_ID);
    }

    QWidget *createWidget(QWidget *parent) const final
    {
        if (!isEnabled())
            return nullptr;
        return new AndroidPotentialKitWidget(parent);
    }

    bool isEnabled() const final
    {
        const QList<Kit *> kits = KitManager::kits();
        for (const Kit *kit : kits) {
            if (kit->isAutoDetected() && !kit->isSdkProvided()) {
                return false;
            }
        }

        return QtSupport::QtVersionManager::version([](const QtSupport::QtVersion *v) {
            return v->type() == QString::fromLatin1(Constants::ANDROID_QT_TYPE);
        });
    }
};

void setupAndroidPotentialKit()
{
    static AndroidPotentialKit theAndroidPotentialKit;
}

} // Android::Internal
