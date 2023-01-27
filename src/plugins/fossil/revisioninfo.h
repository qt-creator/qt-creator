// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QStringList>

namespace Fossil {
namespace Internal {

class RevisionInfo
{
public:
    const QString id;
    const QString parentId;
    const QStringList mergeParentIds;
    const QString commentMsg;
    const QString committer;
};

} // namespace Internal
} // namespace Fossil
