/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cpptools_global.h"

#include <texteditor/semantichighlighter.h>

#include <cplusplus/CppDocument.h>

#include <QHash>

namespace CppTools {

class CPPTOOLS_EXPORT SemanticInfo
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
    using LocalUseIterator = QHashIterator<CPlusPlus::Symbol *, QList<Use>>;

    // Document specific
    unsigned revision = 0;
    bool complete = true;
    CPlusPlus::Snapshot snapshot;
    CPlusPlus::Document::Ptr doc;

    // Widget specific (e.g. related to cursor position)
    bool localUsesUpdated = false;
    LocalUseMap localUses;
};

} // namespace CppTools
