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

#include "bineditor.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>

#include <QtCore/QByteArrayMatcher>
#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QWheelEvent>

using namespace BINEditor;

// QByteArray::toLower() is broken, it stops at the first \0
static void lower(QByteArray &ba)
{
    char *data = ba.data();
    char *end = data + ba.size();
    while (data != end) {
        if (*data >= 0x41 && *data <= 0x5A)
            *data += 0x20;
        ++data;
    }
}

static QByteArray calculateHexPattern(const QByteArray &pattern)
{
    QByteArray result;
    if (pattern.size() % 2 == 0) {
        bool ok = true;
        int i = 0;
        while (i < pattern.size()) {
            ushort s = pattern.mid(i, 2).toUShort(&ok, 16);
            if (!ok) {
                return QByteArray();
            }
            result.append(s);
            i += 2;
        }
    }
    return result;
}

BinEditor::BinEditor(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    m_ieditor = 0;
    m_inLazyMode = false;
    m_baseAddr = 0;
    m_blockSize = 4096;
    m_size = 0;
    m_addressBytes = 4;
    init();
    m_unmodifiedState = 0;
    m_readOnly = false;
    m_hexCursor = true;
    m_cursorPosition = 0;
    m_anchorPosition = 0;
    m_lowNibble = false;
    m_cursorVisible = false;
    m_caseSensitiveSearch = false;
    setFocusPolicy(Qt::WheelFocus);
}

BinEditor::~BinEditor()
{
}

void BinEditor::init()
{
    const int addressStringWidth =
        2*m_addressBytes + (m_addressBytes - 1) / 2;
    m_addressString = QString(addressStringWidth, QLatin1Char(':'));
    QFontMetrics fm(fontMetrics());
    m_margin = 4;
    m_descent = fm.descent();
    m_ascent = fm.ascent();
    m_lineHeight = fm.lineSpacing();
    m_charWidth = fm.width(QChar(QLatin1Char('M')));
    m_columnWidth = 2 * m_charWidth + fm.width(QChar(QLatin1Char(' ')));
    m_numLines = m_size / 16 + 1;
    m_numVisibleLines = viewport()->height() / m_lineHeight;
    m_textWidth = 16 * m_charWidth + m_charWidth;
    int m_numberWidth = fm.width(QChar(QLatin1Char('9')));
    m_labelWidth =
        2*m_addressBytes * m_numberWidth + (m_addressBytes - 1)/2 * m_charWidth;

    int expectedCharWidth = m_columnWidth / 3;
    const char *hex = "0123456789abcdef";
    m_isMonospacedFont = true;
    while (*hex) {
        if (fm.width(QLatin1Char(*hex)) != expectedCharWidth) {
            m_isMonospacedFont = false;
            break;
        }
        ++hex;
    }

    if (m_isMonospacedFont && fm.width("M M ") != m_charWidth * 4) {
        // On Qt/Mac, monospace font widths may have a fractional component
        // This breaks the assumption that width("MMM") == width('M') * 3

        m_isMonospacedFont = false;
        m_columnWidth = fm.width("MMM");
        m_labelWidth = m_addressBytes == 4
            ? fm.width("MMMM:MMMM")
            : fm.width("MMMM:MMMM:MMMM:MMMM");
    }

    horizontalScrollBar()->setRange(0, 2 * m_margin + 16 * m_columnWidth
                                    + m_labelWidth + m_textWidth - viewport()->width());
    horizontalScrollBar()->setPageStep(viewport()->width());
    verticalScrollBar()->setRange(0, m_numLines - m_numVisibleLines);
    verticalScrollBar()->setPageStep(m_numVisibleLines);
}


void BinEditor::addLazyData(quint64 block, const QByteArray &data)
{
    Q_ASSERT(m_inLazyMode);
    Q_ASSERT(data.size() == m_blockSize);
    const quint64 addr = block * m_blockSize;
    if (addr >= m_baseAddr && addr <= m_baseAddr + m_size - 1) {
        if (m_lazyData.size() * m_blockSize >= 64 * 1024 * 1024)
            m_lazyData.clear();
        const int translatedBlock = (addr - m_baseAddr) / m_blockSize;
        m_lazyData.insert(translatedBlock, data);
        m_lazyRequests.remove(translatedBlock);
        viewport()->update();
    }
}

bool BinEditor::requestDataAt(int pos, bool synchronous) const
{
    if (!m_inLazyMode)
        return true;

    int block = pos / m_blockSize;
    QMap<int, QByteArray>::const_iterator it = m_modifiedData.find(block);
    if (it != m_modifiedData.constEnd())
        return true;
    it = m_lazyData.find(block);
    if (it == m_lazyData.end()) {
        if (!m_lazyRequests.contains(block)) {
            m_lazyRequests.insert(block);
            emit const_cast<BinEditor*>(this)->
                lazyDataRequested(m_baseAddr / m_blockSize + block, synchronous);
            if (!m_lazyRequests.contains(block))
                return true; // synchronous data source
        }
        return false;
    }

    return true;
}

char BinEditor::dataAt(int pos) const
{
    if (!m_inLazyMode)
        return m_data.at(pos);
    int block = pos / m_blockSize;
    return blockData(block).at(pos - (block*m_blockSize));
}

void BinEditor::changeDataAt(int pos, char c)
{
    if (!m_inLazyMode) {
        m_data[pos] = c;
        return;
    }
    int block = pos / m_blockSize;
    QMap<int, QByteArray>::iterator it = m_modifiedData.find(block);
    if (it != m_modifiedData.end()) {
        it.value()[pos - (block*m_blockSize)] = c;
    } else {
        it = m_lazyData.find(block);
        if (it != m_lazyData.end()) {
            QByteArray data = it.value();
            data[pos - (block*m_blockSize)] = c;
            m_modifiedData.insert(block, data);
        }
    }
}

QByteArray BinEditor::dataMid(int from, int length) const
{
    if (!m_inLazyMode)
        return m_data.mid(from, length);

    int end = from + length;
    int block = from / m_blockSize;

    QByteArray data;
    do {
        data += blockData(block++);
    } while (block * m_blockSize < end);

    return data.mid(from - ((from / m_blockSize) * m_blockSize), length);
}

QByteArray BinEditor::blockData(int block) const
{
    if (!m_inLazyMode) {
        QByteArray data = m_data.mid(block * m_blockSize, m_blockSize);
        if (data.size() < m_blockSize)
            data.resize(m_blockSize);
        return data;
    }
    QMap<int, QByteArray>::const_iterator it = m_modifiedData.find(block);
    return it != m_modifiedData.constEnd()
            ? it.value() : m_lazyData.value(block, m_emptyBlock);
}


void BinEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    setFont(fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_TEXT)).font());
}

void BinEditor::setBlinkingCursorEnabled(bool enable)
{
    if (enable && QApplication::cursorFlashTime() > 0)
        m_cursorBlinkTimer.start(QApplication::cursorFlashTime() / 2, this);
    else
        m_cursorBlinkTimer.stop();
    m_cursorVisible = enable;
    updateLines();
}

void BinEditor::focusInEvent(QFocusEvent *)
{
    setBlinkingCursorEnabled(true);
}

void BinEditor::focusOutEvent(QFocusEvent *)
{
    setBlinkingCursorEnabled(false);
}

void BinEditor::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_autoScrollTimer.timerId()) {
        QRect visible = viewport()->rect();
        QPoint pos;
        const QPoint globalPos = QCursor::pos();
        pos = viewport()->mapFromGlobal(globalPos);
        QMouseEvent ev(QEvent::MouseMove, pos, globalPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        mouseMoveEvent(&ev);
        int deltaY = qMax(pos.y() - visible.top(), visible.bottom() - pos.y()) - visible.height();
        int deltaX = qMax(pos.x() - visible.left(), visible.right() - pos.x()) - visible.width();
        int delta = qMax(deltaX, deltaY);
        if (delta >= 0) {
            if (delta < 7)
                delta = 7;
            int timeout = 4900 / (delta * delta);
            m_autoScrollTimer.start(timeout, this);

            if (deltaY > 0)
                verticalScrollBar()->triggerAction(pos.y() < visible.center().y() ?
                                       QAbstractSlider::SliderSingleStepSub
                                       : QAbstractSlider::SliderSingleStepAdd);
            if (deltaX > 0)
                horizontalScrollBar()->triggerAction(pos.x() < visible.center().x() ?
                                       QAbstractSlider::SliderSingleStepSub
                                       : QAbstractSlider::SliderSingleStepAdd);
        }
    } else if (e->timerId() == m_cursorBlinkTimer.timerId()) {
        m_cursorVisible = !m_cursorVisible;
        updateLines();
    }
    QAbstractScrollArea::timerEvent(e);
}


void BinEditor::setModified(bool modified)
{
    int unmodifiedState = modified ? -1 : m_undoStack.size();
    if (unmodifiedState == m_unmodifiedState)
        return;
    m_unmodifiedState = unmodifiedState;
    emit modificationChanged(m_undoStack.size() != m_unmodifiedState);
}

bool BinEditor::isModified() const
{
    return (m_undoStack.size() != m_unmodifiedState);
}

void BinEditor::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

bool BinEditor::isReadOnly() const
{
    return m_readOnly;
}

void BinEditor::setData(const QByteArray &data)
{
    m_inLazyMode = false;
    m_baseAddr = 0;
    m_lazyData.clear();
    m_modifiedData.clear();
    m_lazyRequests.clear();
    m_data = data;
    m_size = data.size();
    m_addressBytes = 4;

    m_unmodifiedState = 0;
    m_undoStack.clear();
    m_redoStack.clear();

    init();
    m_cursorPosition = 0;
    verticalScrollBar()->setValue(0);

    emit cursorPositionChanged(m_cursorPosition);
    viewport()->update();
}

QByteArray BinEditor::data() const
{
    return m_data;
}

bool BinEditor::save(const QString &oldFileName, const QString &newFileName)
{
    if (m_inLazyMode) {
        if (oldFileName != newFileName) {
            QString tmpName;
            {
                QTemporaryFile tmp;
                if (!tmp.open())
                    return false;
                tmpName = tmp.fileName();
            }
            if (!QFile::copy(oldFileName, tmpName))
                return false;
            if (QFile::exists(newFileName) && !QFile::remove(newFileName))
                return false;
            if (!QFile::rename(tmpName, newFileName))
                return false;
        }
        QFile output(newFileName);
        if (!output.open(QIODevice::ReadWrite)) // QtBug: WriteOnly truncates.
            return false;
        const qint64 size = output.size();
        for (QMap<int, QByteArray>::const_iterator it = m_modifiedData.constBegin();
            it != m_modifiedData.constEnd(); ++it) {
            if (!output.seek(it.key() * m_blockSize))
                return false;
            if (output.write(it.value()) < m_blockSize)
                return false;
        }
        if (size % m_blockSize != 0 && !output.resize(size))
            return false;
    } else {
        QFile output(newFileName);
        if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        if (output.write(m_data) < m_size)
            return false;
    }
    setModified(false);
    return true;
}

void BinEditor::setLazyData(quint64 startAddr, int range, int blockSize)
{
    m_inLazyMode = true;
    m_blockSize = blockSize;
    Q_ASSERT((blockSize/16) * 16 == blockSize);
    m_emptyBlock = QByteArray(blockSize, '\0');
    m_data.clear();
    m_lazyData.clear();
    m_modifiedData.clear();
    m_lazyRequests.clear();

    // In lazy mode, users can edit data in the range
    // [startAddr - range/2, startAddr + range/2].
    m_baseAddr = static_cast<quint64>(range/2) > startAddr
                ? 0 : startAddr - range/2;
    m_baseAddr = (m_baseAddr / blockSize) * blockSize;
    m_size = m_baseAddr != 0 && static_cast<quint64>(range) >= -m_baseAddr
             ? -m_baseAddr : range;
    m_addressBytes = (m_baseAddr + m_size < quint64(1) << 32
                      && m_baseAddr + m_size >= m_baseAddr) ? 4 : 8;

    m_unmodifiedState = 0;
    m_undoStack.clear();
    m_redoStack.clear();

    init();

    m_cursorPosition = startAddr - m_baseAddr;
    verticalScrollBar()->setValue(m_cursorPosition / 16);

    emit cursorPositionChanged(m_cursorPosition);
    viewport()->update();
}

void BinEditor::resizeEvent(QResizeEvent *)
{
    init();
}

void BinEditor::scrollContentsBy(int dx, int dy)
{
    viewport()->scroll(isRightToLeft() ? -dx : dx, dy * m_lineHeight);
}

void BinEditor::changeEvent(QEvent *e)
{
    QAbstractScrollArea::changeEvent(e);
    if (e->type() == QEvent::ActivationChange) {
        if (!isActiveWindow())
            m_autoScrollTimer.stop();
    }
    init();
    viewport()->update();
}


void BinEditor::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier) {
        const int delta = e->delta();
        if (delta < 0)
            zoomOut();
        else if (delta > 0)
            zoomIn();
        return;
    }
    QAbstractScrollArea::wheelEvent(e);
}



QRect BinEditor::cursorRect() const
{
    int topLine = verticalScrollBar()->value();
    int line = m_cursorPosition / 16;
    int y = (line - topLine) * m_lineHeight;
    int xoffset = horizontalScrollBar()->value();
    int column = m_cursorPosition % 16;
    int x = m_hexCursor ?
            (-xoffset + m_margin + m_labelWidth + column * m_columnWidth)
            : (-xoffset + m_margin + m_labelWidth + 16 * m_columnWidth + m_charWidth + column * m_charWidth);
    int w = m_hexCursor ? m_columnWidth : m_charWidth;
    return QRect(x, y, w, m_lineHeight);
}

int BinEditor::posAt(const QPoint &pos) const
{
    int xoffset = horizontalScrollBar()->value();
    int x = xoffset + pos.x() - m_margin - m_labelWidth;
    int column = qMin(15, qMax(0,x) / m_columnWidth);
    int topLine = verticalScrollBar()->value();
    int line = pos.y() / m_lineHeight;


    if (x > 16 * m_columnWidth + m_charWidth/2) {
        x -= 16 * m_columnWidth + m_charWidth;
        for (column = 0; column < 15; ++column) {
            int pos = (topLine + line) * 16 + column;
            if (pos < 0 || pos >= m_size)
                break;
            QChar qc(QLatin1Char(dataAt(pos)));
            if (!qc.isPrint())
                qc = 0xB7;
            x -= fontMetrics().width(qc);
            if (x <= 0)
                break;
        }
    }

    return (qMin(m_size, qMin(m_numLines, topLine + line) * 16) + column);
}

bool BinEditor::inTextArea(const QPoint &pos) const
{
    int xoffset = horizontalScrollBar()->value();
    int x = xoffset + pos.x() - m_margin - m_labelWidth;
    return (x > 16 * m_columnWidth + m_charWidth/2);
}

void BinEditor::updateLines()
{
    updateLines(m_cursorPosition, m_cursorPosition);
}

void BinEditor::updateLines(int fromPosition, int toPosition)
{
    int topLine = verticalScrollBar()->value();
    int firstLine = qMin(fromPosition, toPosition) / 16;
    int lastLine = qMax(fromPosition, toPosition) / 16;
    int y = (firstLine - topLine) * m_lineHeight;
    int h = (lastLine - firstLine + 1 ) * m_lineHeight;

    viewport()->update(0, y, viewport()->width(), h);
}

int BinEditor::dataIndexOf(const QByteArray &pattern, int from, bool caseSensitive) const
{
    if (!m_inLazyMode && caseSensitive) {
        return m_data.indexOf(pattern, from);
    }

    int trailing = pattern.size();
    if (trailing > m_blockSize)
        return -1;

    QByteArray buffer;
    buffer.resize(m_blockSize + trailing);
    char *b = buffer.data();
    QByteArrayMatcher matcher(pattern);

    int block = from / m_blockSize;
    const int end =
        qMin<qint64>(static_cast<qint64>(from) + SearchStride, m_size);
    while (from < end) {
        if (!requestDataAt(block * m_blockSize, true))
            return -1;
        QByteArray data = blockData(block);
        ::memcpy(b, b + m_blockSize, trailing);
        ::memcpy(b + trailing, data.constData(), m_blockSize);

        if (!caseSensitive)
            ::lower(buffer);

        int pos = matcher.indexIn(buffer, from - (block * m_blockSize) + trailing);
        if (pos >= 0)
            return pos + block * m_blockSize - trailing;
        ++block;
        from = block * m_blockSize - trailing;
    }
    return end == m_size ? -1 : -2;
}

int BinEditor::dataLastIndexOf(const QByteArray &pattern, int from, bool caseSensitive) const
{
    if (!m_inLazyMode && caseSensitive)
        return m_data.lastIndexOf(pattern, from);

    int trailing = pattern.size();
    if (trailing > m_blockSize)
        return -1;

    QByteArray buffer;
    buffer.resize(m_blockSize + trailing);
    char *b = buffer.data();

    int block = from / m_blockSize;
    const int lowerBound = qMax(0, from - SearchStride);
    while (from > lowerBound) {
        if (!requestDataAt(block * m_blockSize, true))
            return -1;
        QByteArray data = blockData(block);
        ::memcpy(b + m_blockSize, b, trailing);
        ::memcpy(b, data.constData(), m_blockSize);

        if (!caseSensitive)
            ::lower(buffer);

        int pos = buffer.lastIndexOf(pattern, from - (block * m_blockSize));
        if (pos >= 0)
            return pos + block * m_blockSize;
        --block;
        from = block * m_blockSize + (m_blockSize-1) + trailing;
    }
    return lowerBound == 0 ? -1 : -2;
}


int BinEditor::find(const QByteArray &pattern_arg, int from,
                    QTextDocument::FindFlags findFlags)
{
    if (pattern_arg.isEmpty())
        return 0;

    QByteArray pattern = pattern_arg;

    bool caseSensitiveSearch = (findFlags & QTextDocument::FindCaseSensitively);

    if (!caseSensitiveSearch)
        ::lower(pattern);

    bool backwards = (findFlags & QTextDocument::FindBackward);
    int found = backwards ? dataLastIndexOf(pattern, from, caseSensitiveSearch)
                : dataIndexOf(pattern, from, caseSensitiveSearch);

    int foundHex = -1;
    QByteArray hexPattern = calculateHexPattern(pattern_arg);
    if (!hexPattern.isEmpty()) {
        foundHex = backwards ? dataLastIndexOf(hexPattern, from)
                   : dataIndexOf(hexPattern, from);
    }

    int pos = foundHex == -1 || (found >= 0 && (foundHex == -2 || found < foundHex))
              ? found : foundHex;

    if (pos >= m_size)
        pos = -1;

    if (pos >= 0) {
        setCursorPosition(pos);
        setCursorPosition(pos + (found == pos ? pattern.size() : hexPattern.size()), KeepAnchor);
    }
    return pos;
}

int BinEditor::findPattern(const QByteArray &data, const QByteArray &dataHex, int from, int offset, int *match)
{
    if (m_searchPattern.isEmpty())
        return -1;
    int normal = m_searchPattern.isEmpty()? -1 : data.indexOf(m_searchPattern, from - offset);
    int hex = m_searchPatternHex.isEmpty()? -1 : dataHex.indexOf(m_searchPatternHex, from - offset);

    if (normal >= 0 && (hex < 0 || normal < hex)) {
        if (match)
            *match = m_searchPattern.length();
        return normal + offset;
    }
    if (hex >= 0) {
        if (match)
            *match = m_searchPatternHex.length();
        return hex + offset;
    }

    return -1;
}


void BinEditor::drawItems(QPainter *painter, int x, int y, const QString &itemString)
{
    if (m_isMonospacedFont) {
        painter->drawText(x, y, itemString);
    } else {
        for (int i = 0; i < 16; ++i)
            painter->drawText(x + i*m_columnWidth, y, itemString.mid(i*3, 2));
    }
}

QString BinEditor::addressString(quint64 address)
{
    QChar *addressStringData = m_addressString.data();
    const char *hex = "0123456789abcdef";

    // Take colons into account.
    const int indices[16] = {
        0, 1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 17, 18
    };

    for (int b = 0; b < m_addressBytes; ++b) {
        addressStringData[indices[2*m_addressBytes - 1 - b*2]] =
            hex[(address >> (8*b)) & 0xf];
        addressStringData[indices[2*m_addressBytes - 2 - b*2]] =
            hex[(address >> (8*b + 4)) & 0xf];
    }
    return m_addressString;
}

void BinEditor::paintEvent(QPaintEvent *e)
{
    QPainter painter(viewport());
    int topLine = verticalScrollBar()->value();
    int xoffset = horizontalScrollBar()->value();
    painter.drawLine(-xoffset + m_margin + m_labelWidth - m_charWidth/2, 0,
                     -xoffset + m_margin + m_labelWidth - m_charWidth/2, viewport()->height());
    painter.drawLine(-xoffset + m_margin + m_labelWidth + 16 * m_columnWidth + m_charWidth/2, 0,
                     -xoffset + m_margin + m_labelWidth + 16 * m_columnWidth + m_charWidth/2, viewport()->height());


    int viewport_height = viewport()->height();
    QBrush alternate_base = palette().alternateBase();
    for (int i = 0; i < 8; ++i) {
        int bg_x = -xoffset +  m_margin + (2 * i + 1) * m_columnWidth + m_labelWidth;
        QRect r(bg_x - m_charWidth/2, 0, m_columnWidth, viewport_height);
        painter.fillRect(e->rect() & r, palette().alternateBase());
    }

    int matchLength = 0;

    QByteArray patternData, patternDataHex;
    int patternOffset = qMax(0, topLine*16 - m_searchPattern.size());
    if (!m_searchPattern.isEmpty()) {
        patternData = dataMid(patternOffset, m_numVisibleLines * 16 + (topLine*16 - patternOffset));
        patternDataHex = patternData;
        if (!m_caseSensitiveSearch)
            ::lower(patternData);
    }


    int foundPatternAt = findPattern(patternData, patternDataHex, patternOffset, patternOffset, &matchLength);

    int selStart = qMin(m_cursorPosition, m_anchorPosition);
    int selEnd = qMax(m_cursorPosition, m_anchorPosition);

    QString itemString(16*3, QLatin1Char(' '));
    QChar *itemStringData = itemString.data();
    const char *hex = "0123456789abcdef";

    painter.setPen(palette().text().color());
    const QFontMetrics &fm = painter.fontMetrics();
    for (int i = 0; i <= m_numVisibleLines; ++i) {
        int line = topLine + i;
        if (line >= m_numLines)
            break;

        int y = i * m_lineHeight + m_ascent;
        if (y - m_ascent > e->rect().bottom())
            break;
        if (y + m_descent < e->rect().top())
            continue;


        painter.drawText(-xoffset, i * m_lineHeight + m_ascent,
                         addressString(m_baseAddr + ((uint) line) * 16));

        int cursor = -1;
        if (line * 16 <= m_cursorPosition && m_cursorPosition < line * 16 + 16)
            cursor = m_cursorPosition - line * 16;

        bool hasData = requestDataAt(line * 16);

        QString printable;

        if (hasData) {
            for (int c = 0; c < 16; ++c) {
                int pos = line * 16 + c;
                if (pos >= m_size)
                    break;
                QChar qc(QLatin1Char(dataAt(pos)));
                if (qc.unicode() >= 127 || !qc.isPrint())
                    qc = 0xB7;
                printable += qc;
            }
        } else {
            printable = QString(16, QLatin1Char(' '));
        }

        QRect selectionRect;
        QRect printableSelectionRect;

        bool isFullySelected = (selStart < selEnd && selStart <= line*16 && (line+1)*16 <= selEnd);

        if (hasData) {
            for (int c = 0; c < 16; ++c) {
                int pos = line * 16 + c;
                if (pos >= m_size) {
                    while (c < 16) {
                        itemStringData[c*3] = itemStringData[c*3+1] = ' ';
                        ++c;
                    }
                    break;
                }

                if (foundPatternAt >= 0 && pos >= foundPatternAt + matchLength)
                    foundPatternAt = findPattern(patternData, patternDataHex, foundPatternAt + matchLength, patternOffset, &matchLength);


                uchar value = (uchar)dataAt(pos);
                itemStringData[c*3] = hex[value >> 4];
                itemStringData[c*3+1] = hex[value & 0xf];

                int item_x = -xoffset +  m_margin + c * m_columnWidth + m_labelWidth;

                if (foundPatternAt >= 0 && pos >= foundPatternAt && pos < foundPatternAt + matchLength) {
                    painter.fillRect(item_x, y-m_ascent, m_columnWidth, m_lineHeight, QColor(0xffef0b));
                    int printable_item_x = -xoffset + m_margin + m_labelWidth + 16 * m_columnWidth + m_charWidth
                                           + fm.width(printable.left(c));
                    painter.fillRect(printable_item_x, y-m_ascent,
                                     fm.width(printable.at(c)),
                                     m_lineHeight, QColor(0xffef0b));
                }

                if (selStart < selEnd && !isFullySelected && pos >= selStart && pos < selEnd) {
                    selectionRect |= QRect(item_x, y-m_ascent, m_columnWidth, m_lineHeight);
                    int printable_item_x = -xoffset + m_margin + m_labelWidth + 16 * m_columnWidth + m_charWidth
                                           + fm.width(printable.left(c));
                    printableSelectionRect |= QRect(printable_item_x, y-m_ascent,
                                                    fm.width(printable.at(c)),
                                                    m_lineHeight);
                }
            }
        }

        int x = -xoffset +  m_margin + m_labelWidth;

        if (isFullySelected) {
            painter.save();
            painter.fillRect(x, y-m_ascent, 16*m_columnWidth, m_lineHeight, palette().highlight());
            painter.setPen(palette().highlightedText().color());
            drawItems(&painter, x, y, itemString);
            painter.restore();
        } else {
            drawItems(&painter, x, y, itemString);
            if (!selectionRect.isEmpty()) {
                painter.save();
                painter.fillRect(selectionRect, palette().highlight());
                painter.setPen(palette().highlightedText().color());
                painter.setClipRect(selectionRect);
                drawItems(&painter, x, y, itemString);
                painter.restore();
            }
        }


        if (cursor >= 0) {
            int w = fm.boundingRect(itemString.mid(cursor*3, 2)).width();
            QRect cursorRect(x + cursor * m_columnWidth, y - m_ascent, w + 1, m_lineHeight);
            painter.save();
            painter.setPen(Qt::red);
            painter.drawRect(cursorRect.adjusted(0, 0, 0, -1));
            painter.restore();
            if (m_hexCursor && m_cursorVisible) {
                if (m_lowNibble)
                    cursorRect.adjust(fm.width(itemString.left(1)), 0, 0, 0);
                painter.fillRect(cursorRect, Qt::red);
                painter.save();
                painter.setClipRect(cursorRect);
                painter.setPen(Qt::white);
                drawItems(&painter, x, y, itemString);
                painter.restore();
            }
        }

        int text_x = -xoffset + m_margin + m_labelWidth + 16 * m_columnWidth + m_charWidth;

        if (isFullySelected) {
                painter.save();
                painter.fillRect(text_x, y-m_ascent, fm.width(printable), m_lineHeight,
                                 palette().highlight());
                painter.setPen(palette().highlightedText().color());
                painter.drawText(text_x, y, printable);
                painter.restore();
        }else {
            painter.drawText(text_x, y, printable);
            if (!printableSelectionRect.isEmpty()) {
                painter.save();
                painter.fillRect(printableSelectionRect, palette().highlight());
                painter.setPen(palette().highlightedText().color());
                painter.setClipRect(printableSelectionRect);
                painter.drawText(text_x, y, printable);
                painter.restore();
            }
        }

        if (cursor >= 0 && !printable.isEmpty()) {
            QRect cursorRect(text_x + fm.width(printable.left(cursor)),
                             y-m_ascent,
                             fm.width(printable.at(cursor)),
                             m_lineHeight);
            painter.save();
            if (m_hexCursor || !m_cursorVisible) {
                painter.setPen(Qt::red);
                painter.drawRect(cursorRect.adjusted(0, 0, 0, -1));
            } else {
                painter.setClipRect(cursorRect);
                painter.fillRect(cursorRect, Qt::red);
                painter.setPen(Qt::white);
                painter.drawText(text_x, y, printable);
            }
            painter.restore();
        }
    }
}


int BinEditor::cursorPosition() const
{
    return m_cursorPosition;
}

void BinEditor::setCursorPosition(int pos, MoveMode moveMode)
{
    pos = qMin(m_size-1, qMax(0, pos));
    if (pos == m_cursorPosition
        && (m_anchorPosition == m_cursorPosition || moveMode == KeepAnchor)
        && !m_lowNibble)
        return;

    int oldCursorPosition = m_cursorPosition;

    bool hasSelection = m_anchorPosition != m_cursorPosition;
    m_lowNibble = false;
    if (!hasSelection)
        updateLines();
    m_cursorPosition = pos;
    if (moveMode == MoveAnchor) {
        if (hasSelection)
            updateLines(m_anchorPosition, oldCursorPosition);
        m_anchorPosition = m_cursorPosition;
    }

    hasSelection = m_anchorPosition != m_cursorPosition;
    updateLines(hasSelection ? oldCursorPosition : m_cursorPosition, m_cursorPosition);
    ensureCursorVisible();
    if (hasSelection != (m_anchorPosition != m_anchorPosition))
        emit copyAvailable(m_anchorPosition != m_cursorPosition);
    emit cursorPositionChanged(m_cursorPosition);
}


void BinEditor::ensureCursorVisible()
{
    QRect cr = cursorRect();
    QRect vr = viewport()->rect();
    if (!vr.contains(cr)) {
        if (cr.top() < vr.top())
            verticalScrollBar()->setValue(m_cursorPosition / 16);
        else if (cr.bottom() > vr.bottom())
            verticalScrollBar()->setValue(m_cursorPosition / 16 - m_numVisibleLines + 1);
    }
}

void BinEditor::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;
    setCursorPosition(posAt(e->pos()));
    setBlinkingCursorEnabled(true);
    if (m_hexCursor == inTextArea(e->pos())) {
        m_hexCursor = !m_hexCursor;
        updateLines();
    }
}

void BinEditor::mouseMoveEvent(QMouseEvent *e)
{
    if (!(e->buttons() & Qt::LeftButton))
        return;
    setCursorPosition(posAt(e->pos()), KeepAnchor);
    if (m_hexCursor == inTextArea(e->pos())) {
        m_hexCursor = !m_hexCursor;
        updateLines();
    }
    QRect visible = viewport()->rect();
    if (visible.contains(e->pos()))
        m_autoScrollTimer.stop();
    else if (!m_autoScrollTimer.isActive())
        m_autoScrollTimer.start(100, this);
}

void BinEditor::mouseReleaseEvent(QMouseEvent *)
{
    if (m_autoScrollTimer.isActive()) {
        m_autoScrollTimer.stop();
        ensureCursorVisible();
    }
}

void BinEditor::selectAll()
{
    setCursorPosition(0);
    setCursorPosition(m_size-1, KeepAnchor);
}

void BinEditor::clear()
{
    setData(QByteArray());
}

bool BinEditor::event(QEvent *e) {
    if (e->type() == QEvent::KeyPress) {
        switch (static_cast<QKeyEvent*>(e)->key()) {
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            m_hexCursor = !m_hexCursor;
            setBlinkingCursorEnabled(true);
            ensureCursorVisible();
            e->accept();
            return true;
        default:;
        }
    }
    return QAbstractScrollArea::event(e);
}

void BinEditor::keyPressEvent(QKeyEvent *e)
{

    if (e == QKeySequence::SelectAll) {
            e->accept();
            selectAll();
            return;
    } else if (e == QKeySequence::Copy) {
        e->accept();
        copy();
        return;
    } else if (e == QKeySequence::Undo) {
        e->accept();
        undo();
        return;
    } else if (e == QKeySequence::Redo) {
        e->accept();
        redo();
        return;
    }


    MoveMode moveMode = e->modifiers() & Qt::ShiftModifier ? KeepAnchor : MoveAnchor;
    switch (e->key()) {
    case Qt::Key_Up:
        setCursorPosition(m_cursorPosition - 16, moveMode);
        break;
    case Qt::Key_Down:
        setCursorPosition(m_cursorPosition + 16, moveMode);
        break;
    case Qt::Key_Right:
        setCursorPosition(m_cursorPosition + 1, moveMode);
        break;
    case Qt::Key_Left:
        setCursorPosition(m_cursorPosition - 1, moveMode);
        break;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown: {
        int line = qMax(0, m_cursorPosition / 16 - verticalScrollBar()->value());
        verticalScrollBar()->triggerAction(e->key() == Qt::Key_PageUp ?
                                           QScrollBar::SliderPageStepSub : QScrollBar::SliderPageStepAdd);
        setCursorPosition((verticalScrollBar()->value() + line) * 16 + m_cursorPosition % 16, moveMode);
    } break;

    case Qt::Key_Home:
        setCursorPosition((e->modifiers() & Qt::ControlModifier) ?
                          0 : (m_cursorPosition/16 * 16), moveMode);
        break;
    case Qt::Key_End:
        setCursorPosition((e->modifiers() & Qt::ControlModifier) ?
                          (m_size-1) : (m_cursorPosition/16 * 16 + 15), moveMode);
        break;

    default:
        if (m_readOnly)
            break;
        {
        QString text = e->text();
        for (int i = 0; i < text.length(); ++i) {
            QChar c = text.at(i);
            if (m_hexCursor) {
                c = c.toLower();
                int nibble = -1;
                if (c.unicode() >= 'a' && c.unicode() <= 'f')
                    nibble = c.unicode() - 'a' + 10;
                else if (c.unicode() >= '0' && c.unicode() <= '9')
                    nibble = c.unicode() - '0';
                if (nibble < 0)
                    continue;
                if (m_lowNibble) {
                    changeData(m_cursorPosition, nibble + (dataAt(m_cursorPosition) & 0xf0));
                    m_lowNibble = false;
                    setCursorPosition(m_cursorPosition + 1);
                } else {
                    changeData(m_cursorPosition, (nibble << 4) + (dataAt(m_cursorPosition) & 0x0f), true);
                    m_lowNibble = true;
                    updateLines();
                }
            } else {
                if (c.unicode() >= 128 || !c.isPrint())
                    continue;
                changeData(m_cursorPosition, c.unicode(), m_cursorPosition + 1);
                setCursorPosition(m_cursorPosition + 1);
            }
            setBlinkingCursorEnabled(true);
        }
    }
    }

    e->accept();
}

void BinEditor::zoomIn(int range)
{
    QFont f = font();
    const int newSize = f.pointSize() + range;
    if (newSize <= 0)
        return;
    f.setPointSize(newSize);
    setFont(f);
}

void BinEditor::zoomOut(int range)
{
    zoomIn(-range);
}

void BinEditor::copy()
{
    const int selStart = selectionStart();
    const int selEnd = selectionEnd();
    if (selStart < selEnd) {
        const QByteArray &data = dataMid(selStart, selEnd - selStart);
        QString hexString;
        const char * const hex = "0123456789abcdef";
        for (int i = 0; i < data.size(); ++i) {
            const uchar val = static_cast<uchar>(data[i]);
            hexString.append(hex[val >> 4]).append(hex[val & 0xf]).append(' ');
        }
        hexString.chop(1);
        QApplication::clipboard()->setText(hexString);
    }
}

void BinEditor::highlightSearchResults(const QByteArray &pattern, QTextDocument::FindFlags findFlags)
{
    if (m_searchPattern == pattern)
        return;
    m_searchPattern = pattern;
    m_caseSensitiveSearch = (findFlags & QTextDocument::FindCaseSensitively);
    if (!m_caseSensitiveSearch)
        ::lower(m_searchPattern);
    m_searchPatternHex = calculateHexPattern(pattern);
    viewport()->update();
}


void BinEditor::changeData(int position, uchar character, bool highNibble)
{
    if (!requestDataAt(position))
        return;
    m_redoStack.clear();
    if (m_unmodifiedState > m_undoStack.size())
        m_unmodifiedState = -1;
    BinEditorEditCommand cmd;
    cmd.position = position;
    cmd.character = (uchar) dataAt(position);
    cmd.highNibble = highNibble;

    if (!highNibble && !m_undoStack.isEmpty() && m_undoStack.top().position == position && m_undoStack.top().highNibble) {
        // compress
        cmd.character = m_undoStack.top().character;
        m_undoStack.pop();
    }

    changeDataAt(position, (char) character);
    bool emitModificationChanged = (m_undoStack.size() == m_unmodifiedState);
    m_undoStack.push(cmd);
    if (emitModificationChanged) {
        emit modificationChanged(m_undoStack.size() != m_unmodifiedState);
    }

    if (m_undoStack.size() == 1)
        emit undoAvailable(true);
}


void BinEditor::undo()
{
    if (m_undoStack.isEmpty())
        return;
    bool emitModificationChanged = (m_undoStack.size() == m_unmodifiedState);
    BinEditorEditCommand cmd = m_undoStack.pop();
    emitModificationChanged |= (m_undoStack.size() == m_unmodifiedState);
    uchar c = dataAt(cmd.position);
    changeDataAt(cmd.position, (char)cmd.character);
    cmd.character = c;
    m_redoStack.push(cmd);
    setCursorPosition(cmd.position);
    if (emitModificationChanged)
        emit modificationChanged(m_undoStack.size() != m_unmodifiedState);
    if (!m_undoStack.size())
        emit undoAvailable(false);
    if (m_redoStack.size() == 1)
        emit redoAvailable(true);
}

void BinEditor::redo()
{
    if (m_redoStack.isEmpty())
        return;
    BinEditorEditCommand cmd = m_redoStack.pop();
    uchar c = dataAt(cmd.position);
    changeDataAt(cmd.position, (char)cmd.character);
    cmd.character = c;
    bool emitModificationChanged = (m_undoStack.size() == m_unmodifiedState);
    m_undoStack.push(cmd);
    setCursorPosition(cmd.position + 1);
    if (emitModificationChanged)
        emit modificationChanged(m_undoStack.size() != m_unmodifiedState);
    if (m_undoStack.size() == 1)
        emit undoAvailable(true);
    if (!m_redoStack.size())
        emit redoAvailable(false);
}
