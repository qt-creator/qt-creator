// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "usercategory.h"

namespace QmlDesigner {

class ContentLibraryTexture;

class UserTextureCategory : public UserCategory
{
    Q_OBJECT

public:
    UserTextureCategory(const QString &title, const Utils::FilePath &bundlePath);

    void loadBundle(bool force) override;
    void filter(const QString &searchText) override;

    void addItems(const Utils::FilePaths &paths);
};

} // namespace QmlDesigner
