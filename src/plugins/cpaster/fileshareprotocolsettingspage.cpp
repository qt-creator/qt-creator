/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "fileshareprotocolsettingspage.h"
#include "cpasterconstants.h"

#include <coreplugin/icore.h>

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>

static const char settingsGroupC[] = "FileSharePasterSettings";
static const char pathKeyC[] = "Path";
static const char displayCountKeyC[] = "DisplayCount";

namespace CodePaster {

FileShareProtocolSettings::FileShareProtocolSettings() :
        path(QDir::tempPath()), displayCount(10)
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
    Core::IOptionsPage(parent), m_settings(s)
{
}

QString FileShareProtocolSettingsPage::id() const
{
    return QLatin1String("X.FileSharePaster");
}

QString FileShareProtocolSettingsPage::displayName() const
{
    return tr("Fileshare");
}

QString FileShareProtocolSettingsPage::category() const
{
    return QLatin1String(Constants::CPASTER_SETTINGS_CATEGORY);
}

QString FileShareProtocolSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("CodePaster", Constants::CPASTER_SETTINGS_TR_CATEGORY);
}

QIcon FileShareProtocolSettingsPage::categoryIcon() const
{
    return QIcon();
}

QWidget *FileShareProtocolSettingsPage::createPage(QWidget *parent)
{
    m_widget = new FileShareProtocolSettingsWidget(parent);
    m_widget->setSettings(*m_settings);
    return m_widget;
}

void FileShareProtocolSettingsPage::apply()
{
    const FileShareProtocolSettings newSettings = m_widget->settings();
    if (newSettings != *m_settings) {
        *m_settings = newSettings;
        m_settings->toSettings(Core::ICore::instance()->settings());
    }
}
} // namespace CodePaster
