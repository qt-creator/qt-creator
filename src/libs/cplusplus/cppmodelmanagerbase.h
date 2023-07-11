// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CppDocument.h>

namespace CPlusPlus::CppModelManagerBase {

CPLUSPLUS_EXPORT bool trySetExtraDiagnostics
    (const QString &, const QString &, const QList<Document::DiagnosticMessage> &);

CPLUSPLUS_EXPORT bool setSetExtraDiagnostics
    (const QString &, const QString &, const QList<Document::DiagnosticMessage> &);

CPLUSPLUS_EXPORT bool hasSnapshots();

CPLUSPLUS_EXPORT CPlusPlus::Snapshot snapshot();


// These callback are provided by the CppEditor plugin.

CPLUSPLUS_EXPORT void registerSnapshotCallback(CPlusPlus::Snapshot (*)(void));

CPLUSPLUS_EXPORT void registerSetExtraDiagnosticsCallback(
    bool(*)(const QString &, const QString &, const QList<Document::DiagnosticMessage> &));

} // CPlusPlus::CppModelManagerBase
