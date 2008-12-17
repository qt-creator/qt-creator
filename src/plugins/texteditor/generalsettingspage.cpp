/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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

#include "displaysettings.h"
#include "generalsettingspage.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "ui_generalsettingspage.h"

#include <QtCore/QSettings>
#include <QtCore/QDebug>

using namespace TextEditor;

struct GeneralSettingsPage::GeneralSettingsPagePrivate {
    GeneralSettingsPagePrivate(Core::ICore *core,
                           const GeneralSettingsPageParameters &p);

    Core::ICore *m_core;
    const GeneralSettingsPageParameters m_parameters;
    Ui::generalSettingsPage m_page;
    TabSettings m_tabSettings;
    StorageSettings m_storageSettings;
    DisplaySettings m_displaySettings;
};

GeneralSettingsPage::GeneralSettingsPagePrivate::GeneralSettingsPagePrivate(Core::ICore *core,
                                                                const GeneralSettingsPageParameters &p) :
   m_core(core),
   m_parameters(p)
{
    if (m_core)
        if (const QSettings *s = m_core->settings()) {
            m_tabSettings.fromSettings(m_parameters.settingsPrefix, s);
            m_storageSettings.fromSettings(m_parameters.settingsPrefix, s);
            m_displaySettings.fromSettings(m_parameters.settingsPrefix, s);
        }
}

GeneralSettingsPage::GeneralSettingsPage(Core::ICore *core,
                                 const GeneralSettingsPageParameters &p,
                                 QObject *parent) :
    Core::IOptionsPage(parent),
    m_d(new GeneralSettingsPagePrivate(core, p))
{
}

GeneralSettingsPage::~GeneralSettingsPage()
{
    delete m_d;
}

QString GeneralSettingsPage::name() const
{
    return m_d->m_parameters.name;
}

QString GeneralSettingsPage::category() const
{
    return  m_d->m_parameters.category;
}

QString GeneralSettingsPage::trCategory() const
{
    return m_d->m_parameters.trCategory;
}

QWidget *GeneralSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_d->m_page.setupUi(w);

    settingsToUI();

    return w;
}

void GeneralSettingsPage::finished(bool accepted)
{
    if (!accepted)
        return;

    TabSettings newTabSettings;
    StorageSettings newStorageSettings;
    DisplaySettings newDisplaySettings;
    settingsFromUI(newTabSettings, newStorageSettings, newDisplaySettings);

    if (newTabSettings != m_d->m_tabSettings) {
        m_d->m_tabSettings = newTabSettings;
        if (m_d->m_core)
            if (QSettings *s = m_d->m_core->settings())
                m_d->m_tabSettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit tabSettingsChanged(newTabSettings);
    }

    if (newStorageSettings != m_d->m_storageSettings) {
        m_d->m_storageSettings = newStorageSettings;
        if (m_d->m_core)
            if (QSettings *s = m_d->m_core->settings())
                m_d->m_storageSettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit storageSettingsChanged(newStorageSettings);
    }

    if (newDisplaySettings != m_d->m_displaySettings) {
        m_d->m_displaySettings = newDisplaySettings;
        if (m_d->m_core)
            if (QSettings *s = m_d->m_core->settings())
                m_d->m_displaySettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit displaySettingsChanged(newDisplaySettings);
    }
}

void GeneralSettingsPage::settingsFromUI(TabSettings &rc,
                                         StorageSettings &storageSettings,
                                         DisplaySettings &displaySettings) const
{
    rc.m_spacesForTabs = m_d->m_page.insertSpaces->isChecked();
    rc.m_autoIndent = m_d->m_page.autoIndent->isChecked();
    rc.m_smartBackspace = m_d->m_page.smartBackspace->isChecked();
    rc.m_tabSize = m_d->m_page.tabSize->value();
    rc.m_indentSize = m_d->m_page.indentSize->value();

    storageSettings.m_cleanWhitespace = m_d->m_page.cleanWhitespace->isChecked();
    storageSettings.m_inEntireDocument = m_d->m_page.inEntireDocument->isChecked();
    storageSettings.m_addFinalNewLine = m_d->m_page.addFinalNewLine->isChecked();

    displaySettings.m_displayLineNumbers = m_d->m_page.displayLineNumbers->isChecked();
    displaySettings.m_textWrapping = m_d->m_page.enableTextWrapping->isChecked();
    displaySettings.m_showWrapColumn = m_d->m_page.showWrapColumn->isChecked();
    displaySettings.m_wrapColumn = m_d->m_page.wrapColumn->value();
    displaySettings.m_visualizeWhitespace = m_d->m_page.visualizeWhitespace->isChecked();
    displaySettings.m_displayFoldingMarkers = m_d->m_page.displayFoldingMarkers->isChecked();
    displaySettings.m_highlightCurrentLine = m_d->m_page.highlightCurrentLine->isChecked();
}

void GeneralSettingsPage::settingsToUI()
{
    TabSettings rc = m_d->m_tabSettings;
    m_d->m_page.insertSpaces->setChecked(rc.m_spacesForTabs);
    m_d->m_page.autoIndent->setChecked(rc.m_autoIndent);
    m_d->m_page.smartBackspace->setChecked(rc.m_smartBackspace);
    m_d->m_page.tabSize->setValue(rc.m_tabSize);
    m_d->m_page.indentSize->setValue(rc.m_indentSize);

    StorageSettings storageSettings = m_d->m_storageSettings;
    m_d->m_page.cleanWhitespace->setChecked(storageSettings.m_cleanWhitespace);
    m_d->m_page.inEntireDocument->setChecked(storageSettings.m_inEntireDocument);
    m_d->m_page.addFinalNewLine->setChecked(storageSettings.m_addFinalNewLine);

    DisplaySettings displaySettings = m_d->m_displaySettings;
    m_d->m_page.displayLineNumbers->setChecked(displaySettings.m_displayLineNumbers);
    m_d->m_page.enableTextWrapping->setChecked(displaySettings.m_textWrapping);
    m_d->m_page.showWrapColumn->setChecked(displaySettings.m_showWrapColumn);
    m_d->m_page.wrapColumn->setValue(displaySettings.m_wrapColumn);
    m_d->m_page.visualizeWhitespace->setChecked(displaySettings.m_visualizeWhitespace);
    m_d->m_page.displayFoldingMarkers->setChecked(displaySettings.m_displayFoldingMarkers);
    m_d->m_page.highlightCurrentLine->setChecked(displaySettings.m_highlightCurrentLine);
}

TabSettings GeneralSettingsPage::tabSettings() const
{
    return m_d->m_tabSettings;
}

StorageSettings GeneralSettingsPage::storageSettings() const
{
    return m_d->m_storageSettings;
}

DisplaySettings GeneralSettingsPage::displaySettings() const
{
    return m_d->m_displaySettings;
}

void GeneralSettingsPage::setDisplaySettings(const DisplaySettings &newDisplaySettings)
{
    if (newDisplaySettings != m_d->m_displaySettings) {
        m_d->m_displaySettings = newDisplaySettings;
        if (m_d->m_core)
            if (QSettings *s = m_d->m_core->settings())
                m_d->m_displaySettings.toSettings(m_d->m_parameters.settingsPrefix, s);

        emit displaySettingsChanged(newDisplaySettings);
    }
}
