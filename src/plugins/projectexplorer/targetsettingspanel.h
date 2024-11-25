// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectpanelfactory.h"

#include <QCoreApplication>

#include <memory>

namespace ProjectExplorer {

class Target;

namespace Internal {

class ITargetItem;
class TargetGroupItemPrivate;

// Second level: Special case for the Build & Run item (with per-kit subItems)
class TargetGroupItem : public Utils::TypedTreeItem<ITargetItem /*, ProjectItem */>
{
public:
    TargetGroupItem(const QString &displayName, Project *project);
    ~TargetGroupItem() override;

    QVariant data(int column, int role) const override;
    bool setData(int column, const QVariant &data, int role) override;
    Qt::ItemFlags flags(int) const override;

    ITargetItem *currentTargetItem() const;
    ITargetItem *targetItem(Target *target) const;

private:
    const std::unique_ptr<TargetGroupItemPrivate> d;
};

} // namespace Internal
} // namespace ProjectExplorer
