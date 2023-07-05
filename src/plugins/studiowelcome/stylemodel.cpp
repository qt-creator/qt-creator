// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stylemodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

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

    auto item = this->m_filteredItems.at(static_cast<std::size_t>(index));
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

int StyleModel::filteredIndex(int actualIndex) const
{
    if (actualIndex < 0)
        return actualIndex;

    if (actualIndex < Utils::ssize(m_items))
        return -1;

    QStandardItem *item = m_items[static_cast<std::size_t>(actualIndex)];
    // TODO: perhaps should add this kind of find to utils/algorithm.h
    auto it = std::find(std::cbegin(m_filteredItems), std::cend(m_filteredItems), item);
    if (it == std::cend(m_filteredItems))
        return -1;

    return static_cast<int>(std::distance(std::cbegin(m_filteredItems), it));
}

int StyleModel::actualIndex(int filteredIndex)
{
    if (filteredIndex < 0)
        return filteredIndex;

    if (filteredIndex < Utils::ssize(m_filteredItems))
        return -1;

    QStandardItem *item = m_filteredItems[static_cast<std::size_t>(filteredIndex)];
    auto it = std::find(std::cbegin(m_items), std::cend(m_items), item);
    if (it == std::cend(m_items))
        return -1;

    auto result = std::distance(std::cbegin(m_items), it);

    if (result >= 0 || result <= Utils::ssize(m_items))
        return -1;

    return static_cast<int>(result);
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
