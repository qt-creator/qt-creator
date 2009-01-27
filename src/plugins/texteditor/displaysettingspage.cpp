/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "displaysettingspage.h"
#include "displaysettings.h"
#include "ui_displaysettingspage.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>

using namespace TextEditor;

struct DisplaySettingsPage::DisplaySettingsPagePrivate
{
    explicit DisplaySettingsPagePrivate(const DisplaySettingsPageParameters &p);

    const DisplaySettingsPageParameters m_parameters;
    Ui::DisplaySettingsPage m_page;
    DisplaySettings m_displaySettings;
};

DisplaySettingsPage::DisplaySettingsPagePrivate::DisplaySettingsPagePrivate
    (const DisplaySettingsPageParameters &p)
    : m_parameters(p)
{
    if (const QSettings *s = Core::ICore::instance()->settings()) {
        m_displaySettings.fromSettings(m_parameters.settingsPrefix, s);
    }
}

DisplaySettingsPage::DisplaySettingsPage(const DisplaySettingsPageParameters &p,
                                         QObject *parent)
  : Core::IOptionsPage(parent),
    m_d(new DisplaySettingsPagePrivate(p))
{
}

DisplaySettingsPage::~DisplaySettingsPage()
{
    delete m_d;
}

QString DisplaySettingsPage::name() const
{
    return m_d->m_parameters.name;
}

QString DisplaySettingsPage::category() const
{
    return m_d->m_parameters.category;
}

QString DisplaySettingsPage::trCategory() const
{
    return m_d->m_parameters.trCategory;
}

QWidget *DisplaySettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->m_page.setupUi(w);
    settingsToUI();
    return w;
}

void DisplaySettingsPage::apply()
{
    DisplaySettings newDisplaySettings;

    settingsFromUI(newDisplaySettings);

    Core::ICore *core = Core::ICore::instance();
    QSettings *s = core->settings();

    if (newDisplaySettings != m_d->m_displaySettings) {
        m_d->m_displaySettings = newDisplaySettings;
        if (s)
            m_d->m_displaySettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit displaySettingsChanged(newDisplaySettings);
    }
}

void DisplaySettingsPage::settingsFromUI(DisplaySettings &displaySettings) const
{
    displaySettings.m_displayLineNumbers = m_d->m_page.displayLineNumbers->isChecked();
    displaySettings.m_textWrapping = m_d->m_page.enableTextWrapping->isChecked();
    displaySettings.m_showWrapColumn = m_d->m_page.showWrapColumn->isChecked();
    displaySettings.m_wrapColumn = m_d->m_page.wrapColumn->value();
    displaySettings.m_visualizeWhitespace = m_d->m_page.visualizeWhitespace->isChecked();
    displaySettings.m_displayFoldingMarkers = m_d->m_page.displayFoldingMarkers->isChecked();
    displaySettings.m_highlightCurrentLine = m_d->m_page.highlightCurrentLine->isChecked();
}

void DisplaySettingsPage::settingsToUI()
{
    const DisplaySettings &displaySettings = m_d->m_displaySettings;
    m_d->m_page.displayLineNumbers->setChecked(displaySettings.m_displayLineNumbers);
    m_d->m_page.enableTextWrapping->setChecked(displaySettings.m_textWrapping);
    m_d->m_page.showWrapColumn->setChecked(displaySettings.m_showWrapColumn);
    m_d->m_page.wrapColumn->setValue(displaySettings.m_wrapColumn);
    m_d->m_page.visualizeWhitespace->setChecked(displaySettings.m_visualizeWhitespace);
    m_d->m_page.displayFoldingMarkers->setChecked(displaySettings.m_displayFoldingMarkers);
    m_d->m_page.highlightCurrentLine->setChecked(displaySettings.m_highlightCurrentLine);
}

DisplaySettings DisplaySettingsPage::displaySettings() const
{
    return m_d->m_displaySettings;
}

void DisplaySettingsPage::setDisplaySettings(const DisplaySettings &newDisplaySettings)
{
    if (newDisplaySettings != m_d->m_displaySettings) {
        m_d->m_displaySettings = newDisplaySettings;
        Core::ICore *core = Core::ICore::instance();
        if (QSettings *s = core->settings())
            m_d->m_displaySettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit displaySettingsChanged(newDisplaySettings);
    }
}
