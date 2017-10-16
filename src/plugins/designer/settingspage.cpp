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

#include "settingspage.h"
#include "formeditorw.h"

#include <coreplugin/icontext.h>

#include <utils/asconst.h>
#include <utils/stringutils.h>

#include <QDesignerOptionsPageInterface>
#include <QCoreApplication>

using namespace Designer::Internal;

SettingsPage::SettingsPage(QDesignerOptionsPageInterface *designerPage) :
    m_designerPage(designerPage)
{
    setId(Core::Id::fromString(m_designerPage->name()));
    setDisplayName(m_designerPage->name());
    setCategory(Designer::Constants::SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Designer",
        Designer::Constants::SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Designer::Constants::SETTINGS_CATEGORY_ICON));
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
    : IOptionsPageProvider(parent)
{
    setCategory(Designer::Constants::SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Designer",
        Designer::Constants::SETTINGS_TR_CATEGORY));
    setCategoryIcon(Designer::Constants::SETTINGS_CATEGORY_ICON);
}

QList<Core::IOptionsPage *> SettingsPageProvider::pages() const
{
    if (!m_initialized) {
        // get options pages from designer
        m_initialized = true;
        FormEditorW::ensureInitStage(FormEditorW::RegisterPlugins);
    }
    return FormEditorW::optionsPages();
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
            m_keywords << Utils::stripAccelerator(QCoreApplication::translate(uitext[i].context, uitext[i].value));
    }
    for (const QString &key : Utils::asConst(m_keywords)) {
        if (key.contains(searchKeyWord, Qt::CaseInsensitive))
            return true;
    }
    return false;
}
