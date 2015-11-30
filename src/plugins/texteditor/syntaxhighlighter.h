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

#ifndef TEXTEDITOR_SYNTAXHIGHLIGHTER_H
#define TEXTEDITOR_SYNTAXHIGHLIGHTER_H

#include "texteditor_global.h"
#include <texteditor/texteditorconstants.h>
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

class FontSettings;
class SyntaxHighlighterPrivate;

class TEXTEDITOR_EXPORT SyntaxHighlighter : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(SyntaxHighlighter)
public:
    SyntaxHighlighter(QObject *parent = 0);
    SyntaxHighlighter(QTextDocument *parent);
    SyntaxHighlighter(QTextEdit *parent);
    virtual ~SyntaxHighlighter();

    void setDocument(QTextDocument *doc);
    QTextDocument *document() const;

    void setExtraAdditionalFormats(const QTextBlock& block, QList<QTextLayout::FormatRange> &formats);

    static QList<QColor> generateColors(int n, const QColor &background);

    // Don't call in constructors of derived classes
    virtual void setFontSettings(const TextEditor::FontSettings &fontSettings);
public Q_SLOTS:
    void rehighlight();
    void rehighlightBlock(const QTextBlock &block);

protected:
    void setTextFormatCategories(const QVector<TextEditor::TextStyle> &categories);
    QTextCharFormat formatForCategory(int categoryIndex) const;
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
