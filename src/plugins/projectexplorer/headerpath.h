// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/algorithm.h>
#include <utils/filepath.h>

namespace ProjectExplorer {

enum class HeaderPathType {
    User,
    BuiltIn,
    System,
    Framework,
};

class HeaderPath
{
public:
    HeaderPath() = default;
    HeaderPath(const Utils::FilePath &path, HeaderPathType type)
        : path(path), type(type)
    {}

    bool operator==(const HeaderPath &other) const
    {
        return type == other.type && path == other.path;
    }

    bool operator!=(const HeaderPath &other) const
    {
        return !(*this == other);
    }

    static HeaderPath makeUser(const Utils::FilePath &fp)
    {
        return {fp, HeaderPathType::User};
    }
    static  HeaderPath makeBuiltIn(const Utils::FilePath &fp)
    {
        return {fp, HeaderPathType::BuiltIn};
    }
    static HeaderPath makeSystem(const Utils::FilePath &fp)
    {
        return {fp, HeaderPathType::System};
    }
    static HeaderPath makeFramework(const Utils::FilePath &fp)
    {
        return {fp, HeaderPathType::Framework};
    }

    friend size_t qHash(const HeaderPath &key, size_t seed = 0)
    {
        return ((qHash(key.path) << 2) | size_t(key.type)) ^ seed;
    }

    Utils::FilePath path;
    HeaderPathType type = HeaderPathType::User;
};

using HeaderPaths = QList<HeaderPath>;

inline HeaderPaths toHeaderPaths(const Utils::FilePaths &list, HeaderPathType type)
{
    return Utils::transform<HeaderPaths>(list, [type](const Utils::FilePath &fp) {
        return HeaderPath(fp, type);
    });
}

} // namespace ProjectExplorer
