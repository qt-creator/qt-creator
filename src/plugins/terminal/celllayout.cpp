// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "celllayout.h"

#include <QFontMetrics>

namespace Terminal::Internal {

QColor toQColor(const VTermColor &c)
{
    return QColor(qRgb(c.rgb.red, c.rgb.green, c.rgb.blue));
};

std::u32string cellToString(const VTermScreenCell &cell)
{
    if (cell.chars[0] != 0xFFFFFFFF) {
        QString ch = QString::fromUcs4(cell.chars);
        if (ch.size() > 1)
            ch = ch.normalized(QString::NormalizationForm_C);
        QList<uint> asUcs4 = ch.toUcs4();

        std::u32string chUcs = std::u32string(asUcs4.begin(), asUcs4.end());
        if (chUcs.size() > 0) {
            if (chUcs[0] == 0)
                chUcs[0] = 0x00a0;

            return chUcs;
        }
    }
    return std::u32string(1, (char32_t) 0x00a0);
}

void createTextLayout(QTextLayout &textLayout,
                      std::u32string *resultText,
                      VTermColor defaultBg,
                      QRect cellRect,
                      qreal lineSpacing,
                      std::function<const VTermScreenCell *(int x, int y)> fetchCell)
{
    QList<QTextLayout::FormatRange> formats;

    QTextCharFormat currentFormat;
    int currentFormatStart = 0;
    currentFormat.setForeground(QColor(0xff, 0xff, 0xff));
    currentFormat.clearBackground();

    QString layoutText;
    if (resultText)
        resultText->clear();

    for (int y = cellRect.y(); y < cellRect.bottom() + 1; y++) {
        QTextCharFormat format;

        const auto setNewFormat = [&formats, &currentFormatStart, &layoutText, &currentFormat](
                                      const QTextCharFormat &format) {
            if (layoutText.size() != currentFormatStart) {
                QTextLayout::FormatRange fr;
                fr.start = currentFormatStart;
                fr.length = layoutText.size() - currentFormatStart;
                fr.format = currentFormat;
                formats.append(fr);

                currentFormat = format;
                currentFormatStart = layoutText.size();
            } else {
                currentFormat = format;
            }
        };

        for (int x = cellRect.x(); x < cellRect.right() + 1; x++) {
            const VTermScreenCell *cell = fetchCell(x, y);

            const VTermColor *bg = &cell->bg;
            const VTermColor *fg = &cell->fg;

            if (static_cast<bool>(cell->attrs.reverse)) {
                bg = &cell->fg;
                fg = &cell->bg;
            }

            format = QTextCharFormat();
            format.setForeground(toQColor(*fg));

            if (!vterm_color_is_equal(bg, &defaultBg))
                format.setBackground(toQColor(*bg));
            else
                format.clearBackground();

            if (cell->attrs.bold)
                format.setFontWeight(QFont::Bold);
            if (cell->attrs.underline)
                format.setFontUnderline(true);
            if (cell->attrs.italic)
                format.setFontItalic(true);
            if (cell->attrs.strike)
                format.setFontStrikeOut(true);

            if (format != currentFormat)
                setNewFormat(format);

            if (cell->chars[0] != 0xFFFFFFFF) {
                QString ch = QString::fromUcs4(cell->chars);
                if (ch == ' ')
                    ch = QChar::Nbsp;
                if (ch.size() > 0) {
                    layoutText += ch;
                } else {
                    layoutText += QChar::Nbsp;
                }
            }

            if (resultText)
                *resultText += cellToString(*cell);
        } // for x
        setNewFormat(format);
        if (y != cellRect.bottom())
            layoutText.append(QChar::LineSeparator);
    } // for y

    QTextLayout::FormatRange fr;
    fr.start = currentFormatStart;
    fr.length = (layoutText.size() - 1) - currentFormatStart;
    fr.format = currentFormat;
    formats.append(fr);

    textLayout.setText(layoutText);
    textLayout.setFormats(formats);

    qreal height = 0;
    textLayout.beginLayout();
    while (1) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;

        // Just give it a number that is definitely larger than
        // the number of columns in a line.
        line.setNumColumns(std::numeric_limits<int>::max());
        line.setPosition(QPointF(0, height));
        height += lineSpacing;
    }
    textLayout.endLayout();
}

} // namespace Terminal::Internal
