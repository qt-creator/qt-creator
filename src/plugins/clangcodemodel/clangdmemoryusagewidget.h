// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace ClangCodeModel::Internal {

class ClangdClient;

class ClangdMemoryUsageWidget : public QWidget
{
public:
    explicit ClangdMemoryUsageWidget(ClangdClient *client);
    ~ClangdMemoryUsageWidget();

    class Private;
    Private * const d;
};

} // ClangCodeModel::Internal

