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

#ifndef TEXTEDITOR_SYNTAXHIGHLIGHTER_H
#define TEXTEDITOR_SYNTAXHIGHLIGHTER_H

#include "texteditor_global.h"

#include <QObject>
#include <QTextLayout>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QSyntaxHighlighterPrivate;
class QTextCharFormat;
class QFont;
class QColor;
class QTextBlockUserData;
class QTextEdit;
QT_END_NAMESPACE

namespace TextEditor {

class BaseTextDocument;
class SyntaxHighlighterPrivate;

class TEXTEDITOR_EXPORT SyntaxHighlighter : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(SyntaxHighlighter)
public:
    SyntaxHighlighter(QObject *parent);
    SyntaxHighlighter(QTextDocument *parent);
    SyntaxHighlighter(BaseTextDocument *parent);
    SyntaxHighlighter(QTextEdit *parent);
    virtual ~SyntaxHighlighter();

    void setDocument(QTextDocument *doc);
    QTextDocument *document() const;

    void setExtraAdditionalFormats(const QTextBlock& block, QList<QTextLayout::FormatRange> &formats);

    static QList<QColor> generateColors(int n, const QColor &background);

public Q_SLOTS:
    void rehighlight();
    void rehighlightBlock(const QTextBlock &block);

protected:
    virtual void highlightBlock(const QString &text) = 0;

    void setFormat(int start, int count, const QTextCharFormat &format);
    void setFormat(int start, int count, const QColor &color);
    void setFormat(int start, int count, const QFont &font);
    QTextCharFormat format(int pos) const;

    void applyFormatToSpaces(const QString &text, const QTextCharFormat &format);

    int previousBlockState() const;
    int currentBlockState() const;
    void setCurrentBlockState(int newState);

    void setCurrentBlockUserData(QTextBlockUserData *data);
    QTextBlockUserData *currentBlockUserData() const;

    QTextBlock currentBlock() const;

private:
    Q_PRIVATE_SLOT(d_ptr, void _q_reformatBlocks(int from, int charsRemoved, int charsAdded))
    Q_PRIVATE_SLOT(d_ptr, void _q_delayedRehighlight())

    QScopedPointer<SyntaxHighlighterPrivate> d_ptr;
};

} // namespace TextEditor

#endif // TEXTEDITOR_SYNTAXHIGHLIGHTER_H
