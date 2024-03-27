// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffeditorfactory.h"
#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditortr.h"

#include <QCoreApplication>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor::Internal {

DiffEditorFactory::DiffEditorFactory()
{
    setId(Constants::DIFF_EDITOR_ID);
    setDisplayName(Tr::tr("Diff Editor"));
    addMimeType(Constants::DIFF_EDITOR_MIMETYPE);
    setEditorCreator([] { return new DiffEditor(new DiffEditorDocument); });
}

} // namespace DiffEditor::Internal
