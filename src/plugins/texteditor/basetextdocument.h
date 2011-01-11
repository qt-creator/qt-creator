/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BASETEXTDOCUMENT_H
#define BASETEXTDOCUMENT_H

#include "texteditor_global.h"

#include <coreplugin/ifile.h>

QT_BEGIN_NAMESPACE
class QTextCursor;
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {
namespace Internal {
class DocumentMarker;
}

class ITextMarkable;
class StorageSettings;
class TabSettings;
class SyntaxHighlighter;
class BaseTextDocumentPrivate;

class TEXTEDITOR_EXPORT BaseTextDocument : public Core::IFile
{
    Q_OBJECT

public:
    BaseTextDocument();
    virtual ~BaseTextDocument();

    void setStorageSettings(const StorageSettings &storageSettings);
    void setTabSettings(const TabSettings &tabSettings);

    const StorageSettings &storageSettings() const;
    const TabSettings &tabSettings() const;

    ITextMarkable *documentMarker() const;

    //IFile
    virtual bool save(const QString &fileName = QString());
    virtual QString fileName() const;
    virtual bool isReadOnly() const;
    virtual bool isModified() const;
    virtual bool isSaveAsAllowed() const;
    virtual void checkPermissions();
    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    void reload(ReloadFlag flag, ChangeType type);
    virtual QString mimeType() const;
    void setMimeType(const QString &mt);
    virtual void rename(const QString &newName);

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;

    void setDefaultPath(const QString &defaultPath);
    void setSuggestedFileName(const QString &suggestedFileName);

    virtual bool open(const QString &fileName = QString());
    virtual void reload();

    QTextDocument *document() const;
    void setSyntaxHighlighter(SyntaxHighlighter *highlighter);
    SyntaxHighlighter *syntaxHighlighter() const;


    bool isBinaryData() const;
    bool hasDecodingError() const;
    QTextCodec *codec() const;
    void setCodec(QTextCodec *c);
    QByteArray decodingErrorSample() const;

    void reload(QTextCodec *codec);

    void cleanWhitespace(const QTextCursor &cursor);

signals:
    void titleChanged(QString title);

private:
    void cleanWhitespace(QTextCursor& cursor, bool cleanIndentation, bool inEntireDocument);
    void ensureFinalNewLine(QTextCursor& cursor);
    void documentClosing();

    BaseTextDocumentPrivate *d;
};

} // namespace TextEditor

#endif // BASETEXTDOCUMENT_H
