/*
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_ANSIHIGHLIGHTER_H
#define KSYNTAXHIGHLIGHTING_ANSIHIGHLIGHTER_H

#include "abstracthighlighter.h"
#include "ksyntaxhighlighting_export.h"
#include "theme.h"

#include <QFlags>
#include <QString>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace KSyntaxHighlighting
{
class AnsiHighlighterPrivate;

// Exported for a bundled helper program
class KSYNTAXHIGHLIGHTING_EXPORT AnsiHighlighter final : public AbstractHighlighter
{
public:
    enum class AnsiFormat {
        TrueColor,
        XTerm256Color,
    };

    enum class Option {
        NoOptions,
        UseEditorBackground = 1 << 0,
        Unbuffered = 1 << 1,

        // Options that displays a useful visual aid for syntax creation
        TraceFormat = 1 << 2,
        TraceRegion = 1 << 3,
        TraceContext = 1 << 4,
        TraceStackSize = 1 << 5,
        TraceAll = TraceFormat | TraceRegion | TraceContext | TraceStackSize,
    };
    Q_DECLARE_FLAGS(Options, Option)

    AnsiHighlighter();
    ~AnsiHighlighter() override;

    void highlightFile(const QString &fileName, AnsiFormat format = AnsiFormat::TrueColor, Options options = Option::UseEditorBackground);
    void highlightData(QIODevice *device, AnsiFormat format = AnsiFormat::TrueColor, Options options = Option::UseEditorBackground);

    void setOutputFile(const QString &fileName);
    void setOutputFile(FILE *fileHandle);

    void setBackgroundRole(Theme::EditorColorRole bgRole);

protected:
    void applyFormat(int offset, int length, const Format &format) override;

private:
    Q_DECLARE_PRIVATE(AnsiHighlighter)
};
}

Q_DECLARE_OPERATORS_FOR_FLAGS(KSyntaxHighlighting::AnsiHighlighter::Options)

#endif // KSYNTAXHIGHLIGHTING_ANSIHIGHLIGHTER_H
