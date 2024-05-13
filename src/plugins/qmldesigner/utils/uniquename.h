// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace QmlDesigner {

class UniqueName
{
public:
    static QString get(const QString &oldName, std::function<bool(const QString &)> predicate);

private:
    static QString nextName(const QString &oldName);
};

} // namespace QmlDesigner
