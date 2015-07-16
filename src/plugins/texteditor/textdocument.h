/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASETEXTDOCUMENT_H
#define BASETEXTDOCUMENT_H

#include "texteditor_global.h"

#include <coreplugin/id.h>
#include <coreplugin/textdocument.h>

#include <QList>
#include <QMap>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QTextCursor;
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class CompletionAssistProvider;
class ExtraEncodingSettings;
class FontSettings;
class Indenter;
class StorageSettings;
class SyntaxHighlighter;
class TabSettings;
class TextDocumentPrivate;
class TextMark;
class TypingSettings;

typedef QList<TextMark *> TextMarks;

class TEXTEDITOR_EXPORT TextDocument : public Core::BaseTextDocument
{
    Q_OBJECT

public:
    explicit TextDocument(Core::Id id = Core::Id());
    ~TextDocument() override;

    static QMap<QString, QString> openedTextDocumentContents();
    static QMap<QString, QTextCodec *> openedTextDocumentEncodings();

    virtual QString plainText() const;
    virtual QString textAt(int pos, int length) const;
    virtual QChar characterAt(int pos) const;

    void setTypingSettings(const TypingSettings &typingSettings);
    void setStorageSettings(const StorageSettings &storageSettings);
    void setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings);

    const TypingSettings &typingSettings() const;
    const StorageSettings &storageSettings() const;
    const TabSettings &tabSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;
    const FontSettings &fontSettings() const;

    void setIndenter(Indenter *indenter);
    Indenter *indenter() const;
    void autoIndent(const QTextCursor &cursor, QChar typedChar = QChar::Null);
    void autoReindent(const QTextCursor &cursor);
    QTextCursor indent(const QTextCursor &cursor, bool blockSelection = false, int column = 0,
                       int *offset = 0);
    QTextCursor unindent(const QTextCursor &cursor, bool blockSelection = false, int column = 0,
                         int *offset = 0);

    TextMarks marks() const;
    bool addMark(TextMark *mark);
    TextMarks marksAt(int line) const;
    void removeMark(TextMark *mark);
    void updateMark(TextMark *mark);
    void moveMark(TextMark *mark, int previousLine);
    void removeMarkFromMarksCache(TextMark *mark);

    // IDocument implementation.
    bool save(QString *errorString, const QString &fileName, bool autoSave) override;
    bool setContents(const QByteArray &contents) override;
    bool shouldAutoSave() const override;
    bool isFileReadOnly() const override;
    bool isModified() const override;
    bool isSaveAsAllowed() const override;
    void checkPermissions() override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;
    void setFilePath(const Utils::FileName &newName) override;

    QString defaultPath() const override;
    QString suggestedFileName() const override;

    void setDefaultPath(const QString &defaultPath);
    void setSuggestedFileName(const QString &suggestedFileName);

    OpenResult open(QString *errorString, const QString &fileName,
                    const QString &realFileName) override;
    virtual bool reload(QString *errorString);

    bool setPlainText(const QString &text);
    QTextDocument *document() const;
    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);
    SyntaxHighlighter *syntaxHighlighter() const;

    bool reload(QString *errorString, QTextCodec *codec);
    void cleanWhitespace(const QTextCursor &cursor);

    virtual void triggerPendingUpdates();

    void setCompletionAssistProvider(CompletionAssistProvider *provider);
    virtual CompletionAssistProvider *completionAssistProvider() const;

public slots:
    void setTabSettings(const TextEditor::TabSettings &tabSettings);
    void setFontSettings(const TextEditor::FontSettings &fontSettings);

signals:
    void aboutToOpen(const QString &fileName, const QString &realFileName);
    void openFinishedSuccessfully();
    void contentsChanged();
    void tabSettingsChanged();
    void fontSettingsChanged();

protected slots:
    virtual void applyFontSettings();

private:
    OpenResult openImpl(QString *errorString, const QString &fileName, const QString &realFileName,
                        bool reload);
    void cleanWhitespace(QTextCursor &cursor, bool cleanIndentation, bool inEntireDocument);
    void ensureFinalNewLine(QTextCursor &cursor);

    TextDocumentPrivate *d;
};

typedef QSharedPointer<TextDocument> TextDocumentPtr;

} // namespace TextEditor

#endif // BASETEXTDOCUMENT_H
