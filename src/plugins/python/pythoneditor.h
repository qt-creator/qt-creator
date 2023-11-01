// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

namespace Python::Internal {

class PythonEditorFactory : public TextEditor::TextEditorFactory
{
public:
    PythonEditorFactory();
private:
    QObject m_guard;
};

class PythonDocument : public TextEditor::TextDocument
{
    Q_OBJECT
public:
    PythonDocument();

    void updateCurrentPython();
    void updatePython(const Utils::FilePath &python);

signals:
    void pythonUpdated(const Utils::FilePath &python);
};

} // Python::Internal
