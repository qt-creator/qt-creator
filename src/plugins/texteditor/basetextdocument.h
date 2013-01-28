/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASETEXTDOCUMENT_H
#define BASETEXTDOCUMENT_H

#include "texteditor_global.h"

#include <coreplugin/textdocument.h>

QT_BEGIN_NAMESPACE
class QTextCursor;
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class ITextMarkable;
class TypingSettings;
class StorageSettings;
class TabSettings;
class ExtraEncodingSettings;
class SyntaxHighlighter;
class BaseTextDocumentPrivate;

class TEXTEDITOR_EXPORT BaseTextDocument : public Core::TextDocument
{
    Q_OBJECT

public:
    BaseTextDocument();
    virtual ~BaseTextDocument();

    void setTypingSettings(const TypingSettings &typingSettings);
    void setStorageSettings(const StorageSettings &storageSettings);
    void setTabSettings(const TabSettings &tabSettings);
    void setExtraEncodingSettings(const ExtraEncodingSettings &extraEncodingSettings);

    const TypingSettings &typingSettings() const;
    const StorageSettings &storageSettings() const;
    const TabSettings &tabSettings() const;
    const ExtraEncodingSettings &extraEncodingSettings() const;

    ITextMarkable *documentMarker() const;

    // IDocument implementation.
    virtual bool save(QString *errorString, const QString &fileName, bool autoSave);
    virtual QString fileName() const;
    virtual bool shouldAutoSave() const;
    virtual bool isFileReadOnly() const;
    virtual bool isModified() const;
    virtual bool isSaveAsAllowed() const;
    virtual void checkPermissions();
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);
    virtual QString mimeType() const;
    void setMimeType(const QString &mt);
    virtual void rename(const QString &newName);

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;

    void setDefaultPath(const QString &defaultPath);
    void setSuggestedFileName(const QString &suggestedFileName);

    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName);
    virtual bool reload(QString *errorString);

    QTextDocument *document() const;
    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);
    SyntaxHighlighter *syntaxHighlighter() const;

    bool reload(QString *errorString, QTextCodec *codec);
    void cleanWhitespace(const QTextCursor &cursor);

    bool hasHighlightWarning() const;
    void setHighlightWarning(bool has);

signals:
    void titleChanged(QString title);

private:
    void cleanWhitespace(QTextCursor &cursor, bool cleanIndentation, bool inEntireDocument);
    void ensureFinalNewLine(QTextCursor &cursor);

    BaseTextDocumentPrivate *d;
};

} // namespace TextEditor

#endif // BASETEXTDOCUMENT_H
