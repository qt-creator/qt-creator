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

#include "wizardfactories.h"
#include "algorithm.h"

#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <qmldesigner/components/componentcore/theme.h>

using namespace StudioWelcome;

WizardFactories::GetIconUnicodeFunc WizardFactories::m_getIconUnicode = &QmlDesigner::Theme::getIconUnicode;

WizardFactories::WizardFactories(const QList<Core::IWizardFactory *> &factories,
                                 QWidget *wizardParent,
                                 const Utils::Id &platform)
    : m_wizardParent{wizardParent}
    , m_platform{platform}
{
    m_factories = Utils::filtered(Utils::transform(factories, [](Core::IWizardFactory *f) {
        return qobject_cast<JsonWizardFactory *>(f);
    }));

    sortByCategoryAndId();
    filter();
    m_presetItems = makePresetItemsGroupedByCategory();
}

const Core::IWizardFactory *WizardFactories::front() const
{
    return m_factories.front();
}

void WizardFactories::sortByCategoryAndId()
{
    Utils::sort(m_factories, [](JsonWizardFactory *lhs, JsonWizardFactory *rhs) {
        if (lhs->category() == rhs->category())
            return lhs->id().toString() < rhs->id().toString();
        else
            return lhs->category() < rhs->category();
    });
}

void WizardFactories::filter()
{
    QList<JsonWizardFactory *> acceptedFactories = Utils::filtered(m_factories, [&](auto *wizard) {
        return wizard->isAvailable(m_platform)
               && wizard->kind() == JsonWizardFactory::ProjectWizard;
    });

    m_factories = acceptedFactories;
}

std::shared_ptr<PresetItem> WizardFactories::makePresetItem(JsonWizardFactory *f, QWidget *parent,
                                             const Utils::Id &platform)
{
    using namespace std::placeholders;

    QString sizeName;
    auto [index, screenSizes] = f->screenSizeInfoFromPage("Fields");

    if (index < 0 || index >= screenSizes.size())
        sizeName.clear();
    else
        sizeName = screenSizes[index];

    auto result = std::make_shared<PresetItem>();
    result->wizardName = f->displayName();
    result->categoryId = f->category();
    result->screenSizeName=sizeName;
    result->description = f->description();
    result->qmlPath = f->detailsPageQmlPath();
    result->fontIconCode = m_getIconUnicode(f->fontIconName());
    result->create
        = std::bind(&JsonWizardFactory::runWizard, f, _1, parent, platform, QVariantMap(), false);

    return result;
}

std::map<QString, WizardCategory> WizardFactories::makePresetItemsGroupedByCategory()
{
    QMap<QString, WizardCategory> categories;

    for (auto *f : std::as_const(m_factories)) {
        if (!categories.contains(f->category())) {
            categories[f->category()] = {
                /*.id =*/ f->category(),
                /*.name =*/ f->displayCategory(),
                /*.items = */
                {
                    makePresetItem(f, m_wizardParent, m_platform),
                },
            };
        } else {
            auto presetItem = makePresetItem(f, m_wizardParent, m_platform);
            categories[f->category()].items.push_back(presetItem);
        }
    }

    return categories.toStdMap();
}
