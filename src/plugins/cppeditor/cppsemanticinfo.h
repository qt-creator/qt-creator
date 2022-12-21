// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/semantichighlighter.h>

#include <cplusplus/CppDocument.h>

#include <QHash>

namespace CppEditor {

class CPPEDITOR_EXPORT SemanticInfo
{
public:
    struct Source
    {
        const QString fileName;
        const QByteArray code;
        const unsigned revision = 0;
        CPlusPlus::Snapshot snapshot;
        const bool force = false;

        Source(const QString &fileName,
               const QByteArray &code,
               unsigned revision,
               const CPlusPlus::Snapshot &snapshot,
               bool force)
            : fileName(fileName)
            , code(code)
            , revision(revision)
            , snapshot(snapshot)
            , force(force)
        {}
    };

public:
    using Use = TextEditor::HighlightingResult;
    using LocalUseMap = QHash<CPlusPlus::Symbol *, QList<Use>>;

    // Document specific
    unsigned revision = 0;
    bool complete = true;
    CPlusPlus::Snapshot snapshot;
    CPlusPlus::Document::Ptr doc;

    // Widget specific (e.g. related to cursor position)
    bool localUsesUpdated = false;
    LocalUseMap localUses;
};

} // namespace CppEditor
