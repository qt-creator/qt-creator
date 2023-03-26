// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qobject.h>

namespace WebAssembly::Internal {

class WebAssemblyTest : public QObject
{
    Q_OBJECT

private slots:
    void testEmSdkEnvParsing();
    void testEmSdkEnvParsing_data();
    void testEmrunBrowserListParsing();
    void testEmrunBrowserListParsing_data();
};

} // WebAssembly::Internal
