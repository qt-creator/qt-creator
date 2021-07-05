/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "dockerconstants.h"
#include "dockersettings.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>
#include <utils/qtcsettings.h>

using namespace Utils;

namespace Docker {
namespace Internal {

// DockerSettings

const char SETTINGS_KEY[] = "Docker";

static DockerSettings *theSettings = nullptr;

DockerSettings::DockerSettings()
{
    theSettings = this;
    setAutoApply(false);
    readSettings(Core::ICore::settings());

    imageListFilter.setSettingsKey("DockerListFilter");
    imageListFilter.setPlaceHolderText(tr("<filter>"));
    imageListFilter.setDisplayStyle(StringAspect::LineEditDisplay);
    imageListFilter.setLabelText(tr("Filter:"));

    imageList.setDisplayStyle(StringAspect::TextEditDisplay);
    imageList.setLabelText(tr("Images:"));

    connect(&imageListFilter, &BaseAspect::changed, this, &DockerSettings::updateImageList);
}

DockerSettings *DockerSettings::instance()
{
    return theSettings;
}

void DockerSettings::writeSettings(QSettings *settings) const
{
    settings->remove(SETTINGS_KEY);
    settings->beginGroup(SETTINGS_KEY);
    forEachAspect([settings](BaseAspect *aspect) {
        QtcSettings::setValueWithDefault(settings, aspect->settingsKey(),
                                         aspect->value(), aspect->defaultValue());
    });
    settings->endGroup();
}

void DockerSettings::updateImageList()
{
    QtcProcess process;
    process.setCommand({"docker", {"search", imageListFilter.value()}});

    connect(&process, &QtcProcess::finished, this, [&process, this] {
        const QString data = QString::fromUtf8(process.readAllStandardOutput());
        imageList.setValue(data);
    });

    process.start();
    process.waitForFinished();
}

void DockerSettings::readSettings(const QSettings *settings)
{
    const QString keyRoot = QString(SETTINGS_KEY) + '/';
    forEachAspect([settings, keyRoot](BaseAspect *aspect) {
        QString key = aspect->settingsKey();
        const QVariant value = settings->value(keyRoot + key, aspect->defaultValue());
        aspect->setValue(value);
    });
}

// DockerOptionsPage

DockerOptionsPage::DockerOptionsPage(DockerSettings *settings)
{
    setId(Constants::DOCKER_SETTINGS_ID);
    setDisplayName(DockerSettings::tr("Docker"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer", "Devices"));
    setCategoryIconPath(":/projectexplorer/images/settingscategory_devices.png");
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        using namespace Layouting;
        DockerSettings &s = *settings;

        Column {
            Group {
                Title(DockerSettings::tr("Search Images on Docker Hub")),
                Form {
                    s.imageListFilter,
                    s.imageList
                },
            },
            Stretch()
        }.attachTo(widget);
    });
}

} // Internal
} // Docker
