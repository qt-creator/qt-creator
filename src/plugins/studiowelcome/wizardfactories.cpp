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

#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>
#include <utils/algorithm.h>

#include "wizardfactories.h"
#include <qmldesigner/components/componentcore/theme.h>

using namespace StudioWelcome;

WizardFactories::GetIconUnicodeFunc WizardFactories::m_getIconUnicode = &QmlDesigner::Theme::getIconUnicode;

WizardFactories::WizardFactories(QList<Core::IWizardFactory *> &factories, QWidget *wizardParent, const Utils::Id &platform)
    : m_wizardParent{wizardParent}
    , m_platform{platform}
    , m_factories{factories}
{
    sortByCategoryAndId();
    filter();
    m_projectItems = makeProjectItemsGroupedByCategory();
}

void WizardFactories::sortByCategoryAndId()
{
    Utils::sort(m_factories, [](Core::IWizardFactory *lhs, Core::IWizardFactory *rhs) {
        if (lhs->category() == rhs->category())
            return lhs->id().toString() < rhs->id().toString();
        else
            return lhs->category() < rhs->category();
    });
}

void WizardFactories::filter()
{
    QList<Core::IWizardFactory *> acceptedFactories = Utils::filtered(m_factories, [&](auto *wizard) {
        return wizard->isAvailable(m_platform)
                && wizard->kind() == Core::IWizardFactory::ProjectWizard
                && wizard->requiredFeatures().contains("QtStudio");
    });

    m_factories = acceptedFactories;
}

ProjectItem WizardFactories::makeProjectItem(Core::IWizardFactory *f, QWidget *parent,
                                             const Utils::Id &platform)
{
    using namespace std::placeholders;

    return {
        /*.name =*/f->displayName(),
        /*.categoryId =*/f->category(),
        /*. description =*/f->description(),
        /*.qmlPath =*/f->detailsPageQmlPath(),
        /*.fontIconCode =*/m_getIconUnicode(f->fontIconName()),
        /*.create =*/ std::bind(&Core::IWizardFactory::runWizard, f, _1, parent, platform,
                               QVariantMap(), false),
    };
}

std::map<QString, ProjectCategory> WizardFactories::makeProjectItemsGroupedByCategory()
{
    QMap<QString, ProjectCategory> categories;

    for (auto *f : std::as_const(m_factories)) {
        if (!categories.contains(f->category())) {
            categories[f->category()] = {
                /*.id =*/ f->category(),
                /*.name =*/ f->displayCategory(),
                /*.items = */
                {
                    makeProjectItem(f, m_wizardParent, m_platform),
                },
            };
        } else {
            auto projectItem = makeProjectItem(f, m_wizardParent, m_platform);
            categories[f->category()].items.push_back(projectItem);
        }
    }

    return categories.toStdMap();
}
