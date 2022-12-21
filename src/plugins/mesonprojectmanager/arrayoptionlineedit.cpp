// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "arrayoptionlineedit.h"

namespace MesonProjectManager {
namespace Internal {

ArrayOptionLineEdit::ArrayOptionLineEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_highLighter = new RegexHighlighter(this);
    m_highLighter->setDocument(this->document());
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    QFontMetrics metrics(this->font());
    int lineHeight = metrics.lineSpacing();
    this->setFixedHeight(lineHeight * 1.5);
}

QStringList ArrayOptionLineEdit::options()
{
    return m_highLighter->options(this->toPlainText());
}

void ArrayOptionLineEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() != Qt::Key_Return)
        return QPlainTextEdit::keyPressEvent(e);
    e->accept();
}

RegexHighlighter::RegexHighlighter(QWidget *parent)
    : QSyntaxHighlighter(parent)
{
    m_format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
    m_format.setUnderlineColor(QColor(180, 180, 180));
    m_format.setBackground(QBrush(QColor(180, 180, 230, 80)));
}

QStringList RegexHighlighter::options(const QString &text)
{
    QRegularExpressionMatchIterator i = m_regex.globalMatch(text);
    QStringList op;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        for (int j = 1; j <= match.lastCapturedIndex(); j++) {
            auto str = match.captured(j);
            if (!str.isEmpty())
                op.push_back(str);
        }
    }
    return op;
}

void RegexHighlighter::highlightBlock(const QString &text)
{
    QRegularExpressionMatchIterator i = m_regex.globalMatch(text);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        for (int j = 1; j <= match.lastCapturedIndex(); j++) {
            setFormat(match.capturedStart(j), match.capturedLength(j), m_format);
        }
    }
}

} // namespace Internal
} // namespace MesonProjectManager
