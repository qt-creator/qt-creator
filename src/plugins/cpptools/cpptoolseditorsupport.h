/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CPPTOOLSEDITORSUPPORT_H
#define CPPTOOLSEDITORSUPPORT_H

#include <QObject>
#include <QPointer>
#include <QFuture>
#include <QSharedPointer>
#include <QTextCursor>
#include <cplusplus/CppDocument.h>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace CPlusPlus {
    class AST;
}

namespace TextEditor {
    class ITextEditor;
    class ITextMark;
} // end of namespace TextEditor

namespace CppTools {
namespace Internal {

class CppModelManager;

class QuickFixOperation;
typedef QSharedPointer<QuickFixOperation> QuickFixOperationPtr;

class QuickFixOperation
{
    Q_DISABLE_COPY(QuickFixOperation)

public:
    QuickFixOperation(CPlusPlus::Document::Ptr doc,
                      const CPlusPlus::Snapshot &snapshot);

    virtual ~QuickFixOperation();

    virtual QString description() const = 0;
    virtual void apply(QTextCursor cursor) = 0;

    CPlusPlus::Document::Ptr document() const { return _doc; }
    CPlusPlus::Snapshot snapshot() const { return _snapshot; }

    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor &tc);

protected:
    const CPlusPlus::Token &tokenAt(unsigned index) const;
    void getTokenStartPosition(unsigned index, unsigned *line,
                               unsigned *column) const;
    void getTokenEndPosition(unsigned index, unsigned *line,
                             unsigned *column) const;

    QTextCursor cursor(unsigned index) const;
    QTextCursor moveAtStartOfToken(unsigned index) const;
    QTextCursor moveAtEndOfToken(unsigned index) const;

private:
    CPlusPlus::Document::Ptr _doc;
    CPlusPlus::Snapshot _snapshot;
    QTextCursor _textCursor;
};

class CppEditorSupport: public QObject
{
    Q_OBJECT

public:
    CppEditorSupport(CppModelManager *modelManager);
    virtual ~CppEditorSupport();

    QList<QuickFixOperationPtr> quickFixes() const
    { return _quickFixes; }

    TextEditor::ITextEditor *textEditor() const;
    void setTextEditor(TextEditor::ITextEditor *textEditor);

    int updateDocumentInterval() const;
    void setUpdateDocumentInterval(int updateDocumentInterval);

    QString contents();

Q_SIGNALS:
    void contentsChanged();

private Q_SLOTS:
    void updateDocument();
    void updateDocumentNow();

    void checkDocument();
    void checkDocumentNow();

private:
    enum { UPDATE_DOCUMENT_DEFAULT_INTERVAL = 150 };

    CppModelManager *_modelManager;
    QPointer<TextEditor::ITextEditor> _textEditor;
    QTimer *_updateDocumentTimer;
    int _updateDocumentInterval;
    QFuture<void> _documentParser;
    QString _cachedContents;

    QTimer *_quickFixTimer;
    TextEditor::ITextMark *_quickFixMark;
    QList<QuickFixOperationPtr> _quickFixes;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPTOOLSEDITORSUPPORT_H
