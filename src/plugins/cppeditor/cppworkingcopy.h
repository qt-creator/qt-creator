// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/filepath.h>

#include <QHash>
#include <QString>
#include <QPair>

#include <optional>

namespace CppEditor {

class CPPEDITOR_EXPORT WorkingCopy
{
public:
    WorkingCopy();

    void insert(const Utils::FilePath &fileName, const QByteArray &source, unsigned revision = 0)
    { _elements.insert(fileName, {source, revision}); }

    std::optional<QByteArray> source(const Utils::FilePath &fileName) const;

    unsigned revision(const Utils::FilePath &fileName) const
    { return _elements.value(fileName).second; }

    std::optional<QPair<QByteArray, unsigned>> get(const Utils::FilePath &fileName) const;

    using Table = QHash<Utils::FilePath, QPair<QByteArray, unsigned> >;
    const Table &elements() const
    { return _elements; }

    int size() const
    { return _elements.size(); }

private:
    Table _elements;
};

} // CppEditor
