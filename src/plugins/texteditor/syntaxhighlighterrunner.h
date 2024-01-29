// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/fontsettings.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/texteditorsettings.h>

#include <QObject>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class SyntaxHighlighterRunnerPrivate;

class TEXTEDITOR_EXPORT SyntaxHighlighterRunner : public QObject
{
    Q_OBJECT
public:
    using SyntaxHighlighterCreator = std::function<SyntaxHighlighter *()>;

    SyntaxHighlighterRunner(
        SyntaxHighlighterCreator creator,
        QTextDocument *document,
        bool async,
        const QString &mimeType,
        const TextEditor::FontSettings &fontSettings = TextEditorSettings::fontSettings());
    virtual ~SyntaxHighlighterRunner();

    void setExtraFormats(const QMap<int, QList<QTextLayout::FormatRange>> &formats);
    void clearExtraFormats(const QList<int> &blockNumbers);
    void clearAllExtraFormats();
    void setFontSettings(const TextEditor::FontSettings &fontSettings);
    void setLanguageFeaturesFlags(unsigned int flags);
    void setEnabled(bool enabled);
    void rehighlight();

    QString definitionName();
    void setDefinitionName(const QString &name);

    QTextDocument *document() const { return m_document; }
    bool useGenericHighlighter() const;

    bool syntaxInfoUpdated() const { return m_syntaxInfoUpdated == SyntaxHighlighter::State::Done; }

signals:
    void highlightingFinished();

private:
    void applyFormatRanges(const QList<SyntaxHighlighter::Result> &results);
    void changeDocument(int from, int charsRemoved, int charsAdded);

    SyntaxHighlighterRunnerPrivate *d;
    QPointer<QTextDocument> m_document = nullptr;
    SyntaxHighlighter::State m_syntaxInfoUpdated = SyntaxHighlighter::State::Done;
    bool m_useGenericHighlighter = false;
    QString m_definitionName;
    std::optional<QThread> m_thread;
};

} // namespace TextEditor

