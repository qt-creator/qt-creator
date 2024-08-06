// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "usercategory.h"

namespace QmlDesigner {

UserCategory::UserCategory(const QString &title, const Utils::FilePath &bundlePath)
    : m_title(title)
    , m_bundlePath(bundlePath)
{
}

QString UserCategory::title() const
{
    return m_title;
}

bool UserCategory::isEmpty() const
{
    return m_isEmpty;
}

void UserCategory::setIsEmpty(bool val)
{
    if (m_isEmpty == val)
        return;

    m_isEmpty = val;
    emit isEmptyChanged();
}

bool UserCategory::noMatch() const
{
    return m_noMatch;
}

void UserCategory::setNoMatch(bool val)
{
    if (m_noMatch == val)
        return;

    m_noMatch = val;
    emit noMatchChanged();
}

void UserCategory::addItem(QObject *item)
{
    m_items.append(item);
    emit itemsChanged();

    setIsEmpty(false);
}

void UserCategory::removeItem(QObject *item)
{
    m_items.removeOne(item);
    item->deleteLater();
    emit itemsChanged();

    setIsEmpty(m_items.isEmpty());
}

Utils::FilePath UserCategory::bundlePath() const
{
    return m_bundlePath;
}

QObjectList UserCategory::items() const
{
    return m_items;
}

} // namespace QmlDesigner
