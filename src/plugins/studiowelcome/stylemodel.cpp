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

#include "stylemodel.h"

#include "utils/algorithm.h"
#include "utils/qtcassert.h"

#include <QRegularExpression>

using namespace StudioWelcome;

StyleModel::StyleModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_backendModel(nullptr)
{}

QString StyleModel::iconId(int index) const
{
    if (!m_backendModel || index < 0)
        return "style-error";

    auto item = this->m_filteredItems.at(index);
    QString styleName = item->text();
    QString id{"style-"};
    id += styleName.toLower().replace(' ', '_') + ".png";

    return id;
}

void StyleModel::filter(const QString &what)
{
    QTC_ASSERT(!what.isEmpty(), return);

    if (what.toLower() == "all")
        m_filteredItems = this->filterItems(m_items, "");
    else if (what.toLower() == "light")
        m_filteredItems = this->filterItems(m_items, "light");
    else if (what.toLower() == "dark")
        m_filteredItems = this->filterItems(m_items, "dark");
    else
        m_filteredItems.clear();

    reset();
}

StyleModel::Items StyleModel::filterItems(const Items &items, const QString &kind)
{
    if (kind.isEmpty())
        return items;

    return Utils::filtered(items, [&kind](auto *item) {
        QString pattern{"\\S "};
        pattern += kind;

        QRegularExpression re{pattern, QRegularExpression::CaseInsensitiveOption};
        return re.match(item->text()).hasMatch();
    });
}

int StyleModel::filteredIndex(int actualIndex)
{
    if (actualIndex < 0)
        return actualIndex;

    QStandardItem *item = m_items.at(actualIndex);
    // TODO: perhaps should add this kind of find to utils/algorithm.h
    auto it = std::find(std::cbegin(m_filteredItems), std::cend(m_filteredItems), item);
    if (it == std::cend(m_filteredItems))
        return -1;

    return std::distance(std::cbegin(m_filteredItems), it);
}

int StyleModel::actualIndex(int filteredIndex)
{
    if (filteredIndex < 0)
        return filteredIndex;

    QTC_ASSERT(filteredIndex < static_cast<int>(m_filteredItems.size()), return -1);

    QStandardItem *item = m_filteredItems.at(filteredIndex);
    auto it = std::find(std::cbegin(m_items), std::cend(m_items), item);
    if (it == std::cend(m_items))
        return -1;

    auto result = std::distance(std::cbegin(m_items), it);
    QTC_ASSERT(result >= 0, return -1);
    QTC_ASSERT(result <= static_cast<int>(m_items.size()), return -1);

    return result;
}

void StyleModel::setBackendModel(QStandardItemModel *model)
{
    m_backendModel = model;

    if (m_backendModel) {
        m_count = model->rowCount();
        m_roles = model->roleNames();
        m_items.clear();

        for (int i = 0; i < m_count; ++i)
            m_items.push_back(model->item(i, 0));

        m_filteredItems = filterItems(m_items, "");
    } else {
        m_count = 0;
        m_items.clear();
        m_filteredItems.clear();
    }
}
