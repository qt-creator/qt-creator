// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "updateinfosettings.h"

#include "updateinfoplugin.h"
#include "updateinfotr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>
#include <utils/progressindicator.h>

#include <QDate>
#include <QLabel>
#include <QLocale>
#include <QPointer>
#include <QPushButton>

using namespace Utils;

namespace UpdateInfo::Internal {

static QString localizedDate(const QDate &date)
{
    static QLocale locale(Core::ICore::userInterfaceLanguage());
    return locale.toString(date, locale.dateFormat());
}

class UpdateInfoSettings : public AspectContainer
{
public:
    explicit UpdateInfoSettings(UpdateInfoPlugin *plugin);

    void apply() final;

    UpdateInfoPlugin *m_plugin;

    BoolAspect automaticCheck{this};
    SelectionAspect checkInterval{this};
    BoolAspect checkForQtVersions{this};

    QPointer<ProgressIndicator> m_progressIndicator;
};

UpdateInfoSettings::UpdateInfoSettings(UpdateInfoPlugin *plugin)
    : m_plugin(plugin)
{
    setAutoApply(false);

    automaticCheck.setDefaultValue(true);

    checkInterval.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    checkInterval.setLabelText(Tr::tr("Check interval basis:"));
    checkInterval.addOption(Tr::tr("Daily"));
    checkInterval.addOption(Tr::tr("Weekly"));
    checkInterval.addOption(Tr::tr("Monthly"));

    checkForQtVersions.setLabelText(Tr::tr("Check for new Qt versions"));

    automaticCheck.setValue(m_plugin->isAutomaticCheck());
    checkInterval.setValue(int(m_plugin->checkUpdateInterval()));
    checkForQtVersions.setValue(m_plugin->isCheckingForQtVersions());

    setLayouter([this] {
        using namespace Layouting;

        auto infoLabel = new QLabel(
            Tr::tr("Automatically runs a scheduled check for updates on a time interval basis. "
                   "The automatic check for updates will be performed at the scheduled date, or "
                   "the next startup following it."));
        infoLabel->setWordWrap(true);

        auto nextCheckDateLabel = new QLabel;
        auto lastCheckDateLabel = new QLabel(Tr::tr("Not checked yet"));
        auto messageLabel = new QLabel;
        auto checkNowButton = new QPushButton(Tr::tr("Check Now"));

        const auto updateNextCheckDate = [this, nextCheckDateLabel] {
            const auto interval = UpdateInfoPlugin::CheckUpdateInterval(checkInterval.volatileValue());
            QDate date = m_plugin->nextCheckDate(interval);
            if (!date.isValid() || date < QDate::currentDate())
                date = QDate::currentDate();
            nextCheckDateLabel->setText(localizedDate(date));
        };
        const auto updateLastCheckDate = [this, lastCheckDateLabel, updateNextCheckDate] {
            const QDate date = m_plugin->lastCheckDate();
            lastCheckDateLabel->setText(date.isValid() ? localizedDate(date)
                                                       : Tr::tr("Not checked yet"));
            updateNextCheckDate();
        };

        // clang-format off
        QWidget *content = Column {
            Group {
                groupChecker(automaticCheck.groupChecker()),
                title(Tr::tr("Automatic Check for Updates")),
                Column {
                    infoLabel,
                    Row {
                        Form {
                            checkInterval, br,
                            Tr::tr("Next check date:"), nextCheckDateLabel,
                        },
                        st,
                    },
                    checkForQtVersions,
                },
            },
            Row {
                Tr::tr("Last check date:"), lastCheckDateLabel, st,
                messageLabel, st, checkNowButton,
            },
            st,
            noMargin,
        }.emerge();
        // clang-format on

        const auto checkRunningChanged = [this, content, messageLabel, checkNowButton](bool running) {
            checkNowButton->setDisabled(running);
            if (running) {
                if (!m_progressIndicator) {
                    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large);
                    m_progressIndicator->attachToWidget(content);
                }
                m_progressIndicator->show();
            } else {
                delete m_progressIndicator;
            }
            messageLabel->setText(running ? Tr::tr("Checking for updates...") : QString());
        };

        connect(&checkInterval, &BaseAspect::volatileValueChanged,
                nextCheckDateLabel, updateNextCheckDate);
        connect(m_plugin, &UpdateInfoPlugin::lastCheckDateChanged,
                lastCheckDateLabel, updateLastCheckDate);
        connect(m_plugin, &UpdateInfoPlugin::checkForUpdatesRunningChanged,
                content, checkRunningChanged);
        connect(m_plugin, &UpdateInfoPlugin::newUpdatesAvailable, messageLabel, [messageLabel](bool available) {
            messageLabel->setText(available ? Tr::tr("New updates are available.")
                                            : Tr::tr("No new updates are available."));
        });
        connect(checkNowButton, &QPushButton::clicked,
                m_plugin, &UpdateInfoPlugin::startCheckForUpdates);

        updateLastCheckDate();
        checkRunningChanged(m_plugin->isCheckForUpdatesRunning());

        return Column { content };
    });
}

void UpdateInfoSettings::apply()
{
    AspectContainer::apply();
    m_plugin->setCheckUpdateInterval(UpdateInfoPlugin::CheckUpdateInterval(checkInterval()));
    m_plugin->setAutomaticCheck(automaticCheck());
    m_plugin->setCheckingForQtVersions(checkForQtVersions());
}

// SettingsPage

static UpdateInfoSettings &settings(UpdateInfoPlugin *plugin = nullptr)
{
    static UpdateInfoSettings theSettings(plugin);
    return theSettings;
}

class SettingsPage : public Core::IOptionsPage
{
public:
    SettingsPage()
    {
        setId(FILTER_OPTIONS_PAGE_ID);
        setCategory(Core::Constants::SETTINGS_CATEGORY_CORE);
        setDisplayName(Tr::tr("Update"));
        setSettingsProvider([] { return &settings(); });
    }
};

void setupSettings(UpdateInfoPlugin *plugin)
{
    (void) settings(plugin);
    static SettingsPage theSettingsPage;
}

} // UpdateInfo::Internal
