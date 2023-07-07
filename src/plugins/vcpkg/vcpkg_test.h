// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/aspects.h>

namespace Vcpkg::Internal {

class VcpkgSearchTest : public QObject
{
    Q_OBJECT

public:
    VcpkgSearchTest(QObject *parent = nullptr);
    ~VcpkgSearchTest();

private slots:
    void testVcpkgJsonParser_data();
    void testVcpkgJsonParser();
    void testAddDependency_data();
    void testAddDependency();
};

} // namespace Vcpkg::Internal
