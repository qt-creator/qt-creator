// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scxmltexteditor.h"
#include "mainwidget.h"
#include "scxmleditorconstants.h"
#include "scxmleditordocument.h"

#include <coreplugin/coreconstants.h>
#include <texteditor/textdocument.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QBuffer>

using namespace ScxmlEditor::Internal;
using namespace Utils;

namespace ScxmlEditor {

ScxmlTextEditor::ScxmlTextEditor()
{
    addContext(ScxmlEditor::Constants::K_SCXML_EDITOR_ID);
    addContext(ScxmlEditor::Constants::C_SCXML_EDITOR);
}

void ScxmlTextEditor::finalizeInitialization()
{
    // Revert to saved/load externally modified files.
    auto document = qobject_cast<const ScxmlEditorDocument*>(textDocument());
    connect(document, &ScxmlEditorDocument::reloadRequested,
            this, [this](QString *errorString, const FilePath &filePath) {
        open(errorString, filePath);
    });
}

bool ScxmlTextEditor::open(QString *errorString, const FilePath &filePath)
{
    auto document = qobject_cast<ScxmlEditorDocument*>(textDocument());
    Common::MainWidget *designWidget = document->designWidget();
    QTC_ASSERT(designWidget, return false);

    if (filePath.isEmpty())
        return true;

    const FilePath absfilePath = filePath.absoluteFilePath();

    if (!designWidget->load(absfilePath)) {
        *errorString = designWidget->errorMessage();
        return false;
    }

    document->syncXmlFromDesignWidget();
    document->setFilePath(absfilePath);

    return true;
}

} // ScxmlEditor
