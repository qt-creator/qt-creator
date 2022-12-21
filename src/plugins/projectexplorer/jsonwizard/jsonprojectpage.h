// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include <utils/projectintropage.h>

namespace ProjectExplorer {

// Documentation inside.
class PROJECTEXPLORER_EXPORT JsonProjectPage : public Utils::ProjectIntroPage
{
    Q_OBJECT

public:
    JsonProjectPage(QWidget *parent = nullptr);

    void initializePage() override;
    bool validatePage() override;

    static QString uniqueProjectName(const QString &path);
};

} // namespace ProjectExplorer
