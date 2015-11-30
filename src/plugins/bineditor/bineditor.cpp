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

#include "bineditor.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QByteArrayMatcher>
#include <QDebug>
#include <QFile>
#include <QTemporaryFile>
#include <QVariant>

#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QScrollBar>
#include <QToolTip>
#include <QWheelEvent>

static QByteArray calculateHexPattern(const QByteArray &pattern)
{
    QByteArray result;
    if (pattern.size() % 2 == 0) {
        bool ok = true;
        int i = 0;
        while (i < pattern.size()) {
            ushort s = pattern.mid(i, 2).toUShort(&ok, 16);
            if (!ok)
                return QByteArray();
            result.append(s);
            i += 2;
        }
    }
    return result;
}

namespace BinEditor {

BinEditorWidget::BinEditorWidget(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    m_bytesPerLine = 16;
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
    setFrameStyle(QFrame::Plain);

    // Font settings
    setFontSettings(TextEditor::TextEditorSettings::fontSettings());
    connect(TextEditor::TextEditorSettings::instance(),
            SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            this, SLOT(setFontSettings(TextEditor::FontSettings)));

}

BinEditorWidget::~BinEditorWidget()
{
}

void BinEditorWidget::init()
{
    const int addressStringWidth =
        2*m_addressBytes + (m_addressBytes - 1) / 2;
    m_addressString = QString(addressStringWidth, QLatin1Char(':'));
    QFontMetrics fm(fontMetrics());
    m_descent = fm.descent();
    m_ascent = fm.ascent();
    m_lineHeight = fm.lineSpacing();
    m_charWidth = fm.width(QChar(QLatin1Char('M')));
    m_margin = m_charWidth;
    m_columnWidth = 2 * m_charWidth + fm.width(QChar(QLatin1Char(' ')));
    m_numLines = m_size / m_bytesPerLine + 1;
    m_numVisibleLines = viewport()->height() / m_lineHeight;
    m_textWidth = m_bytesPerLine * m_charWidth + m_charWidth;
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

    if (m_isMonospacedFont && fm.width(QLatin1String("M M ")) != m_charWidth * 4) {
        // On Qt/Mac, monospace font widths may have a fractional component
        // This breaks the assumption that width("MMM") == width('M') * 3

        m_isMonospacedFont = false;
        m_columnWidth = fm.width(QLatin1String("MMM"));
        m_labelWidth = m_addressBytes == 4
            ? fm.width(QLatin1String("MMMM:MMMM"))
            : fm.width(QLatin1String("MMMM:MMMM:MMMM:MMMM"));
    }

    horizontalScrollBar()->setRange(0, 2 * m_margin + m_bytesPerLine * m_columnWidth
                                    + m_labelWidth + m_textWidth - viewport()->width());
    horizontalScrollBar()->setPageStep(viewport()->width());
    verticalScrollBar()->setRange(0, m_numLines - m_numVisibleLines);
    verticalScrollBar()->setPageStep(m_numVisibleLines);
    ensureCursorVisible();
}


void BinEditorWidget::addData(quint64 block, const QByteArray &data)
{
    QTC_ASSERT(data.size() == m_blockSize, return);
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

bool BinEditorWidget::requestDataAt(int pos) const
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
        emit const_cast<BinEditorWidget*>(this)->
            dataRequested(m_baseAddr / m_blockSize + block);
        return true;
    }
    return false;
}

bool BinEditorWidget::requestOldDataAt(int pos) const
{
    int block = pos / m_blockSize;
    BlockMap::const_iterator it = m_oldData.find(block);
    return it != m_oldData.end();
}

char BinEditorWidget::dataAt(int pos, bool old) const
{
    int block = pos / m_blockSize;
    return blockData(block, old).at(pos - block*m_blockSize);
}

void BinEditorWidget::changeDataAt(int pos, char c)
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

    emit dataChanged(m_baseAddr + pos, QByteArray(1, c));
}

QByteArray BinEditorWidget::dataMid(int from, int length, bool old) const
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

QByteArray BinEditorWidget::blockData(int block, bool old) const
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

void BinEditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    setFont(fs.toTextCharFormat(TextEditor::C_TEXT).font());
}

void BinEditorWidget::setBlinkingCursorEnabled(bool enable)
{
    if (enable && QApplication::cursorFlashTime() > 0)
        m_cursorBlinkTimer.start(QApplication::cursorFlashTime() / 2, this);
    else
        m_cursorBlinkTimer.stop();
    m_cursorVisible = enable;
    updateLines();
}

void BinEditorWidget::focusInEvent(QFocusEvent *)
{
    setBlinkingCursorEnabled(true);
}

void BinEditorWidget::focusOutEvent(QFocusEvent *)
{
    setBlinkingCursorEnabled(false);
}

void BinEditorWidget::timerEvent(QTimerEvent *e)
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


void BinEditorWidget::setModified(bool modified)
{
    int unmodifiedState = modified ? -1 : m_undoStack.size();
    if (unmodifiedState == m_unmodifiedState)
        return;
    m_unmodifiedState = unmodifiedState;
    emit modificationChanged(m_undoStack.size() != m_unmodifiedState);
}

bool BinEditorWidget::isModified() const
{
    return (m_undoStack.size() != m_unmodifiedState);
}

void BinEditorWidget::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

bool BinEditorWidget::isReadOnly() const
{
    return m_readOnly;
}

bool BinEditorWidget::save(QString *errorString, const QString &oldFileName, const QString &newFileName)
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
    Utils::FileSaver saver(newFileName, QIODevice::ReadWrite); // QtBug: WriteOnly truncates.
    if (!saver.hasError()) {
        QFile *output = saver.file();
        const qint64 size = output->size();
        for (BlockMap::const_iterator it = m_modifiedData.constBegin();
            it != m_modifiedData.constEnd(); ++it) {
            if (!saver.setResult(output->seek(it.key() * m_blockSize)))
                break;
            if (!saver.write(it.value()))
                break;
            if (!saver.setResult(output->flush()))
                break;
        }

        // We may have padded the displayed data, so we have to make sure
        // changes to that area are not actually written back to disk.
        if (!saver.hasError())
            saver.setResult(output->resize(size));
    }
    if (!saver.finalize(errorString))
        return false;

    setModified(false);
    return true;
}

void BinEditorWidget::setSizes(quint64 startAddr, int range, int blockSize)
{
    int newBlockSize = blockSize;
    QTC_ASSERT((blockSize/m_bytesPerLine) * m_bytesPerLine == blockSize,
               blockSize = (blockSize/m_bytesPerLine + 1) * m_bytesPerLine);
    // Users can edit data in the range
    // [startAddr - range/2, startAddr + range/2].
    quint64 newBaseAddr = quint64(range/2) > startAddr ? 0 : startAddr - range/2;
    newBaseAddr = (newBaseAddr / blockSize) * blockSize;

    const quint64 maxRange = Q_UINT64_C(0xffffffffffffffff) - newBaseAddr + 1;
    int newSize = newBaseAddr != 0 && quint64(range) >= maxRange
              ? maxRange : range;
    int newAddressBytes = (newBaseAddr + newSize < quint64(1) << 32
                   && newBaseAddr + newSize >= newBaseAddr) ? 4 : 8;



    if (newBlockSize == m_blockSize
            && newBaseAddr == m_baseAddr
            && newSize == m_size
            && newAddressBytes == m_addressBytes)
        return;

    m_blockSize = blockSize;
    m_emptyBlock = QByteArray(blockSize, '\0');
    m_modifiedData.clear();
    m_requests.clear();

    m_baseAddr = newBaseAddr;
    m_size = newSize;
    m_addressBytes = newAddressBytes;

    m_unmodifiedState = 0;
    m_undoStack.clear();
    m_redoStack.clear();
    init();

    setCursorPosition(startAddr - m_baseAddr);
    viewport()->update();
}

void BinEditorWidget::resizeEvent(QResizeEvent *)
{
    init();
}

void BinEditorWidget::scrollContentsBy(int dx, int dy)
{
    viewport()->scroll(isRightToLeft() ? -dx : dx, dy * m_lineHeight);
    const QScrollBar * const scrollBar = verticalScrollBar();
    const int scrollPos = scrollBar->value();
    if (dy <= 0 && scrollPos == scrollBar->maximum())
        emit newRangeRequested(baseAddress() + m_size);
    else if (dy >= 0 && scrollPos == scrollBar->minimum())
        emit newRangeRequested(baseAddress());
}

void BinEditorWidget::changeEvent(QEvent *e)
{
    QAbstractScrollArea::changeEvent(e);
    if (e->type() == QEvent::ActivationChange) {
        if (!isActiveWindow())
            m_autoScrollTimer.stop();
    }
    init();
    viewport()->update();
}


void BinEditorWidget::wheelEvent(QWheelEvent *e)
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



QRect BinEditorWidget::cursorRect() const
{
    int topLine = verticalScrollBar()->value();
    int line = m_cursorPosition / m_bytesPerLine;
    int y = (line - topLine) * m_lineHeight;
    int xoffset = horizontalScrollBar()->value();
    int column = m_cursorPosition % m_bytesPerLine;
    int x = m_hexCursor
            ? (-xoffset + m_margin + m_labelWidth + column * m_columnWidth)
            : (-xoffset + m_margin + m_labelWidth + m_bytesPerLine * m_columnWidth
               + m_charWidth + column * m_charWidth);
    int w = m_hexCursor ? m_columnWidth : m_charWidth;
    return QRect(x, y, w, m_lineHeight);
}

int BinEditorWidget::posAt(const QPoint &pos) const
{
    int xoffset = horizontalScrollBar()->value();
    int x = xoffset + pos.x() - m_margin - m_labelWidth;
    int column = qMin(15, qMax(0,x) / m_columnWidth);
    int topLine = verticalScrollBar()->value();
    int line = pos.y() / m_lineHeight;


    if (x > m_bytesPerLine * m_columnWidth + m_charWidth/2) {
        x -= m_bytesPerLine * m_columnWidth + m_charWidth;
        for (column = 0; column < 15; ++column) {
            int dataPos = (topLine + line) * m_bytesPerLine + column;
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

    return qMin(m_size, qMin(m_numLines, topLine + line) * m_bytesPerLine) + column;
}

bool BinEditorWidget::inTextArea(const QPoint &pos) const
{
    int xoffset = horizontalScrollBar()->value();
    int x = xoffset + pos.x() - m_margin - m_labelWidth;
    return (x > m_bytesPerLine * m_columnWidth + m_charWidth/2);
}

void BinEditorWidget::updateLines()
{
    updateLines(m_cursorPosition, m_cursorPosition);
}

void BinEditorWidget::updateLines(int fromPosition, int toPosition)
{
    int topLine = verticalScrollBar()->value();
    int firstLine = qMin(fromPosition, toPosition) / m_bytesPerLine;
    int lastLine = qMax(fromPosition, toPosition) / m_bytesPerLine;
    int y = (firstLine - topLine) * m_lineHeight;
    int h = (lastLine - firstLine + 1 ) * m_lineHeight;

    viewport()->update(0, y, viewport()->width(), h);
}

int BinEditorWidget::dataIndexOf(const QByteArray &pattern, int from, bool caseSensitive) const
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
            buffer = buffer.toLower();

        int pos = matcher.indexIn(buffer, from - (block * m_blockSize) + trailing);
        if (pos >= 0)
            return pos + block * m_blockSize - trailing;
        ++block;
        from = block * m_blockSize - trailing;
    }
    return end == m_size ? -1 : -2;
}

int BinEditorWidget::dataLastIndexOf(const QByteArray &pattern, int from, bool caseSensitive) const
{
    int trailing = pattern.size();
    if (trailing > m_blockSize)
        return -1;

    QByteArray buffer;
    buffer.resize(m_blockSize + trailing);
    char *b = buffer.data();

    if (from == -1)
        from = m_size;
    int block = from / m_blockSize;
    const int lowerBound = qMax(0, from - SearchStride);
    while (from > lowerBound) {
        if (!requestDataAt(block * m_blockSize))
            return -1;
        QByteArray data = blockData(block);
        ::memcpy(b + m_blockSize, b, trailing);
        ::memcpy(b, data.constData(), m_blockSize);

        if (!caseSensitive)
            buffer = buffer.toLower();

        int pos = buffer.lastIndexOf(pattern, from - (block * m_blockSize));
        if (pos >= 0)
            return pos + block * m_blockSize;
        --block;
        from = block * m_blockSize + (m_blockSize-1) + trailing;
    }
    return lowerBound == 0 ? -1 : -2;
}


int BinEditorWidget::find(const QByteArray &pattern_arg, int from,
                    QTextDocument::FindFlags findFlags)
{
    if (pattern_arg.isEmpty())
        return 0;

    QByteArray pattern = pattern_arg;

    bool caseSensitiveSearch = (findFlags & QTextDocument::FindCaseSensitively);

    if (!caseSensitiveSearch)
        pattern = pattern.toLower();

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
        setCursorPosition(pos + (found == pos ? pattern.size() : hexPattern.size()) - 1, KeepAnchor);
    }
    return pos;
}

int BinEditorWidget::findPattern(const QByteArray &data, const QByteArray &dataHex,
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


void BinEditorWidget::drawItems(QPainter *painter, int x, int y, const QString &itemString)
{
    if (m_isMonospacedFont) {
        painter->drawText(x, y, itemString);
    } else {
        for (int i = 0; i < m_bytesPerLine; ++i)
            painter->drawText(x + i*m_columnWidth, y, itemString.mid(i*3, 2));
    }
}

void BinEditorWidget::drawChanges(QPainter *painter, int x, int y, const char *changes)
{
    const QBrush red(QColor(250, 150, 150));
    for (int i = 0; i < m_bytesPerLine; ++i) {
        if (changes[i]) {
            painter->fillRect(x + i*m_columnWidth, y - m_ascent,
                2*m_charWidth, m_lineHeight, red);
        }
    }
}

QString BinEditorWidget::addressString(quint64 address)
{
    QChar *addressStringData = m_addressString.data();
    const char *hex = "0123456789abcdef";

    // Take colons into account.
    const int indices[16] = {
        0, 1, 2, 3, 5, 6, 7, 8, 10, 11, 12, 13, 15, 16, 17, 18
    };

    for (int b = 0; b < m_addressBytes; ++b) {
        addressStringData[indices[2*m_addressBytes - 1 - b*2]] =
            QLatin1Char(hex[(address >> (8*b)) & 0xf]);
        addressStringData[indices[2*m_addressBytes - 2 - b*2]] =
            QLatin1Char(hex[(address >> (8*b + 4)) & 0xf]);
    }
    return m_addressString;
}

static void paintCursorBorder(QPainter *painter, const QRect &cursorRect)
{
    painter->save();
    QPen borderPen(Qt::red);
    borderPen.setJoinStyle(Qt::MiterJoin);
    painter->setPen(borderPen);
    painter->drawRect(QRectF(cursorRect).adjusted(0.5, 0.5, -0.5, -0.5));
    painter->restore();
}

void BinEditorWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter(viewport());
    const int topLine = verticalScrollBar()->value();
    const int xoffset = horizontalScrollBar()->value();
    const int x1 = -xoffset + m_margin + m_labelWidth - m_charWidth/2 - 1;
    const int x2 = -xoffset + m_margin + m_labelWidth + m_bytesPerLine * m_columnWidth + m_charWidth/2;
    painter.drawLine(x1, 0, x1, viewport()->height());
    painter.drawLine(x2, 0, x2, viewport()->height());

    int viewport_height = viewport()->height();
    for (int i = 0; i < 8; ++i) {
        int bg_x = -xoffset +  m_margin + (2 * i + 1) * m_columnWidth + m_labelWidth;
        QRect r(bg_x - m_charWidth/2, 0, m_columnWidth, viewport_height);
        painter.fillRect(e->rect() & r, palette().alternateBase());
    }

    int matchLength = 0;

    QByteArray patternData, patternDataHex;
    int patternOffset = qMax(0, topLine*m_bytesPerLine - m_searchPattern.size());
    if (!m_searchPattern.isEmpty()) {
        patternData = dataMid(patternOffset, m_numVisibleLines * m_bytesPerLine + (topLine*m_bytesPerLine - patternOffset));
        patternDataHex = patternData;
        if (!m_caseSensitiveSearch)
            patternData = patternData.toLower();
    }

    int foundPatternAt = findPattern(patternData, patternDataHex, patternOffset, patternOffset, &matchLength);

    int selStart, selEnd;
    if (m_cursorPosition >= m_anchorPosition) {
        selStart = m_anchorPosition;
        selEnd = m_cursorPosition;
    } else {
        selStart = m_cursorPosition;
        selEnd = m_anchorPosition;
    }

    QString itemString(m_bytesPerLine*3, QLatin1Char(' '));
    QChar *itemStringData = itemString.data();
    char changedString[160] = { false };
    QTC_ASSERT((size_t)m_bytesPerLine < sizeof(changedString), return);
    const char *hex = "0123456789abcdef";

    painter.setPen(palette().text().color());
    const QFontMetrics &fm = painter.fontMetrics();
    for (int i = 0; i <= m_numVisibleLines; ++i) {
        int line = topLine + i;
        if (line >= m_numLines)
            break;

        const quint64 lineAddress = m_baseAddr + uint(line) * m_bytesPerLine;
        int y = i * m_lineHeight + m_ascent;
        if (y - m_ascent > e->rect().bottom())
            break;
        if (y + m_descent < e->rect().top())
            continue;

        painter.drawText(-xoffset, i * m_lineHeight + m_ascent,
                         addressString(lineAddress));

        int cursor = -1;
        if (line * m_bytesPerLine <= m_cursorPosition
                && m_cursorPosition < line * m_bytesPerLine + m_bytesPerLine)
            cursor = m_cursorPosition - line * m_bytesPerLine;

        bool hasData = requestDataAt(line * m_bytesPerLine);
        bool hasOldData = requestOldDataAt(line * m_bytesPerLine);
        bool isOld = hasOldData && !hasData;

        QString printable;

        if (hasData || hasOldData) {
            for (int c = 0; c < m_bytesPerLine; ++c) {
                int pos = line * m_bytesPerLine + c;
                if (pos >= m_size)
                    break;
                QChar qc(QLatin1Char(dataAt(pos, isOld)));
                if (qc.unicode() >= 127 || !qc.isPrint())
                    qc = 0xB7;
                printable += qc;
            }
        } else {
            printable = QString(m_bytesPerLine, QLatin1Char(' '));
        }

        QRect selectionRect;
        QRect printableSelectionRect;

        bool isFullySelected = (selStart < selEnd && selStart <= line*m_bytesPerLine && (line+1)*m_bytesPerLine <= selEnd);
        bool somethingChanged = false;

        if (hasData || hasOldData) {
            for (int c = 0; c < m_bytesPerLine; ++c) {
                int pos = line * m_bytesPerLine + c;
                if (pos >= m_size) {
                    while (c < m_bytesPerLine) {
                        itemStringData[c*3] = itemStringData[c*3+1] = QLatin1Char(' ');
                        ++c;
                    }
                    break;
                }
                if (foundPatternAt >= 0 && pos >= foundPatternAt + matchLength)
                    foundPatternAt = findPattern(patternData, patternDataHex, foundPatternAt + matchLength, patternOffset, &matchLength);


                const uchar value = uchar(dataAt(pos, isOld));
                itemStringData[c*3] = QLatin1Char(hex[value >> 4]);
                itemStringData[c*3+1] = QLatin1Char(hex[value & 0xf]);
                if (hasOldData && !isOld && value != uchar(dataAt(pos, true))) {
                    changedString[c] = true;
                    somethingChanged = true;
                }

                int item_x = -xoffset +  m_margin + c * m_columnWidth + m_labelWidth;

                QColor color;
                foreach (const Markup &m, m_markup) {
                    if (m.covers(lineAddress + c)) {
                        color = m.color;
                        break;
                    }
                }
                if (foundPatternAt >= 0 && pos >= foundPatternAt && pos < foundPatternAt + matchLength)
                    color = QColor(0xffef0b);

                if (color.isValid()) {
                    painter.fillRect(item_x - m_charWidth/2, y-m_ascent, m_columnWidth, m_lineHeight, color);
                    int printable_item_x = -xoffset + m_margin + m_labelWidth + m_bytesPerLine * m_columnWidth + m_charWidth
                                           + fm.width(printable.left(c));
                    painter.fillRect(printable_item_x, y-m_ascent,
                                     fm.width(printable.at(c)),
                                     m_lineHeight, color);
                }

                if (!isFullySelected && pos >= selStart && pos <= selEnd) {
                    selectionRect |= QRect(item_x - m_charWidth/2, y-m_ascent, m_columnWidth, m_lineHeight);
                    int printable_item_x = -xoffset + m_margin + m_labelWidth + m_bytesPerLine * m_columnWidth + m_charWidth
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
            painter.fillRect(x - m_charWidth/2, y-m_ascent, m_bytesPerLine*m_columnWidth, m_lineHeight, palette().highlight());
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

        if (cursor >= 0) {
            int w = fm.boundingRect(itemString.mid(cursor*3, 2)).width();
            QRect cursorRect(x + cursor * m_columnWidth, y - m_ascent, w + 1, m_lineHeight);
            paintCursorBorder(&painter, cursorRect);
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

        int text_x = -xoffset + m_margin + m_labelWidth + m_bytesPerLine * m_columnWidth + m_charWidth;

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

        if (cursor >= 0 && !printable.isEmpty()) {
            QRect cursorRect(text_x + fm.width(printable.left(cursor)),
                             y-m_ascent,
                             fm.width(printable.at(cursor)),
                             m_lineHeight);
            if (m_hexCursor || !m_cursorVisible) {
                paintCursorBorder(&painter, cursorRect);
            } else {
                painter.save();
                painter.setClipRect(cursorRect);
                painter.fillRect(cursorRect, Qt::red);
                painter.setPen(Qt::white);
                painter.drawText(text_x, y, printable);
                painter.restore();
            }
        }
    }
}


int BinEditorWidget::cursorPosition() const
{
    return m_cursorPosition;
}

void BinEditorWidget::setCursorPosition(int pos, MoveMode moveMode)
{
    pos = qMin(m_size-1, qMax(0, pos));
    int oldCursorPosition = m_cursorPosition;

    m_lowNibble = false;
    m_cursorPosition = pos;
    if (moveMode == MoveAnchor) {
        updateLines(m_anchorPosition, oldCursorPosition);
        m_anchorPosition = m_cursorPosition;
    }

    updateLines(oldCursorPosition, m_cursorPosition);
    ensureCursorVisible();
    emit cursorPositionChanged(m_cursorPosition);
}


void BinEditorWidget::ensureCursorVisible()
{
    QRect cr = cursorRect();
    QRect vr = viewport()->rect();
    if (!vr.contains(cr)) {
        if (cr.top() < vr.top())
            verticalScrollBar()->setValue(m_cursorPosition / m_bytesPerLine);
        else if (cr.bottom() > vr.bottom())
            verticalScrollBar()->setValue(m_cursorPosition / m_bytesPerLine - m_numVisibleLines + 1);
    }
}

void BinEditorWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;
    MoveMode moveMode = e->modifiers() & Qt::ShiftModifier ? KeepAnchor : MoveAnchor;
    setCursorPosition(posAt(e->pos()), moveMode);
    setBlinkingCursorEnabled(true);
    if (m_hexCursor == inTextArea(e->pos())) {
        m_hexCursor = !m_hexCursor;
        updateLines();
    }
}

void BinEditorWidget::mouseMoveEvent(QMouseEvent *e)
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

void BinEditorWidget::mouseReleaseEvent(QMouseEvent *)
{
    if (m_autoScrollTimer.isActive()) {
        m_autoScrollTimer.stop();
        ensureCursorVisible();
    }
}

void BinEditorWidget::selectAll()
{
    setCursorPosition(0);
    setCursorPosition(m_size-1, KeepAnchor);
}

void BinEditorWidget::clear()
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

bool BinEditorWidget::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::KeyPress:
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
            const int maximum = scrollBar->maximum();
            if (maximum && scrollBar->value() >= maximum - 1) {
                emit newRangeRequested(baseAddress() + m_size);
                return true;
            }
            break;
        }
        default:;
        }
        break;
    case QEvent::ToolTip: {
        const QHelpEvent *helpEvent = static_cast<const QHelpEvent *>(e);
        const QString tt = toolTip(helpEvent);
        if (tt.isEmpty())
            QToolTip::hideText();
        else
            QToolTip::showText(helpEvent->globalPos(), tt, this);
        e->accept();
        return true;
    }
    default:
        break;
    }

    return QAbstractScrollArea::event(e);
}

QString BinEditorWidget::toolTip(const QHelpEvent *helpEvent) const
{
    int selStart = selectionStart();
    int selEnd = selectionEnd();
    int byteCount = selEnd - selStart + 1;
    if (m_hexCursor == 0 || byteCount > 8)
        return QString();

    const QPoint &startPoint = offsetToPos(selStart);
    const QPoint &endPoint = offsetToPos(selEnd + 1);
    QRect selRect(startPoint, endPoint);
    selRect.setHeight(m_lineHeight);
    if (!selRect.contains(helpEvent->pos()))
        return QString();

    quint64 bigEndianValue, littleEndianValue;
    quint64 bigEndianValueOld, littleEndianValueOld;
    asIntegers(selStart, byteCount, bigEndianValue, littleEndianValue);
    asIntegers(selStart, byteCount, bigEndianValueOld, littleEndianValueOld, true);
    QString littleEndianSigned;
    QString bigEndianSigned;
    QString littleEndianSignedOld;
    QString bigEndianSignedOld;
    int intSize = 0;
    switch (byteCount) {
    case 8: case 7: case 6: case 5:
        littleEndianSigned = QString::number(static_cast<qint64>(littleEndianValue));
        bigEndianSigned = QString::number(static_cast<qint64>(bigEndianValue));
        littleEndianSignedOld = QString::number(static_cast<qint64>(littleEndianValueOld));
        bigEndianSignedOld = QString::number(static_cast<qint64>(bigEndianValueOld));
        intSize = 8;
        break;
    case 4: case 3:
        littleEndianSigned = QString::number(static_cast<qint32>(littleEndianValue));
        bigEndianSigned = QString::number(static_cast<qint32>(bigEndianValue));
        littleEndianSignedOld = QString::number(static_cast<qint32>(littleEndianValueOld));
        bigEndianSignedOld = QString::number(static_cast<qint32>(bigEndianValueOld));
        intSize = 4;
        break;
    case 2:
        littleEndianSigned = QString::number(static_cast<qint16>(littleEndianValue));
        bigEndianSigned = QString::number(static_cast<qint16>(bigEndianValue));
        littleEndianSignedOld = QString::number(static_cast<qint16>(littleEndianValueOld));
        bigEndianSignedOld = QString::number(static_cast<qint16>(bigEndianValueOld));
        intSize = 2;
        break;
    case 1:
        littleEndianSigned = QString::number(static_cast<qint8>(littleEndianValue));
        bigEndianSigned = QString::number(static_cast<qint8>(bigEndianValue));
        littleEndianSignedOld = QString::number(static_cast<qint8>(littleEndianValueOld));
        bigEndianSignedOld = QString::number(static_cast<qint8>(bigEndianValueOld));
        intSize = 1;
        break;
    }

    const quint64 address = m_baseAddr + selStart;
    const char tableRowStartC[] = "<tr><td>";
    const char tableRowEndC[] = "</td></tr>";
    const char numericTableRowSepC[] = "</td><td align=\"right\">";

    QString msg;
    QTextStream str(&msg);
    str << "<html><head/><body><p align=\"center\"><b>"
        << tr("Memory at 0x%1").arg(address, 0, 16) << "</b></p>";

    foreach (const Markup &m, m_markup) {
        if (m.covers(address) && !m.toolTip.isEmpty()) {
            str << "<p>" <<  m.toolTip << "</p><br>";
            break;
        }
    }
    const QString msgDecimalUnsigned = tr("Decimal&nbsp;unsigned&nbsp;value:");
    const QString msgDecimalSigned = tr("Decimal&nbsp;signed&nbsp;value:");
    const QString msgOldDecimalUnsigned = tr("Previous&nbsp;decimal&nbsp;unsigned&nbsp;value:");
    const QString msgOldDecimalSigned = tr("Previous&nbsp;decimal&nbsp;signed&nbsp;value:");

    // Table showing little vs. big endian integers for multi-byte
    if (intSize > 1) {
        str << "<table><tr><th>"
            << tr("%1-bit&nbsp;Integer&nbsp;Type").arg(8 * intSize) << "</th><th>"
            << tr("Little Endian") << "</th><th>" << tr("Big Endian") << "</th></tr>";
        str << tableRowStartC << msgDecimalUnsigned
            << numericTableRowSepC << littleEndianValue << numericTableRowSepC
            << bigEndianValue << tableRowEndC <<  tableRowStartC << msgDecimalSigned
            << numericTableRowSepC << littleEndianSigned << numericTableRowSepC
            << bigEndianSigned << tableRowEndC;
        if (bigEndianValue != bigEndianValueOld) {
            str << tableRowStartC << msgOldDecimalUnsigned
                << numericTableRowSepC << littleEndianValueOld << numericTableRowSepC
                << bigEndianValueOld << tableRowEndC << tableRowStartC
                << msgOldDecimalSigned << numericTableRowSepC << littleEndianSignedOld
                << numericTableRowSepC << bigEndianSignedOld << tableRowEndC;
        }
        str << "</table>";
    }

    switch (byteCount) {
    case 1:
        // 1 byte: As octal, decimal, etc.
        str << "<table>";
        str << tableRowStartC << msgDecimalUnsigned << numericTableRowSepC
            << littleEndianValue << tableRowEndC;
        if (littleEndianValue & 0x80) {
            str << tableRowStartC << msgDecimalSigned << numericTableRowSepC
                << littleEndianSigned << tableRowEndC;
        }
        str << tableRowStartC << tr("Binary&nbsp;value:") << numericTableRowSepC;
        str.setIntegerBase(2);
        str.setFieldWidth(8);
        str.setPadChar(QLatin1Char('0'));
        str << littleEndianValue;
        str.setFieldWidth(0);
        str << tableRowEndC << tableRowStartC
            << tr("Octal&nbsp;value:") << numericTableRowSepC;
        str.setIntegerBase(8);
        str.setFieldWidth(3);
        str << littleEndianValue << tableRowEndC;
        str.setIntegerBase(10);
        str.setFieldWidth(0);
        if (littleEndianValue != littleEndianValueOld) {
            str << tableRowStartC << msgOldDecimalUnsigned << numericTableRowSepC
                << littleEndianValueOld << tableRowEndC;
            if (littleEndianValueOld & 0x80) {
                str << tableRowStartC << msgOldDecimalSigned << numericTableRowSepC
                    << littleEndianSignedOld << tableRowEndC;
            }
            str << tableRowStartC << tr("Previous&nbsp;binary&nbsp;value:")
                << numericTableRowSepC;
            str.setIntegerBase(2);
            str.setFieldWidth(8);
            str << littleEndianValueOld;
            str.setFieldWidth(0);
            str << tableRowEndC << tableRowStartC << tr("Previous&nbsp;octal&nbsp;value:")
                << numericTableRowSepC;
            str.setIntegerBase(8);
            str.setFieldWidth(3);
            str << littleEndianValueOld << tableRowEndC;
        }
        str.setIntegerBase(10);
        str.setFieldWidth(0);
        str << "</table>";
        break;
    // Double value
    case sizeof(double): {
        str << "<br><table>";
        double doubleValue, doubleValueOld;
        asDouble(selStart, doubleValue, false);
        asDouble(selStart, doubleValueOld, true);
        str << tableRowStartC << tr("<i>double</i>&nbsp;value:") << numericTableRowSepC
            << doubleValue << tableRowEndC;
        if (doubleValue != doubleValueOld)
            str << tableRowStartC << tr("Previous <i>double</i>&nbsp;value:") << numericTableRowSepC
                << doubleValueOld << tableRowEndC;
        str << "</table>";
    }
    break;
    // Float value
    case sizeof(float): {
        str << "<br><table>";
        float floatValue, floatValueOld;
        asFloat(selStart, floatValue, false);
        asFloat(selStart, floatValueOld, true);
        str << tableRowStartC << tr("<i>float</i>&nbsp;value:") << numericTableRowSepC
            << floatValue << tableRowEndC;
        if (floatValue != floatValueOld)
            str << tableRowStartC << tr("Previous <i>float</i>&nbsp;value:") << numericTableRowSepC
                << floatValueOld << tableRowEndC;

        str << "</table>";
    }
    break;
    }
    str << "</body></html>";
    return msg;
}

void BinEditorWidget::keyPressEvent(QKeyEvent *e)
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
    bool ctrlPressed = e->modifiers() & Qt::ControlModifier;
    switch (e->key()) {
    case Qt::Key_Up:
        if (ctrlPressed)
            verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepSub);
        else
            setCursorPosition(m_cursorPosition - m_bytesPerLine, moveMode);
        break;
    case Qt::Key_Down:
        if (ctrlPressed)
            verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepAdd);
        else
            setCursorPosition(m_cursorPosition + m_bytesPerLine, moveMode);
        break;
    case Qt::Key_Right:
        setCursorPosition(m_cursorPosition + 1, moveMode);
        break;
    case Qt::Key_Left:
        setCursorPosition(m_cursorPosition - 1, moveMode);
        break;
    case Qt::Key_PageUp:
    case Qt::Key_PageDown: {
        int line = qMax(0, m_cursorPosition / m_bytesPerLine - verticalScrollBar()->value());
        verticalScrollBar()->triggerAction(e->key() == Qt::Key_PageUp ?
                                           QScrollBar::SliderPageStepSub : QScrollBar::SliderPageStepAdd);
        if (!ctrlPressed)
            setCursorPosition((verticalScrollBar()->value() + line) * m_bytesPerLine + m_cursorPosition % m_bytesPerLine, moveMode);
    } break;

    case Qt::Key_Home: {
        int pos;
        if (ctrlPressed)
            pos = 0;
        else
            pos = m_cursorPosition/m_bytesPerLine * m_bytesPerLine;
        setCursorPosition(pos, moveMode);
    } break;
    case Qt::Key_End: {
        int pos;
        if (ctrlPressed)
            pos = m_size;
        else
            pos = m_cursorPosition/m_bytesPerLine * m_bytesPerLine + 15;
        setCursorPosition(pos, moveMode);
    } break;
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

void BinEditorWidget::zoomIn(int range)
{
    QFont f = font();
    const int newSize = f.pointSize() + range;
    if (newSize <= 0)
        return;
    f.setPointSize(newSize);
    setFont(f);
}

void BinEditorWidget::zoomOut(int range)
{
    zoomIn(-range);
}

void BinEditorWidget::copy(bool raw)
{
    int selStart = selectionStart();
    int selEnd = selectionEnd();
    const int selectionLength = selEnd - selStart + 1;
    if (selectionLength >> 22) {
        QMessageBox::warning(this, tr("Copying Failed"),
                             tr("You cannot copy more than 4 MB of binary data."));
        return;
    }
    QByteArray data = dataMid(selStart, selectionLength);
    if (raw) {
        data.replace(0, ' ');
        QApplication::clipboard()->setText(QString::fromLatin1(data));
        return;
    }
    QString hexString;
    const char * const hex = "0123456789abcdef";
    hexString.reserve(3 * data.size());
    for (int i = 0; i < data.size(); ++i) {
        const uchar val = static_cast<uchar>(data[i]);
        hexString.append(QLatin1Char(hex[val >> 4])).append(QLatin1Char(hex[val & 0xf])).append(QLatin1Char(' '));
    }
    hexString.chop(1);
    QApplication::clipboard()->setText(hexString);
}

void BinEditorWidget::highlightSearchResults(const QByteArray &pattern, QTextDocument::FindFlags findFlags)
{
    if (m_searchPattern == pattern)
        return;
    m_searchPattern = pattern;
    m_caseSensitiveSearch = (findFlags & QTextDocument::FindCaseSensitively);
    if (!m_caseSensitiveSearch)
        m_searchPattern = m_searchPattern.toLower();
    m_searchPatternHex = calculateHexPattern(pattern);
    viewport()->update();
}

void BinEditorWidget::changeData(int position, uchar character, bool highNibble)
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
    if (emitModificationChanged)
        emit modificationChanged(m_undoStack.size() != m_unmodifiedState);

    if (m_undoStack.size() == 1)
        emit undoAvailable(true);
}


void BinEditorWidget::undo()
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

void BinEditorWidget::redo()
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

void BinEditorWidget::contextMenuEvent(QContextMenuEvent *event)
{
    const int selStart = selectionStart();
    const int byteCount = selectionEnd() - selStart + 1;

    QPointer<QMenu> contextMenu(new QMenu(this));

    QAction *copyAsciiAction = new QAction(tr("Copy Selection as ASCII Characters"), contextMenu);
    QAction *copyHexAction = new QAction(tr("Copy Selection as Hex Values"), contextMenu);
    QAction *jumpToBeAddressHereAction = new QAction(contextMenu);
    QAction *jumpToBeAddressNewWindowAction = new QAction(contextMenu);
    QAction *jumpToLeAddressHereAction = new QAction(contextMenu);
    QAction *jumpToLeAddressNewWindowAction = new QAction(contextMenu);
    QAction *addWatchpointAction = new QAction(tr("Set Data Breakpoint on Selection"), contextMenu);
    contextMenu->addAction(copyAsciiAction);
    contextMenu->addAction(copyHexAction);
    contextMenu->addAction(addWatchpointAction);

    addWatchpointAction->setEnabled(byteCount > 0 && byteCount <= 32);

    quint64 beAddress = 0;
    quint64 leAddress = 0;
    if (byteCount <= 8) {
        asIntegers(selStart, byteCount, beAddress, leAddress);
        setupJumpToMenuAction(contextMenu, jumpToBeAddressHereAction,
                              jumpToBeAddressNewWindowAction, beAddress);

        // If the menu entries would be identical, show only one of them.
        if (beAddress != leAddress) {
            setupJumpToMenuAction(contextMenu, jumpToLeAddressHereAction,
                                  jumpToLeAddressNewWindowAction, leAddress);
        }
    } else {
        jumpToBeAddressHereAction->setText(tr("Jump to Address in This Window"));
        jumpToBeAddressNewWindowAction->setText(tr("Jump to Address in New Window"));
        jumpToBeAddressHereAction->setEnabled(false);
        jumpToBeAddressNewWindowAction->setEnabled(false);
        contextMenu->addAction(jumpToBeAddressHereAction);
        contextMenu->addAction(jumpToBeAddressNewWindowAction);
    }

    QAction *action = contextMenu->exec(event->globalPos());
    if (!contextMenu)
        return;

    if (action == copyAsciiAction)
        copy(true);
    else if (action == copyHexAction)
        copy(false);
    else if (action == jumpToBeAddressHereAction)
        jumpToAddress(beAddress);
    else if (action == jumpToLeAddressHereAction)
        jumpToAddress(leAddress);
    else if (action == jumpToBeAddressNewWindowAction)
        emit newWindowRequested(beAddress);
    else if (action == jumpToLeAddressNewWindowAction)
        emit newWindowRequested(leAddress);
    else if (action == addWatchpointAction)
        emit addWatchpointRequested(m_baseAddr + selStart, byteCount);
    delete contextMenu;
}

void BinEditorWidget::setupJumpToMenuAction(QMenu *menu, QAction *actionHere,
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

void BinEditorWidget::jumpToAddress(quint64 address)
{
    if (address >= m_baseAddr && address < m_baseAddr + m_size)
        setCursorPosition(address - m_baseAddr);
    else
        emit newRangeRequested(address);
}

void BinEditorWidget::setNewWindowRequestAllowed(bool c)
{
    m_canRequestNewWindow = c;
}

void BinEditorWidget::updateContents()
{
    m_oldData = m_data;
    m_data.clear();
    setSizes(baseAddress() + cursorPosition(), m_size, m_blockSize);
    viewport()->update();
}

QPoint BinEditorWidget::offsetToPos(int offset) const
{
    const int x = m_labelWidth + (offset % m_bytesPerLine) * m_columnWidth;
    const int y = (offset / m_bytesPerLine  - verticalScrollBar()->value()) * m_lineHeight;
    return QPoint(x, y);
}

void BinEditorWidget::asFloat(int offset, float &value, bool old) const
{
    value = 0;
    const QByteArray data = dataMid(offset, sizeof(float), old);
    QTC_ASSERT(data.size() ==  sizeof(float), return);
    const float *f = reinterpret_cast<const float *>(data.constData());
    value = *f;
}

void BinEditorWidget::asDouble(int offset, double &value, bool old) const
{
    value = 0;
    const QByteArray data = dataMid(offset, sizeof(double), old);
    QTC_ASSERT(data.size() ==  sizeof(double), return);
    const double *f = reinterpret_cast<const double *>(data.constData());
    value = *f;
}

void BinEditorWidget::asIntegers(int offset, int count, quint64 &bigEndianValue,
    quint64 &littleEndianValue, bool old) const
{
    bigEndianValue = littleEndianValue = 0;
    const QByteArray &data = dataMid(offset, count, old);
    for (int pos = 0; pos < data.size(); ++pos) {
        const quint64 val = static_cast<quint64>(data.at(pos)) & 0xff;
        littleEndianValue += val << (pos * 8);
        bigEndianValue += val << ((count - pos - 1) * 8);
    }
}

void BinEditorWidget::setMarkup(const QList<Markup> &markup)
{
    m_markup = markup;
    viewport()->update();
}

} // namespace BinEditor
