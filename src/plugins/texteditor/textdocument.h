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

#pragma once

#include "texteditor_global.h"
#include "indenter.h"

#include <coreplugin/id.h>
#include <coreplugin/textdocument.h>
#include <utils/link.h>

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
class TypingSettings;

using TextMarks = QList<TextMark *>;

class TEXTEDITOR_EXPORT TextDocument : public Core::BaseTextDocument
{
    Q_OBJECT

public:
    explicit TextDocument(Core::Id id = Core::Id());
    ~TextDocument() override;

    static QMap<QString, QString> openedTextDocumentContents();
    static QMap<QString, QTextCodec *> openedTextDocumentEncodings();
    static TextDocument *currentTextDocument();
    static TextDocument *textDocumentForFileName(const Utils::FileName &fileName);

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
    QTextCursor indent(const QTextCursor &cursor, bool blockSelection = false, int column = 0,
                       int *offset = nullptr);
    QTextCursor unindent(const QTextCursor &cursor, bool blockSelection = false, int column = 0,
                         int *offset = nullptr);

    TextMarks marks() const;
    bool addMark(TextMark *mark);
    TextMarks marksAt(int line) const;
    void removeMark(TextMark *mark);
    void updateMark(TextMark *mark);
    void moveMark(TextMark *mark, int previousLine);
    void removeMarkFromMarksCache(TextMark *mark);

    // IDocument implementation.
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    QByteArray contents() const override;
    bool setContents(const QByteArray &contents) override;
    bool shouldAutoSave() const override;
    bool isFileReadOnly() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    void checkPermissions() override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    void setFilePath(const Utils::FileName &newName) override;

    QString fallbackSaveAsPath() const override;
    QString fallbackSaveAsFileName() const override;

    void setFallbackSaveAsPath(const QString &fallbackSaveAsPath);
    void setFallbackSaveAsFileName(const QString &fallbackSaveAsFileName);

    OpenResult open(QString *errorString, const QString &fileName,
                    const QString &realFileName) override;
    virtual bool reload(QString *errorString);
    bool reload(QString *errorString, const QString &realFileName);

    bool setPlainText(const QString &text);
    QTextDocument *document() const;
    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);
    SyntaxHighlighter *syntaxHighlighter() const;

    bool reload(QString *errorString, QTextCodec *codec);
    void cleanWhitespace(const QTextCursor &cursor);

    virtual void triggerPendingUpdates();

    void setCompletionAssistProvider(CompletionAssistProvider *provider);
    virtual CompletionAssistProvider *completionAssistProvider() const;
    void setQuickFixAssistProvider(IAssistProvider *provider) const;
    virtual IAssistProvider *quickFixAssistProvider() const;

    void setTabSettings(const TextEditor::TabSettings &tabSettings);
    void setFontSettings(const TextEditor::FontSettings &fontSettings);

    static QAction *createDiffAgainstCurrentFileAction(QObject *parent,
        const std::function<Utils::FileName()> &filePath);

signals:
    void aboutToOpen(const QString &fileName, const QString &realFileName);
    void openFinishedSuccessfully();
    void contentsChangedWithPosition(int position, int charsRemoved, int charsAdded);
    void tabSettingsChanged();
    void fontSettingsChanged();
    void markRemoved(TextMark *mark);

protected:
    virtual void applyFontSettings();

private:
    OpenResult openImpl(QString *errorString, const QString &fileName, const QString &realFileName,
                        bool reload);
    void cleanWhitespace(QTextCursor &cursor, bool cleanIndentation, bool inEntireDocument);
    void ensureFinalNewLine(QTextCursor &cursor);
    void modificationChanged(bool modified);
    void updateLayout() const;

    TextDocumentPrivate *d;
};

using TextDocumentPtr = QSharedPointer<TextDocument>;

} // namespace TextEditor
