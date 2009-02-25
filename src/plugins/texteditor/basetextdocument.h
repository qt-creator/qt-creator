/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef BASETEXTDOCUMENT_H
#define BASETEXTDOCUMENT_H

#include "texteditor_global.h"
#include "storagesettings.h"
#include "itexteditor.h"
#include "tabsettings.h"

QT_BEGIN_NAMESPACE
class QTextCursor;
class QTextDocument;
class QSyntaxHighlighter;
QT_END_NAMESPACE


namespace TextEditor {

class DocumentMarker : public ITextMarkable
{
    Q_OBJECT
public:
    DocumentMarker(QTextDocument *);

    // ITextMarkable
    bool addMark(ITextMark *mark, int line);
    TextMarks marksAt(int line) const;
    void removeMark(ITextMark *mark);
    bool hasMark(ITextMark *mark) const;
    void updateMark(ITextMark *mark);

private:
    QTextDocument *document;
};


class TEXTEDITOR_EXPORT BaseTextDocument : public Core::IFile
{
    Q_OBJECT

public:
    BaseTextDocument();
    ~BaseTextDocument();
    void setStorageSettings(const StorageSettings &storageSettings) { m_storageSettings = storageSettings; }
    void setTabSettings(const TabSettings &tabSettings) { m_tabSettings = tabSettings; }

    inline const StorageSettings &storageSettings() const { return m_storageSettings; }
    inline const TabSettings &tabSettings() const { return m_tabSettings; }

    DocumentMarker *documentMarker() const {return m_documentMarker; }

    //IFile
    virtual bool save(const QString &fileName = QString());
    virtual QString fileName() const { return m_fileName; }
    virtual bool isReadOnly() const;
    virtual bool isModified() const;
    virtual bool isSaveAsAllowed() const { return true; }
    virtual void modified(Core::IFile::ReloadBehavior *behavior);
    virtual QString mimeType() const;
    void setMimeType(const QString &mt);

    virtual QString defaultPath() const { return m_defaultPath; }
    virtual QString suggestedFileName() const { return m_suggestedFileName; }

    void setDefaultPath(const QString &defaultPath) {  m_defaultPath = defaultPath; }
    void setSuggestedFileName(const QString &suggestedFileName) { m_suggestedFileName = suggestedFileName; }

    virtual bool open(const QString &fileName = QString());
    virtual void reload();

    QTextDocument *document() const { return m_document; }
    void setSyntaxHighlighter(QSyntaxHighlighter *highlighter);
    QSyntaxHighlighter *syntaxHighlighter() const { return m_highlighter; }


    inline bool isBinaryData() const { return m_isBinaryData; }
    inline bool hasDecodingError() const { return m_hasDecodingError || m_isBinaryData; }
    inline QTextCodec *codec() const { return m_codec; }
    inline void setCodec(QTextCodec *c) { m_codec = c; }
    inline QByteArray decodingErrorSample() const { return m_decodingErrorSample; }

    void reload(QTextCodec *codec);

    void cleanWhitespace();

signals:
    void titleChanged(QString title);
    void changed();
    void aboutToReload();
    void reloaded();

private:
    QString m_fileName;
    QString m_defaultPath;
    QString m_suggestedFileName;
    QString m_mimeType;
    StorageSettings m_storageSettings;
    TabSettings m_tabSettings;
    QTextDocument *m_document;
    DocumentMarker *m_documentMarker;
    QSyntaxHighlighter *m_highlighter;

    enum LineTerminatorMode {
        LFLineTerminator,
        CRLFLineTerminator,
        NativeLineTerminator =
#if defined (Q_OS_WIN)
        CRLFLineTerminator
#else
        LFLineTerminator
#endif
    };
    LineTerminatorMode m_lineTerminatorMode;
    QTextCodec *m_codec;

    bool m_isBinaryData;
    bool m_hasDecodingError;
    QByteArray m_decodingErrorSample;

    void cleanWhitespace(QTextCursor& cursor, bool onlyInModifiedLines);
    void ensureFinalNewLine(QTextCursor& cursor);
};

} // namespace TextEditor

#endif // BASETEXTDOCUMENT_H
