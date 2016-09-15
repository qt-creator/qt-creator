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
    ScxmlEditorDocument *document = qobject_cast<ScxmlEditorDocument*>(textDocument());
    connect(document, &ScxmlEditorDocument::reloadRequested,
        [this](QString *errorString, const QString &fileName) {
            open(errorString, fileName, fileName);
        });
}

bool ScxmlTextEditor::open(QString *errorString, const QString &fileName, const QString & /*realFileName*/)
{
    ScxmlEditorDocument *document = qobject_cast<ScxmlEditorDocument*>(textDocument());
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
    document->setFilePath(Utils::FileName::fromString(absfileName));

    return true;
}
