/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "bineditor.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QByteArrayMatcher>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTemporaryFile>
#include <QtCore/QVariant>

#include <QtGui/QApplication>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QFontMetrics>
#include <QtGui/QHelpEvent>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>
#include <QtGui/QScrollBar>
#include <QtGui/QToolTip>
#include <QtGui/QWheelEvent>

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

namespace BINEditor {

BinEditor::BinEditor(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    m_ieditor = 0;
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
    m_canRequestNewWindow = false;
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
    ensureCursorVisible();
}


void BinEditor::addData(quint64 block, const QByteArray &data)
{
    Q_ASSERT(data.size() == m_blockSize);
    const quint64 addr = block * m_blockSize;
    if (addr >= m_baseAddr && addr <= m_baseAddr + m_size - 1) {
        if (m_data.size() * m_blockSize >= 64 * 1024 * 1024)
            m_data.clear();
        const int translatedBlock = (addr - m_baseAddr) / m_blockSize;
        m_data.insert(translatedBlock, data);
        m_requests.remove(translatedBlock);
        viewport()->update();
    }
}

bool BinEditor::requestDataAt(int pos) const
{
    int block = pos / m_blockSize;
    BlockMap::const_iterator it = m_modifiedData.find(block);
    if (it != m_modifiedData.constEnd())
        return true;
    it = m_data.find(block);
    if (it != m_data.end())
        return true;
    if (!m_requests.contains(block)) {
        m_requests.insert(block);
        emit const_cast<BinEditor*>(this)->
            dataRequested(editor(), m_baseAddr / m_blockSize + block);
        return true;
    }
    return false;
}

bool BinEditor::requestOldDataAt(int pos) const
{
    int block = pos / m_blockSize;
    BlockMap::const_iterator it = m_oldData.find(block);
    return it != m_oldData.end();
}

char BinEditor::dataAt(int pos, bool old) const
{
    int block = pos / m_blockSize;
    return blockData(block, old).at(pos - block*m_blockSize);
}

void BinEditor::changeDataAt(int pos, char c)
{
    int block = pos / m_blockSize;
    BlockMap::iterator it = m_modifiedData.find(block);
    if (it != m_modifiedData.end()) {
        it.value()[pos - (block*m_blockSize)] = c;
    } else {
        it = m_data.find(block);
        if (it != m_data.end()) {
            QByteArray data = it.value();
            data[pos - (block*m_blockSize)] = c;
            m_modifiedData.insert(block, data);
        }
    }

    emit dataChanged(editor(), m_baseAddr + pos, QByteArray(1, c));
}

QByteArray BinEditor::dataMid(int from, int length, bool old) const
{
    int end = from + length;
    int block = from / m_blockSize;

    QByteArray data;
    data.reserve(length);
    do {
        data += blockData(block++, old);
    } while (block * m_blockSize < end);

    return data.mid(from - ((from / m_blockSize) * m_blockSize), length);
}

QByteArray BinEditor::blockData(int block, bool old) const
{
    if (old) {
        BlockMap::const_iterator it = m_modifiedData.find(block);
        return it != m_modifiedData.constEnd()
                ? it.value() : m_oldData.value(block, m_emptyBlock);
    }
    BlockMap::const_iterator it = m_modifiedData.find(block);
    return it != m_modifiedData.constEnd()
            ? it.value() : m_data.value(block, m_emptyBlock);
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
        QMouseEvent ev(QEvent::MouseMove, pos, globalPos,
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        mouseMoveEvent(&ev);
        int deltaY = qMax(pos.y() - visible.top(),
                          visible.bottom() - pos.y()) - visible.height();
        int deltaX = qMax(pos.x() - visible.left(),
                          visible.right() - pos.x()) - visible.width();
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

bool BinEditor::save(const QString &oldFileName, const QString &newFileName)
{
    if (oldFileName != newFileName) {
        QString tmpName;
        {
            QTemporaryFile tmp(newFileName + QLatin1String("_XXXXXX.new"));
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
    for (BlockMap::const_iterator it = m_modifiedData.constBegin();
        it != m_modifiedData.constEnd(); ++it) {
        if (!output.seek(it.key() * m_blockSize))
            return false;
        if (output.write(it.value()) < m_blockSize)
            return false;
    }

    // We may have padded the displayed data, so we have to make sure
    // changes to that area are not actually written back to disk.
    if (!output.resize(size))
        return false;

    setModified(false);
    return true;
}

void BinEditor::setSizes(quint64 startAddr, int range, int blockSize)
{
    m_blockSize = blockSize;
    Q_ASSERT((blockSize/16) * 16 == blockSize);
    m_emptyBlock = QByteArray(blockSize, '\0');
    m_modifiedData.clear();
    m_requests.clear();

    // Users can edit data in the range
    // [startAddr - range/2, startAddr + range/2].
    m_baseAddr = quint64(range/2) > startAddr ? 0 : startAddr - range/2;
    m_baseAddr = (m_baseAddr / blockSize) * blockSize;

    const quint64 maxRange = Q_UINT64_C(0xffffffffffffffff) - m_baseAddr + 1;
    m_size = m_baseAddr != 0 && quint64(range) >= maxRange
              ? maxRange : range;
    m_addressBytes = (m_baseAddr + m_size < quint64(1) << 32
                   && m_baseAddr + m_size >= m_baseAddr) ? 4 : 8;

    m_unmodifiedState = 0;
    m_undoStack.clear();
    m_redoStack.clear();

    init();

    setCursorPosition(startAddr - m_baseAddr);
    viewport()->update();
}

void BinEditor::resizeEvent(QResizeEvent *)
{
    init();
}

void BinEditor::scrollContentsBy(int dx, int dy)
{
    viewport()->scroll(isRightToLeft() ? -dx : dx, dy * m_lineHeight);
    const QScrollBar * const scrollBar = verticalScrollBar();
    const int scrollPos = scrollBar->value();
    if (dy <= 0 && scrollPos == scrollBar->maximum())
        emit newRangeRequested(editor(), baseAddress() + m_size);
    else if (dy >= 0 && scrollPos == scrollBar->minimum())
        emit newRangeRequested(editor(), baseAddress());
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
    int x = m_hexCursor
            ? (-xoffset + m_margin + m_labelWidth + column * m_columnWidth)
            : (-xoffset + m_margin + m_labelWidth + 16 * m_columnWidth
               + m_charWidth + column * m_charWidth);
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
            int dataPos = (topLine + line) * 16 + column;
            if (dataPos < 0 || dataPos >= m_size)
                break;
            QChar qc(QLatin1Char(dataAt(dataPos)));
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
        if (!requestDataAt(block * m_blockSize))
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
    int trailing = pattern.size();
    if (trailing > m_blockSize)
        return -1;

    QByteArray buffer;
    buffer.resize(m_blockSize + trailing);
    char *b = buffer.data();

    int block = from / m_blockSize;
    const int lowerBound = qMax(0, from - SearchStride);
    while (from > lowerBound) {
        if (!requestDataAt(block * m_blockSize))
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

int BinEditor::findPattern(const QByteArray &data, const QByteArray &dataHex,
    int from, int offset, int *match)
{
    if (m_searchPattern.isEmpty())
        return -1;
    int normal = m_searchPattern.isEmpty()
        ? -1 : data.indexOf(m_searchPattern, from - offset);
    int hex = m_searchPatternHex.isEmpty()
        ? -1 : dataHex.indexOf(m_searchPatternHex, from - offset);

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

void BinEditor::drawChanges(QPainter *painter, int x, int y, const char *changes)
{
    const QBrush red(QColor(250, 150, 150));
    for (int i = 0; i < 16; ++i) {
        if (changes[i]) {
            painter->fillRect(x + i*m_columnWidth, y - m_ascent,
                2*m_charWidth, m_lineHeight, red);
        }
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

    int selStart, selEnd;
    if (m_cursorPosition >= m_anchorPosition) {
        selStart = m_anchorPosition;
        selEnd = m_cursorPosition;
    } else {
        selStart = m_cursorPosition;
        selEnd = m_anchorPosition + 1;
    }

    QString itemString(16*3, QLatin1Char(' '));
    QChar *itemStringData = itemString.data();
    char changedString[16] = { false };
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
                         addressString(m_baseAddr + uint(line) * 16));

        int cursor = -1;
        if (line * 16 <= m_cursorPosition && m_cursorPosition < line * 16 + 16)
            cursor = m_cursorPosition - line * 16;

        bool hasData = requestDataAt(line * 16);
        bool hasOldData = requestOldDataAt(line * 16);
        bool isOld = hasOldData && !hasData;

        QString printable;

        if (hasData || hasOldData) {
            for (int c = 0; c < 16; ++c) {
                int pos = line * 16 + c;
                if (pos >= m_size)
                    break;
                QChar qc(QLatin1Char(dataAt(pos, isOld)));
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
        bool somethingChanged = false;

        if (hasData || hasOldData) {
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


                const uchar value = uchar(dataAt(pos, isOld));
                itemStringData[c*3] = hex[value >> 4];
                itemStringData[c*3+1] = hex[value & 0xf];
                if (hasOldData && !isOld && value != uchar(dataAt(pos, true))) {
                    changedString[c] = true;
                    somethingChanged = true;
                }

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
        bool cursorWanted = m_cursorPosition == m_anchorPosition;

        if (isFullySelected) {
            painter.save();
            painter.fillRect(x, y-m_ascent, 16*m_columnWidth, m_lineHeight, palette().highlight());
            painter.setPen(palette().highlightedText().color());
            drawItems(&painter, x, y, itemString);
            painter.restore();
        } else {
            if (somethingChanged)
                drawChanges(&painter, x, y, changedString);
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


        if (cursor >= 0 && cursorWanted) {
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
        } else {
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

        if (cursor >= 0 && !printable.isEmpty() && cursorWanted) {
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
    m_baseAddr = 0;
    m_data.clear();
    m_oldData.clear();
    m_modifiedData.clear();
    m_requests.clear();
    m_size = 0;
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

bool BinEditor::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        switch (static_cast<QKeyEvent*>(e)->key()) {
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            m_hexCursor = !m_hexCursor;
            setBlinkingCursorEnabled(true);
            ensureCursorVisible();
            e->accept();
            return true;
        case Qt::Key_Down: {
            const QScrollBar * const scrollBar = verticalScrollBar();
            if (scrollBar->value() >= scrollBar->maximum() - 1) {
                emit newRangeRequested(editor(), baseAddress() + m_size);
                return true;
            }
            break;
        }
        default:;
        }
    } else if (e->type() == QEvent::ToolTip) {
        const QHelpEvent * const helpEvent = static_cast<QHelpEvent *>(e);
        bool hide = true;
        int selStart = selectionStart();
        int selEnd = selectionEnd();
        int byteCount = selEnd - selStart;
        if (byteCount <= 0) {
            selStart = m_cursorPosition;
            selStart = posAt(helpEvent->pos());
            selEnd = selStart + 1;
            byteCount = 1;
        }
        if (m_hexCursor && byteCount <= 8) {
            const QPoint &startPoint = offsetToPos(selStart);
            const QPoint &endPoint = offsetToPos(selEnd);
            const QPoint expandedEndPoint
                = QPoint(endPoint.x(), endPoint.y() + m_lineHeight);
            const QRect selRect(startPoint, expandedEndPoint);
            const QPoint &mousePos = helpEvent->pos();
            if (selRect.contains(mousePos)) {
                quint64 beValue, leValue;
                quint64 beValueOld, leValueOld;
                asIntegers(selStart, byteCount, beValue, leValue);
                asIntegers(selStart, byteCount, beValueOld, leValueOld, true);
                QString leSigned;
                QString beSigned;
                QString leSignedOld;
                QString beSignedOld;
                switch (byteCount) {
                case 8: case 7: case 6: case 5:
                    leSigned = QString::number(static_cast<qint64>(leValue));
                    beSigned = QString::number(static_cast<qint64>(beValue));
                    leSignedOld = QString::number(static_cast<qint64>(leValueOld));
                    beSignedOld = QString::number(static_cast<qint64>(beValueOld));
                    break;
                case 4: case 3:
                    leSigned = QString::number(static_cast<qint32>(leValue));
                    beSigned = QString::number(static_cast<qint32>(beValue));
                    leSignedOld = QString::number(static_cast<qint32>(leValueOld));
                    beSignedOld = QString::number(static_cast<qint32>(beValueOld));
                    break;
                case 2:
                    leSigned = QString::number(static_cast<qint16>(leValue));
                    beSigned = QString::number(static_cast<qint16>(beValue));
                    leSignedOld = QString::number(static_cast<qint16>(leValueOld));
                    beSignedOld = QString::number(static_cast<qint16>(beValueOld));
                    break;
                case 1:
                    leSigned = QString::number(static_cast<qint8>(leValue));
                    beSigned = QString::number(static_cast<qint8>(beValue));
                    leSignedOld = QString::number(static_cast<qint8>(leValueOld));
                    beSignedOld = QString::number(static_cast<qint8>(beValueOld));
                    break;
                }
                hide = false;
                //int pos = posAt(mousePos);
                //uchar old = dataAt(pos, true);
                //uchar current = dataAt(pos, false);

                QString msg = 
                    tr("Decimal unsigned value (little endian): %1\n"
                       "Decimal unsigned value (big endian): %2\n"
                       "Decimal signed value (little endian): %3\n"
                       "Decimal signed value (big endian): %4")
                       .arg(QString::number(leValue))
                       .arg(QString::number(beValue))
                       .arg(leSigned)
                       .arg(beSigned);
                if (beValue != beValueOld) {
                    msg += QLatin1Char('\n');
                    msg += tr("Previous decimal unsigned value (little endian): %1\n"
                       "Previous decimal unsigned value (big endian): %2\n"
                       "Previous decimal signed value (little endian): %3\n"
                       "Previous decimal signed value (big endian): %4")
                       .arg(QString::number(leValueOld))
                       .arg(QString::number(beValueOld))
                       .arg(leSignedOld)
                       .arg(beSignedOld);
                }
                QToolTip::showText(helpEvent->globalPos(), msg, this);
            }
        }
        if (hide)
            QToolTip::hideText();
        e->accept();
        return true;
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
        if (e->modifiers() & Qt::ControlModifier) {
            emit startOfFileRequested(editor());
        } else {
            setCursorPosition(m_cursorPosition/16 * 16, moveMode);
        }
        break;
    case Qt::Key_End:
        if (e->modifiers() & Qt::ControlModifier) {
            emit endOfFileRequested(editor());
        } else {
            setCursorPosition(m_cursorPosition/16 * 16 + 15, moveMode);
        }
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

void BinEditor::copy(bool raw)
{
    int selStart = selectionStart();
    int selEnd = selectionEnd();
    if (selStart >= selEnd)
        qSwap(selStart, selEnd);

    const int selectionLength = selEnd - selStart;
    if (selectionLength >> 22) {
        QMessageBox::warning(this, tr("Copying Failed"),
                             tr("You cannot copy more than 4 MB of binary data."));
        return;
    }
    const QByteArray &data = dataMid(selStart, selectionLength);
    if (raw) {
        QApplication::clipboard()->setText(data);
        return;
    }
    QString hexString;
    const char * const hex = "0123456789abcdef";
    hexString.reserve(3 * data.size());
    for (int i = 0; i < data.size(); ++i) {
        const uchar val = static_cast<uchar>(data[i]);
        hexString.append(hex[val >> 4]).append(hex[val & 0xf]).append(' ');
    }
    hexString.chop(1);
    QApplication::clipboard()->setText(hexString);
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

    if (!highNibble
            && !m_undoStack.isEmpty()
            && m_undoStack.top().position == position
            && m_undoStack.top().highNibble) {
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

void BinEditor::contextMenuEvent(QContextMenuEvent *event)
{
    const int selStart = selectionStart();
    const int byteCount = selectionEnd() - selStart;
    if (byteCount == 0)
        return;

    QMenu contextMenu;
    QAction copyAsciiAction(tr("Copy Selection as ASCII Characters"), this);
    QAction copyHexAction(tr("Copy Selection as Hex Values"), this);
    QAction jumpToBeAddressHere(this);
    QAction jumpToBeAddressNewWindow(this);
    QAction jumpToLeAddressHere(this);
    QAction jumpToLeAddressNewWindow(this);
    contextMenu.addAction(&copyAsciiAction);
    contextMenu.addAction(&copyHexAction);

    quint64 beAddress = 0;
    quint64 leAddress = 0;
    if (byteCount <= 8) {
        asIntegers(selStart, byteCount, beAddress, leAddress);
        setupJumpToMenuAction(&contextMenu, &jumpToBeAddressHere,
                              &jumpToBeAddressNewWindow, beAddress);

        // If the menu entries would be identical, show only one of them.
        if (beAddress != leAddress) {
            setupJumpToMenuAction(&contextMenu, &jumpToLeAddressHere,
                              &jumpToLeAddressNewWindow, leAddress);
        }
    } else {
        jumpToBeAddressHere.setText(tr("Jump to Address in This Window"));
        jumpToBeAddressNewWindow.setText(tr("Jump to Address in New Window"));
        jumpToBeAddressHere.setEnabled(false);
        jumpToBeAddressNewWindow.setEnabled(false);
        contextMenu.addAction(&jumpToBeAddressHere);
        contextMenu.addAction(&jumpToBeAddressNewWindow);
    }

    QAction *action = contextMenu.exec(event->globalPos());
    if (action == &copyAsciiAction)
        copy(true);
    else if (action == &copyHexAction)
        copy(false);
    else if (action == &jumpToBeAddressHere)
        jumpToAddress(beAddress);
    else if (action == &jumpToLeAddressHere)
        jumpToAddress(leAddress);
    else if (action == &jumpToBeAddressNewWindow)
        emit newWindowRequested(beAddress);
    else if (action == &jumpToLeAddressNewWindow)
        emit newWindowRequested(leAddress);
}

void BinEditor::setupJumpToMenuAction(QMenu *menu, QAction *actionHere,
                                      QAction *actionNew, quint64 addr)
{
    actionHere->setText(tr("Jump to Address 0x%1 in This Window")
                        .arg(QString::number(addr, 16)));
    actionNew->setText(tr("Jump to Address 0x%1 in New Window")
                        .arg(QString::number(addr, 16)));
    menu->addAction(actionHere);
    menu->addAction(actionNew);
    if (!m_canRequestNewWindow)
        actionNew->setEnabled(false);
}

void BinEditor::jumpToAddress(quint64 address)
{
    if (address >= m_baseAddr && address < m_baseAddr + m_size)
        setCursorPosition(address - m_baseAddr);
    else
        emit newRangeRequested(editor(), address);
}

void BinEditor::setNewWindowRequestAllowed()
{
    m_canRequestNewWindow = true;
}

void BinEditor::updateContents()
{
    m_oldData = m_data;
    m_data.clear();
    setSizes(baseAddress() + cursorPosition(), m_size, m_blockSize);
}

QPoint BinEditor::offsetToPos(int offset)
{
    const int x = m_labelWidth + (offset % 16) * m_columnWidth;
    const int y = (offset / 16  - verticalScrollBar()->value()) * m_lineHeight;
    return QPoint(x, y);
}

void BinEditor::asIntegers(int offset, int count, quint64 &beValue,
    quint64 &leValue, bool old)
{
    beValue = leValue = 0;
    const QByteArray &data = dataMid(offset, count, old);
    for (int pos = 0; pos < data.size(); ++pos) {
        const quint64 val = static_cast<quint64>(data.at(pos)) & 0xff;
        beValue += val << (pos * 8);
        leValue += val << ((count - pos - 1) * 8);
    }
}

bool BinEditor::isMemoryView() const
{
    return editor()->property("MemoryView").toBool();
}

} // namespace BINEditor
