// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingspage.h"

#include "designerconstants.h"
#include "designertr.h"
#include "formeditor.h"

#include <coreplugin/icontext.h>

#include <utils/stringutils.h>

#include <QDesignerOptionsPageInterface>
#include <QCoreApplication>
#include <QVBoxLayout>

namespace Designer::Internal {

class SettingsPageWidget : public Core::IOptionsPageWidget
{
public:
    explicit SettingsPageWidget(QDesignerOptionsPageInterface *designerPage)
        : m_designerPage(designerPage)
    {
        auto vbox = new QVBoxLayout(this);
        vbox->addWidget(m_designerPage->createPage(nullptr));
        vbox->setContentsMargins({});
    }

    void apply() { m_designerPage->apply(); }
    void finish() { m_designerPage->finish(); }

    QDesignerOptionsPageInterface *m_designerPage;
};


SettingsPage::SettingsPage(QDesignerOptionsPageInterface *designerPage)
    : Core::IOptionsPage(false)
{
    setId(Utils::Id::fromString(designerPage->name()));
    setDisplayName(designerPage->name());
    setCategory(Designer::Constants::SETTINGS_CATEGORY);
    setWidgetCreator([designerPage] { return new SettingsPageWidget(designerPage); });
}

SettingsPageProvider::SettingsPageProvider()
{
    setCategory(Designer::Constants::SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr(Designer::Constants::SETTINGS_TR_CATEGORY));
    setCategoryIconPath(":/core/images/settingscategory_design.png");
}

QList<Core::IOptionsPage *> SettingsPageProvider::pages() const
{
    if (!m_initialized) {
        // get options pages from designer
        m_initialized = true;
        ensureInitStage(RegisterPlugins);
    }
    return optionsPages();
}

bool SettingsPageProvider::matches(const QRegularExpression &regex) const
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
    for (const QString &key : std::as_const(m_keywords)) {
        if (key.contains(regex))
            return true;
    }
    return false;
}

} // Designer::Internal
