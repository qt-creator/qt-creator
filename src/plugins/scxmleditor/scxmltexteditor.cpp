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
#include <QFileInfo>

using namespace ScxmlEditor;
using namespace ScxmlEditor::Internal;

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
            this, [this](QString *errorString, const QString &fileName) {
        open(errorString, fileName, fileName);
    });
}

bool ScxmlTextEditor::open(QString *errorString, const QString &fileName, const QString & /*realFileName*/)
{
    auto document = qobject_cast<ScxmlEditorDocument*>(textDocument());
    Common::MainWidget *designWidget = document->designWidget();
    QTC_ASSERT(designWidget, return false);

    if (fileName.isEmpty())
        return true;

    const QFileInfo fi(fileName);
    const QString absfileName = fi.absoluteFilePath();

    if (!designWidget->load(absfileName)) {
        *errorString = designWidget->errorMessage();
        return false;
    }

    document->syncXmlFromDesignWidget();
    document->setFilePath(Utils::FilePath::fromString(absfileName));

    return true;
}
