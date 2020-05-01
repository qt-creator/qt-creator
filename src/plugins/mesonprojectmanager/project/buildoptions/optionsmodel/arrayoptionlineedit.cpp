/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
