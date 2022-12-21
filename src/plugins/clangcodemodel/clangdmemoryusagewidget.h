// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QCoreApplication>
#include <QWidget>

namespace ClangCodeModel::Internal {
class ClangdClient;

class ClangdMemoryUsageWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(ClangCodeModel)
public:
    explicit ClangdMemoryUsageWidget(ClangdClient *client);
    ~ClangdMemoryUsageWidget();

    class Private;
    Private * const d;
};

} // namespace ClangCodeModel::Internal

