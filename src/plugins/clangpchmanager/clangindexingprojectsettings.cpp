/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "clangindexingprojectsettings.h"

#include <projectexplorer/project.h>

namespace ClangPchManager {

namespace {
Utils::NameValueItems fromQVariantMap(const QVariantMap &variantMap,
                                      Utils::NameValueItem::Operation operation)
{
    Utils::NameValueItems nameValueItems;
    nameValueItems.reserve(variantMap.size());

    auto end = variantMap.end();

    for (auto iterator = variantMap.cbegin(); iterator != end; ++iterator) // QMap iterators are broken
        nameValueItems.push_back({iterator.key(), iterator.value().toString(), operation});

    return nameValueItems;
}

} // namespace

ClangIndexingProjectSettings::ClangIndexingProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{}

void ClangIndexingProjectSettings::saveMacros(const Utils::NameValueItems &items)
{
    QVariantMap unsets;
    QVariantMap sets;

    for (const Utils::NameValueItem &item : items) {
        using Operation = Utils::NameValueItem::Operation;
        switch (item.operation) {
        case Operation::SetEnabled:
            sets[item.name] = item.value;
            break;
        case Operation::Unset:
            unsets[item.name] = item.value;
            break;
        default:
            break;
        }
    }

    if (sets.size())
        m_project->setNamedSettings("set_indexing_macro", sets);
    else
        m_project->setNamedSettings("set_indexing_macro", {});

    if (unsets.size())
        m_project->setNamedSettings("unset_indexing_macro", unsets);
    else
        m_project->setNamedSettings("unset_indexing_macro", {});
}

Utils::NameValueItems ClangIndexingProjectSettings::readMacros() const
{
    QVariant unsets = m_project->namedSettings("unset_indexing_macro");

    Utils::NameValueItems items = fromQVariantMap(unsets.toMap(), Utils::NameValueItem::Unset);

    QVariant sets = m_project->namedSettings("set_indexing_macro");

    items += fromQVariantMap(sets.toMap(), Utils::NameValueItem::SetEnabled);

    return items;
}

} // namespace ClangPchManager
