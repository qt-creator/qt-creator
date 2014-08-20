/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "settingspage.h"
#include "formeditorw.h"

#include <QDesignerOptionsPageInterface>
#include <QCoreApplication>

using namespace Designer::Internal;

SettingsPage::SettingsPage(QDesignerOptionsPageInterface *designerPage) :
    m_designerPage(designerPage), m_initialized(false)
{
    setId(Core::Id::fromString(m_designerPage->name()));
    setDisplayName(m_designerPage->name());
    setCategory(Designer::Constants::SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Designer",
        Designer::Constants::SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Designer::Constants::SETTINGS_CATEGORY_ICON));
}

QWidget *SettingsPage::widget()
{
    m_initialized = true;
    if (!m_widget)
        m_widget = m_designerPage->createPage(0);
    return m_widget;

}

void SettingsPage::apply()
{
    if (m_initialized)
        m_designerPage->apply();
}

void SettingsPage::finish()
{
    if (m_initialized)
        m_designerPage->finish();
    delete m_widget;
}

SettingsPageProvider::SettingsPageProvider(QObject *parent)
    : IOptionsPageProvider(parent), m_initialized(false)
{
    setCategory(Designer::Constants::SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Designer",
        Designer::Constants::SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(Designer::Constants::SETTINGS_CATEGORY_ICON));
}

QList<Core::IOptionsPage *> SettingsPageProvider::pages() const
{
    if (!m_initialized) {
        // get options pages from designer
        m_initialized = true;
        FormEditorW::ensureInitStage(FormEditorW::RegisterPlugins);
    }
    return FormEditorW::instance()->optionsPages();
}

bool SettingsPageProvider::matches(const QString &searchKeyWord) const
{
    // to avoid fully initializing designer when typing something in the options' filter edit
    // we hardcode matching of UI text from the designer pages, which are taken if the designer pages
    // were not yet loaded
    // luckily linguist cannot resolve the translated texts, so we do not end up with duplicate
    // translatable strings for Qt Creator
    static const struct { const char *context; const char *value; } uitext[] = {
        {"EmbeddedOptionsPage", "Embedded Design"},
        {"EmbeddedOptionsPage", "Device Profiles"},
        {"FormEditorOptionsPage", "Forms"},
        {"FormEditorOptionsPage", "Preview Zoom"},
        {"FormEditorOptionsPage", "Default Zoom"},
        {"FormEditorOptionsPage", "Default Grid"},
        {"qdesigner_internal::GridPanel", "Visible"},
        {"qdesigner_internal::GridPanel", "Snap"},
        {"qdesigner_internal::GridPanel", "Reset"},
        {"qdesigner_internal::GridPanel", "Grid"},
        {"qdesigner_internal::GridPanel", "Grid &X"},
        {"qdesigner_internal::GridPanel", "Grid &Y"},
        {"PreviewConfigurationWidget", "Print/Preview Configuration"},
        {"PreviewConfigurationWidget", "Style"},
        {"PreviewConfigurationWidget", "Style sheet"},
        {"PreviewConfigurationWidget", "Device skin"},
        {"TemplateOptionsPage", "Template Paths"},
        {"qdesigner_internal::TemplateOptionsWidget", "Additional Template Paths"}
    };
    static const size_t itemCount = sizeof(uitext)/sizeof(uitext[0]);
    if (m_keywords.isEmpty()) {
        m_keywords.reserve(itemCount);
        for (size_t i = 0; i < itemCount; ++i)
            m_keywords << QCoreApplication::translate(uitext[i].context, uitext[i].value).remove(QLatin1Char('&'));
    }
    foreach (const QString &key, m_keywords) {
        if (key.contains(searchKeyWord, Qt::CaseInsensitive))
            return true;
    }
    return false;
}
