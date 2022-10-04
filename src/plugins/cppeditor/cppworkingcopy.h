// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/fileutils.h>

#include <QHash>
#include <QString>
#include <QPair>

namespace CppEditor {

class CPPEDITOR_EXPORT WorkingCopy
{
public:
    WorkingCopy();

    void insert(const QString &fileName, const QByteArray &source, unsigned revision = 0)
    { insert(Utils::FilePath::fromString(fileName), source, revision); }

    void insert(const Utils::FilePath &fileName, const QByteArray &source, unsigned revision = 0)
    { _elements.insert(fileName, {source, revision}); }

    bool contains(const QString &fileName) const
    { return contains(Utils::FilePath::fromString(fileName)); }

    bool contains(const Utils::FilePath &fileName) const
    { return _elements.contains(fileName); }

    QByteArray source(const QString &fileName) const
    { return source(Utils::FilePath::fromString(fileName)); }

    QByteArray source(const Utils::FilePath &fileName) const
    { return _elements.value(fileName).first; }

    unsigned revision(const QString &fileName) const
    { return revision(Utils::FilePath::fromString(fileName)); }

    unsigned revision(const Utils::FilePath &fileName) const
    { return _elements.value(fileName).second; }

    QPair<QByteArray, unsigned> get(const QString &fileName) const
    { return get(Utils::FilePath::fromString(fileName)); }

    QPair<QByteArray, unsigned> get(const Utils::FilePath &fileName) const
    { return _elements.value(fileName); }

    using Table = QHash<Utils::FilePath, QPair<QByteArray, unsigned> >;
    const Table &elements() const
    { return _elements; }

    int size() const
    { return _elements.size(); }

private:
    Table _elements;
};

} // namespace CppEditor
