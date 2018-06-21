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

#include "fileshareprotocolsettingspage.h"
#include "cpasterconstants.h"

#include <coreplugin/icore.h>

#include <utils/temporarydirectory.h>

#include <QSettings>
#include <QCoreApplication>

static const char settingsGroupC[] = "FileSharePasterSettings";
static const char pathKeyC[] = "Path";
static const char displayCountKeyC[] = "DisplayCount";

namespace CodePaster {

FileShareProtocolSettings::FileShareProtocolSettings() :
        path(Utils::TemporaryDirectory::masterDirectoryPath()), displayCount(10)
{
}

void FileShareProtocolSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(pathKeyC), path);
    s->setValue(QLatin1String(displayCountKeyC), displayCount);
    s->endGroup();
}

void FileShareProtocolSettings::fromSettings(const QSettings *s)
{
    FileShareProtocolSettings defaultValues;
    const QString keyRoot = QLatin1String(settingsGroupC) + QLatin1Char('/');
    path = s->value(keyRoot + QLatin1String(pathKeyC), defaultValues.path).toString();
    displayCount = s->value(keyRoot + QLatin1String(displayCountKeyC), defaultValues.displayCount).toInt();
}

bool FileShareProtocolSettings::equals(const FileShareProtocolSettings &rhs) const
{
    return displayCount == rhs.displayCount &&  path == rhs.path;
}

FileShareProtocolSettingsWidget::FileShareProtocolSettingsWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);

    // Add a space in front of the suffix
    QString suffix = m_ui.displayCountSpinBox->suffix();
    suffix.prepend(QLatin1Char(' '));
    m_ui.displayCountSpinBox->setSuffix(suffix);
}

void FileShareProtocolSettingsWidget::setSettings(const FileShareProtocolSettings &s)
{
    m_ui.pathChooser->setPath(s.path);
    m_ui.displayCountSpinBox->setValue(s.displayCount);
}

FileShareProtocolSettings FileShareProtocolSettingsWidget::settings() const
{
    FileShareProtocolSettings rc;
    rc.path = m_ui.pathChooser->path();
    rc.displayCount = m_ui.displayCountSpinBox->value();
    return rc;
}

// ----------FileShareProtocolSettingsPage
FileShareProtocolSettingsPage::FileShareProtocolSettingsPage(const QSharedPointer<FileShareProtocolSettings> &s,
                                                             QObject *parent) :
    Core::IOptionsPage(parent), m_settings(s), m_widget(nullptr)
{
    setId("X.CodePaster.FileSharePaster");
    setDisplayName(tr("Fileshare"));
    setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
}

QWidget *FileShareProtocolSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new FileShareProtocolSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void FileShareProtocolSettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    const FileShareProtocolSettings newSettings = m_widget->settings();
    if (newSettings != *m_settings) {
        *m_settings = newSettings;
        m_settings->toSettings(Core::ICore::settings());
    }
}
} // namespace CodePaster
