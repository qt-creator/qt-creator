// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/syntaxhighlighter.h>

#include <KSyntaxHighlighting/Definition>

#include <QObject>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class SyntaxHighlighterRunnerPrivate;

class TEXTEDITOR_EXPORT BaseSyntaxHighlighterRunner : public QObject
{
    Q_OBJECT
public:
    using SyntaxHighLighterCreator = std::function<SyntaxHighlighter *()>;
    struct BlockPreeditData {
        int position;
        QString text;
    };

    BaseSyntaxHighlighterRunner(SyntaxHighLighterCreator creator, QTextDocument *document);
    virtual ~BaseSyntaxHighlighterRunner();

    void setExtraFormats(const QMap<int, QList<QTextLayout::FormatRange>> &formats);
    void clearExtraFormats(const QList<int> &blockNumbers);
    void clearAllExtraFormats();
    void setFontSettings(const TextEditor::FontSettings &fontSettings);
    void setLanguageFeaturesFlags(unsigned int flags);
    void setEnabled(bool enabled);
    void rehighlight();

    virtual KSyntaxHighlighting::Definition getDefinition();

    QTextDocument *document() const { return m_document; }
    SyntaxHighLighterCreator creator() const { return m_creator; }

protected:
    std::unique_ptr<SyntaxHighlighterRunnerPrivate> d;
    QPointer<QTextDocument> m_document = nullptr;
    void applyFormatRanges(const SyntaxHighlighter::Result &result);
    void cloneDocumentData(int from, int charsRemoved, int charsAdded);
    void cloneDocument(int from,
                       int charsRemoved,
                       const QString textAdded,
                       const QMap<int, BlockPreeditData> &blocksPreedit);

private:
    SyntaxHighLighterCreator m_creator;
};

class TEXTEDITOR_EXPORT ThreadedSyntaxHighlighterRunner : public BaseSyntaxHighlighterRunner
{
public:
    ThreadedSyntaxHighlighterRunner(SyntaxHighLighterCreator SyntaxHighLighterCreator,
                                    QTextDocument *document);
    ~ThreadedSyntaxHighlighterRunner();

    KSyntaxHighlighting::Definition getDefinition() override;

private:
    QThread m_thread;
};

} // namespace TextEditor

