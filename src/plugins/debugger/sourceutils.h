// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

namespace TextEditor {
class TextDocument;
class TextEditorWidget;
}

namespace Utils { class FilePath; }
namespace CPlusPlus { class Snapshot; }

namespace Debugger::Internal {

class ContextData;
class Location;

// Editor tooltip support
QString cppExpressionAt(TextEditor::TextEditorWidget *editorWidget, int pos,
                        int *line, int *column, QString *function = nullptr,
                        int *scopeFromLine = nullptr, int *scopeToLine = nullptr);
QString fixCppExpression(const QString &exp);
QString cppFunctionAt(const Utils::FilePath &filePath, int line, int column = 0);

class RunToTarget
{
public:
    QString function;
    bool anyClass = false; // A virtual call, matched by the bare name.
    bool exact = false; // The name is complete, so no same-named function elsewhere matches.

    bool operator==(const RunToTarget &other) const = default;
};

QList<RunToTarget> functionCallTargets(const Utils::FilePath &filePath,
                                       const QByteArray &source, int lineNumber);
QList<RunToTarget> functionCallTargets(const CPlusPlus::Snapshot &snapshot,
                                       const Utils::FilePath &filePath,
                                       const QByteArray &source, int lineNumber);

// Get variables that are not initialized at a certain line
// of a function from the code model. Shadowed variables will
// be reported using the debugger naming conventions '<shadowed n>'
QStringList getUninitializedVariables(const CPlusPlus::Snapshot &snapshot,
                                      const QString &function, const Utils::FilePath &file, int line);

ContextData getLocationContext(TextEditor::TextDocument *document, int lineNumber);

} // Debugger::Internal
