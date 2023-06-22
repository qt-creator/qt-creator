// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textdocument.h>

namespace Android {
namespace Internal {

class AndroidManifestEditorWidget;

class AndroidManifestDocument : public TextEditor::TextDocument
{
public:
    explicit AndroidManifestDocument(AndroidManifestEditorWidget *editorWidget);

    bool isModified() const override;
    bool isSaveAsAllowed() const override;

protected:
    bool saveImpl(QString *errorString,
                  const Utils::FilePath &filePath,
                  bool autoSave = false) override;

private:
    AndroidManifestEditorWidget *m_editorWidget;
};

} // namespace Internal
} // namespace Android
