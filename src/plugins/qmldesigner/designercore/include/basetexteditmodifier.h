// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"
#include "plaintexteditmodifier.h"

#include <texteditor/texteditor.h>

#include <QStringList>

namespace QmlJS { class Snapshot; }

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT BaseTextEditModifier: public PlainTextEditModifier
{
public:
    BaseTextEditModifier(TextEditor::TextEditorWidget *textEdit);

    void indentLines(int startLine, int endLine) override;
    void indent(int offset, int length) override;

    TextEditor::TabSettings tabSettings() const override;

    bool renameId(const QString &oldId, const QString &newId) override;
    bool moveToComponent(int nodeOffset, const QString &importData) override;
    QStringList autoComplete(QTextDocument *textDocument, int position, bool explicitComplete) override;

private:
    TextEditor::TextEditorWidget *m_textEdit;
};

} // namespace QmlDesigner
