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

namespace {
// TODO: should be extern, check coreplugin/dialogs/newdialogwidget.cpp
const char BLACKLISTED_CATEGORIES_KEY[] = "Core/NewDialog/BlacklistedCategories";
}

using namespace StudioWelcome;

WizardFactories::WizardFactories(QList<Core::IWizardFactory *> &factories, QWidget *wizardParent, const Utils::Id &platform)
    : m_wizardParent{wizardParent}
    , m_platform{platform}
    , m_factories{factories}
{
    QVariant value = Core::ICore::settings()->value(BLACKLISTED_CATEGORIES_KEY);
    m_blacklist = Utils::Id::fromStringList(value.toStringList());

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
    QList<Core::IWizardFactory *> acceptedFactories;
    // TODO: perhaps I could use Utils::filtered here.
    std::copy_if(std::begin(m_factories), std::end(m_factories), std::back_inserter(acceptedFactories),
                 [&](auto *wizard) {
                     return wizard->isAvailable(m_platform)
                            && wizard->kind() == Core::IWizardFactory::ProjectWizard
                            && !m_blacklist.contains(Utils::Id::fromString(wizard->category()));
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
        /*.fontIconCode =*/QmlDesigner::Theme::getIconUnicode(f->fontIconName()),
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
