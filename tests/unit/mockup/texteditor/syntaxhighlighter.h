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

#include <texteditor/texteditorconstants.h>

#include <QObject>
#include <QTextBlock>
#include <QTextLayout>

#include <functional>
#include <limits.h>

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

class SyntaxHighlighter : public QObject
{
public:
    SyntaxHighlighter(QObject *parent = 0) {}
    SyntaxHighlighter(QTextDocument *parent) {}
    SyntaxHighlighter(QTextEdit *parent) {}

    void setDocument(QTextDocument *doc) { m_document = doc; }
    QTextDocument *document() const { return m_document; }

    void setExtraFormats(const QTextBlock &block, QVector<QTextLayout::FormatRange> &formats) {}

    static QList<QColor> generateColors(int n, const QColor &background)
    {
        return QList<QColor>();
    }

    // Don't call in constructors of derived classes
    virtual void setFontSettings(const TextEditor::FontSettings &fontSettings) {}

    void setNoAutomaticHighlighting(bool noAutomatic) {}

    void rehighlight() {}
    void rehighlightBlock(const QTextBlock &block) {}

protected:
    void setDefaultTextFormatCategories() {}
    void setTextFormatCategories(int count, std::function<TextStyle(int)> formatMapping) {}
    QTextCharFormat formatForCategory(int categoryIndex) const { return QTextCharFormat(); }
    virtual void highlightBlock(const QString &text) {}

    void setFormat(int start, int count, const QTextCharFormat &format) {}
    void setFormat(int start, int count, const QColor &color) {}
    void setFormat(int start, int count, const QFont &font) {}
    QTextCharFormat format(int pos) const { return QTextCharFormat(); }

    void formatSpaces(const QString &text, int start = 0, int count = INT_MAX) {}
    void setFormatWithSpaces(const QString &text, int start, int count,
                             const QTextCharFormat &format) {}

    int previousBlockState() const { return -1; }
    int currentBlockState() const { return -1; }
    void setCurrentBlockState(int newState) {}

    void setCurrentBlockUserData(QTextBlockUserData *data) {}
    QTextBlockUserData *currentBlockUserData() const { return nullptr; }

    QTextBlock currentBlock() const { return QTextBlock(); }

private:
    void setTextFormatCategories(const QVector<std::pair<int, TextStyle>> &categories) {}
    void reformatBlocks(int from, int charsRemoved, int charsAdded) {}
    void delayedRehighlight() {}

    QTextDocument *m_document = nullptr;
};

} // namespace TextEditor
