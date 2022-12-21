// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QDialog>
#include <QString>

namespace TextEditor { class SnippetEditorWidget; }

namespace CppEditor::Internal {

class CppPreProcessorDialog : public QDialog
{
    Q_OBJECT

public:
    CppPreProcessorDialog(const Utils::FilePath &filePath, QWidget *parent);
    ~CppPreProcessorDialog() override;

    int exec() override;

    QString extraPreprocessorDirectives() const;

private:
    const Utils::FilePath m_filePath;
    const QString m_projectPartId;

    TextEditor::SnippetEditorWidget *m_editWidget;
};

} // CppEditor::Internal
