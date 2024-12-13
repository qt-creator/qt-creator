// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stylemodel.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

using namespace StudioWelcome;

StyleModel::StyleModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setFilterRole(Qt::DisplayRole);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(0);
}

void StyleModel::filter(const QString &what)
{
    using namespace Qt::StringLiterals;

    const QString &filterName = (what.toLower() == "all"_L1) ? "" : what;
    if (filterName.isEmpty()) {
        setFilterFixedString("");
    } else {
        QString pattern = R"(^(?:\w+|\s)+ )" + filterName + R"($)";
        setFilterRegularExpression(pattern);
    }
}

QVariant StyleModel::data(const QModelIndex &index, int role) const
{
    using namespace Qt::StringLiterals;
    if (!index.isValid()) {
        if (role == IconNameRole)
            return "style-error";
        return {};
    }

    switch (role) {
    case IconNameRole: {
        const QString &styleName = mapToSource(index).data(Qt::DisplayRole).toString();
        return QString{"style-%1.png"}.arg(styleName.toLower().replace(' ','_'));
    }; break;
    case Qt::DisplayRole:
    case Qt::EditRole:
    default:
        return mapToSource(index).data(role);
    }
}

QHash<int, QByteArray> StyleModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractProxyModel::roleNames();
    result.insert(IconNameRole, "iconName");
    return result;
}

int StyleModel::findIndex(const QString &styleName) const
{
    if (!sourceModel())
        return -1;

    int sourceIdx = findSourceIndex(styleName);
    const QModelIndex sourceModelIdx = sourceModel()->index(sourceIdx, 0);
    return mapFromSource(sourceModelIdx).row();
}

int StyleModel::findSourceIndex(const QString &styleName) const
{
    if (!sourceModel())
        return -1;

    const int sourceCount = sourceModel()->rowCount();
    const QString &styleNameLower = styleName.toLower();
    for (int i = 0; i < sourceCount; ++i) {
        const QModelIndex sourceIndex = sourceModel()->index(i, 0);
        const QString &itemStyleName = sourceIndex.data(Qt::DisplayRole).toString();
        if (styleNameLower == itemStyleName.toLower())
            return i;
    }

    return -1;
}
