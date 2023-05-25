// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"
#include "blockrange.h"
#include "formatter.h"
#include "indenter.h"

#include <coreplugin/textdocument.h>

#include <utils/id.h>
#include <utils/link.h>
#include <utils/multitextcursor.h>

#include <QList>
#include <QMap>
#include <QSharedPointer>

#include <functional>

QT_BEGIN_NAMESPACE
class QAction;
class QTextCursor;
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class CompletionAssistProvider;
class ExtraEncodingSettings;
class FontSettings;
class IAssistProvider;
class StorageSettings;
class SyntaxHighlighter;
class TabSettings;
class TextDocumentPrivate;
class TextMark;
class TextSuggestion;
class TypingSettings;

using TextMarks = QList<TextMark *>;

class TEXTEDITOR_EXPORT TextDocument : public Core::BaseTextDocument
{
    Q_OBJECT

public:
    explicit TextDocument(Utils::Id id = Utils::Id());
    ~TextDocument() override;

    static QMap<Utils::FilePath, QString> openedTextDocumentContents();
    static QMap<Utils::FilePath, QTextCodec *> openedTextDocumentEncodings();
    static TextDocument *currentTextDocument();
    static TextDocument *textDocumentForFilePath(const Utils::FilePath &filePath);
    static QString convertToPlainText(const QString &rawText);

    virtual QString plainText() const;
    virtual QString textAt(int pos, int length) const;
    virtual QChar characterAt(int pos) const;

    void setTypingSettings(const TypingSettings &typingSettings);
    void setStorageSettings(const StorageSettings &storageSettings);
    void setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings);

    const TypingSettings &typingSettings() const;
    const StorageSettings &storageSettings() const;
    virtual TabSettings tabSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;
    const FontSettings &fontSettings() const;

    void setIndenter(Indenter *indenter);
    Indenter *indenter() const;
    void autoIndent(const QTextCursor &cursor,
                    QChar typedChar = QChar::Null,
                    int currentCursorPosition = -1);
    void autoReindent(const QTextCursor &cursor, int currentCursorPosition = -1);
    void autoFormatOrIndent(const QTextCursor &cursor);
    Utils::MultiTextCursor indent(const Utils::MultiTextCursor &cursor);
    Utils::MultiTextCursor unindent(const Utils::MultiTextCursor &cursor);

    void setFormatter(Formatter *indenter); // transfers ownership
    void autoFormat(const QTextCursor &cursor);
    bool applyChangeSet(const Utils::ChangeSet &changeSet);

    // the blocks list must be sorted
    void setIfdefedOutBlocks(const QList<BlockRange> &blocks);

    TextMarks marks() const;
    bool addMark(TextMark *mark);
    TextMarks marksAt(int line) const;
    void removeMark(TextMark *mark);
    void updateLayout() const;
    void scheduleUpdateLayout() const;
    void updateMark(TextMark *mark);
    void moveMark(TextMark *mark, int previousLine);
    void removeMarkFromMarksCache(TextMark *mark);
    static void temporaryHideMarksAnnotation(const Utils::Id &category);
    static void showMarksAnnotation(const Utils::Id &category);
    static bool marksAnnotationHidden(const Utils::Id &category);

    // IDocument implementation.
    QByteArray contents() const override;
    bool setContents(const QByteArray &contents) override;
    void formatContents() override;
    bool shouldAutoSave() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    void setFilePath(const Utils::FilePath &newName) override;
    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;

    Utils::FilePath fallbackSaveAsPath() const override;
    QString fallbackSaveAsFileName() const override;

    void setFallbackSaveAsPath(const Utils::FilePath &fallbackSaveAsPath);
    void setFallbackSaveAsFileName(const QString &fallbackSaveAsFileName);

    OpenResult open(QString *errorString, const Utils::FilePath &filePath,
                    const Utils::FilePath &realFilePath) override;
    virtual bool reload(QString *errorString);
    bool reload(QString *errorString, const Utils::FilePath &realFilePath);

    bool setPlainText(const QString &text);
    QTextDocument *document() const;
    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);
    SyntaxHighlighter *syntaxHighlighter() const;

    bool reload(QString *errorString, QTextCodec *codec);
    void cleanWhitespace(const QTextCursor &cursor);

    virtual void triggerPendingUpdates();

    virtual void setCompletionAssistProvider(CompletionAssistProvider *provider);
    virtual CompletionAssistProvider *completionAssistProvider() const;
    virtual void setFunctionHintAssistProvider(CompletionAssistProvider *provider);
    virtual CompletionAssistProvider *functionHintAssistProvider() const;
    void setQuickFixAssistProvider(IAssistProvider *provider) const;
    virtual IAssistProvider *quickFixAssistProvider() const;

    void setTabSettings(const TextEditor::TabSettings &tabSettings);
    void setFontSettings(const TextEditor::FontSettings &fontSettings);

    static QAction *createDiffAgainstCurrentFileAction(QObject *parent,
        const std::function<Utils::FilePath()> &filePath);

    void insertSuggestion(const QString &text, const QTextCursor &cursor);
    void insertSuggestion(std::unique_ptr<TextSuggestion> &&suggestion);

#ifdef WITH_TESTS
    void setSilentReload();
#endif

signals:
    void aboutToOpen(const Utils::FilePath &filePath, const Utils::FilePath &realFilePath);
    void openFinishedSuccessfully();
    void contentsChangedWithPosition(int position, int charsRemoved, int charsAdded);
    void tabSettingsChanged();
    void fontSettingsChanged();
    void markRemoved(TextMark *mark);

#ifdef WITH_TESTS
    void ifdefedOutBlocksChanged(const QList<BlockRange> &blocks);
#endif

protected:
    virtual void applyFontSettings();
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) override;

private:
    OpenResult openImpl(QString *errorString,
                        const Utils::FilePath &filePath,
                        const Utils::FilePath &realFileName,
                        bool reload);
    void cleanWhitespace(QTextCursor &cursor, bool inEntireDocument, bool cleanIndentation);
    void ensureFinalNewLine(QTextCursor &cursor);
    void modificationChanged(bool modified);

    TextDocumentPrivate *d;
};

using TextDocumentPtr = QSharedPointer<TextDocument>;

} // namespace TextEditor
