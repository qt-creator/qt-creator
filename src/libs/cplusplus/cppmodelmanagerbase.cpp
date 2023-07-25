// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppmodelmanagerbase.h"

#include <utils/filepath.h>
#include <utils/qtcassert.h>

using namespace Utils;

namespace CPlusPlus::CppModelManagerBase {

static bool (*setExtraDiagnosticsCallback)
    (const FilePath &, const QString &, const QList<Document::DiagnosticMessage> &) = nullptr;

static CPlusPlus::Snapshot (*snapshotCallback)() = nullptr;


bool trySetExtraDiagnostics(const FilePath &filePath, const QString &kind,
                            const QList<Document::DiagnosticMessage> &diagnostics)
{
    if (!setExtraDiagnosticsCallback)
        return false;
    return setExtraDiagnosticsCallback(filePath, kind, diagnostics);
}

bool setExtraDiagnostics(const FilePath &filePath, const QString &kind,
                         const QList<Document::DiagnosticMessage> &diagnostics)
{
    QTC_ASSERT(setExtraDiagnosticsCallback, return false);
    return setExtraDiagnosticsCallback(filePath, kind, diagnostics);
}

Snapshot snapshot()
{
    QTC_ASSERT(snapshotCallback, return {});
    return snapshotCallback();
}

bool hasSnapshots()
{
    return snapshotCallback;
}

// Installation

void registerSetExtraDiagnosticsCallback(
    bool (*callback)(const FilePath &, const QString &, const QList<Document::DiagnosticMessage> &))
{
    QTC_ASSERT(callback, return);
    QTC_CHECK(!setExtraDiagnosticsCallback); // bark when used twice
    setExtraDiagnosticsCallback = callback;
}

void registerSnapshotCallback(Snapshot (*callback)())
{
    QTC_ASSERT(callback, return);
    QTC_CHECK(!snapshotCallback); // bark when used twice
    snapshotCallback = callback;
}

} // CPlusPlus::CppModelManagerBase
