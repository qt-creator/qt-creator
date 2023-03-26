// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace qmt {

class ModelTreeViewInterface
{
public:
    virtual ~ModelTreeViewInterface() { }

    virtual QModelIndex currentSourceModelIndex() const = 0;
    virtual QList<QModelIndex> selectedSourceModelIndexes() const = 0;
};

} // namespace qmt
