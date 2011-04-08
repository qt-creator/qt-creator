/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "memoryviewwidget.h"
#include "memoryagent.h"
#include "registerhandler.h"

#include <coreplugin/coreconstants.h>
#include <texteditor/fontsettings.h>

#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QScrollBar>
#include <QtGui/QToolButton>
#include <QtGui/QToolBar>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtGui/QTextDocument>
#include <QtGui/QIcon>
#include <QtGui/QFont>
#include <QtGui/QFontMetrics>
#include <QtGui/QMenu>

#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCore/QVariant>

#include <cctype>

enum { debug = 0 };

// Formatting: 'aaaa:aaaa 0f ab... ASC..'
enum
{
    bytesPerLine = 16,
    lineWidth = 11 + 4 * bytesPerLine
};

namespace Debugger {
namespace Internal {

/*!
    \class Debugger::Internal::MemoryViewWidget
    \brief Base class for memory view tool windows

    Small tool-window that stays on top and displays a chunk of memory.
    Provides next/previous browsing.

    Constructed by passing an instance to \c DebuggerEngine::addMemoryView()
    which will pass it on to \c Debugger::Internal::MemoryAgent::addMemoryView()
    to set up the signal connections to the engine.

    Provides API for marking text with a special format/color.
    The formatting is stored as a list of struct MemoryViewWidget::Markup and applied
    by converting into extra selections once data arrives in setData().

    Provides a context menu that offers to open a subview from a pointer value
    obtained from the memory shown (converted using the Abi).

    \sa Debugger::Internal::MemoryAgent, Debugger::DebuggerEngine
    \sa ProjectExplorer::Abi
*/

const quint64 MemoryViewWidget::defaultLength = 128;

MemoryViewWidget::MemoryViewWidget(QWidget *parent) :
    QWidget(parent, Qt::Tool|Qt::WindowStaysOnTopHint),
    m_previousButton(new QToolButton),
    m_nextButton(new QToolButton),
    m_textEdit(new QPlainTextEdit),
    m_content(new QLabel),
    m_address(0),
    m_length(0),
    m_requestedAddress(0),
    m_requestedLength(0),
    m_updateOnInferiorStop(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout *layout = new QVBoxLayout(this);

    QToolBar *toolBar = new QToolBar;
    toolBar->setObjectName(QLatin1String("MemoryViewWidgetToolBar"));
    toolBar->setProperty("_q_custom_style_disabled", QVariant(true));
    toolBar->setIconSize(QSize(16, 16));
    m_previousButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_PREV)));
    connect(m_previousButton, SIGNAL(clicked()), this, SLOT(slotPrevious()));
    toolBar->addWidget(m_previousButton);
    m_nextButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_NEXT)));
    connect(m_nextButton, SIGNAL(clicked()), this, SLOT(slotNext()));
    toolBar->addWidget(m_nextButton);

    layout->addWidget(toolBar);
    m_textEdit->setObjectName(QLatin1String("MemoryViewWidgetTextEdit"));
    m_textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_textEdit->setReadOnly(true);
    m_textEdit->setWordWrapMode(QTextOption::NoWrap);
    m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_textEdit, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotContextMenuRequested(QPoint)));
    // Text: Pick a fixed font and set minimum size to accommodate default length with vertical scrolling
    const QFont fixedFont(TextEditor::FontSettings::defaultFixedFontFamily(), TextEditor::FontSettings::defaultFontSize());
    const QFontMetrics metrics(fixedFont);
    const QSize lineSize = metrics.size(Qt::TextSingleLine , QString(lineWidth, QLatin1Char('0')));
    int defaultLineCount = defaultLength / bytesPerLine;
    if (defaultLength % bytesPerLine)
        defaultLineCount++;
    const QSize textSize(lineSize.width() + m_textEdit->verticalScrollBar()->width() + 10,
                         lineSize.height() * defaultLineCount + 10);
    m_textEdit->setFont(fixedFont);
    m_textEdit->setMinimumSize(textSize);
    m_textEdit->installEventFilter(this);
    layout->addWidget(m_textEdit);
}

void MemoryViewWidget::engineStateChanged(Debugger::DebuggerState s)
{
    switch (s) {
    case Debugger::InferiorUnrunnable:
        setBrowsingEnabled(true);
        break;
    case Debugger::InferiorStopOk:
        setBrowsingEnabled(true);
        if (m_updateOnInferiorStop)
            requestMemory();
        break;
    case Debugger::DebuggerFinished:
        close();
        break;
    default:
        setBrowsingEnabled(false);
        break;
    }
}

void MemoryViewWidget::setBrowsingEnabled(bool b)
{
    m_previousButton->setEnabled(b && m_address >= m_length);
    m_nextButton->setEnabled(b);
}

void MemoryViewWidget::clear()
{
    m_data.clear();
    m_textEdit->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    m_textEdit->setPlainText(tr("No data available."));
    setBrowsingEnabled(false);
    updateTitle();
}

void MemoryViewWidget::requestMemory()
{
    requestMemory(m_address, m_length);
}

void MemoryViewWidget::requestMemory(quint64 address, quint64 length)
{
    m_requestedAddress = address;
    m_requestedLength = length;

    // For RegisterMemoryViewWidget, the register values sometimes switch to 0
    // while stepping, handle gracefully.
    if (!address || !length) {
        m_address = address;
        m_length = length;
        clear();
        return;
    }

    // Is this the first request and no data available yet? -> Set initial state.
    if (m_data.isEmpty() && !m_address && !m_length) {
        m_address = address;
        m_length = length;
        updateTitle();
        m_textEdit->setPlainText(tr("Fetching %1 bytes...").arg(length));
        setBrowsingEnabled(false);
    }
    if (debug)
        qDebug() << this << "requestMemory()" <<  m_requestedAddress <<  m_requestedLength
                 << " currently at: " << m_address << m_length;
    emit memoryRequested(m_requestedAddress, m_requestedLength);
}

void MemoryViewWidget::setTitle(const QString &t)
{
    setWindowTitle(t);
}

void MemoryViewWidget::slotNext()
{
    requestMemory(m_address + m_length, m_length);
}

void MemoryViewWidget::slotPrevious()
{
    if (m_address >= m_length)
        requestMemory(m_address - m_length, m_length);
}

// Convert address to line and column in range 0..(n - 1), return false
// if out of range.
bool MemoryViewWidget::addressToLineColumn(quint64 posAddress,
                                           int *lineIn /* = 0 */, int *columnIn /* = 0 */,
                                           quint64 *lineStartIn /* = 0 */) const
{
    if (posAddress < m_address)
        return false;
    const quint64 offset = posAddress - m_address;
    if (offset >= quint64(m_data.size()))
        return false;
    const quint64 line = offset / bytesPerLine;
    const quint64 lineStart = m_address + line * bytesPerLine;
    if (lineStartIn)
        *lineStartIn = lineStart;
    if (lineIn)
        *lineIn = int(line);
    const int column = 3 * int(offset % bytesPerLine) + 10;
    if (columnIn)
        *columnIn = column;
    if (debug)
        qDebug() << this << "at" << m_address << " addressToLineColumn "
                 << posAddress << "->" << line << ',' << column << " lineAt" << lineStart;
    return true;
}

// Return address at position
quint64 MemoryViewWidget::addressAt(const QPoint &textPos) const
{
    QTextCursor cursor = m_textEdit->cursorForPosition(textPos);
    if (cursor.isNull())
        return 0;
    const int line = cursor.blockNumber();
    const int column = cursor.columnNumber() - 1;
    const quint64 lineAddress = m_address + line * bytesPerLine;
    const int byte = (qMax(0, column - 9)) /  3;
    if (byte >= bytesPerLine) // Within ASC part
        return 0;
    return lineAddress + byte;
}

void MemoryViewWidget::slotContextMenuRequested(const QPoint &pos)
{
    QMenu *menu = m_textEdit->createStandardContextMenu();
    menu->addSeparator();
    // Add action offering to open a sub-view with a pointer read from the memory
    // at the location: Dereference the chunk of memory as pointer address.
    QAction *derefPointerAction = 0;
    quint64 pointerValue = 0;
    if (!m_data.isEmpty()) {
        const quint64 pointerSize = m_abi.wordWidth() / 8;
        quint64 contextAddress = addressAt(pos);
        if (const quint64 remainder = contextAddress % pointerSize) // Pad pointer location.
            contextAddress -= remainder;
        // Dereference pointer from location
        if (contextAddress) {
            const quint64 dataOffset = contextAddress - address();
            if (pointerSize && (dataOffset + pointerSize) <= quint64(m_data.size())) {
                const unsigned char *data = reinterpret_cast<const unsigned char *>(m_data.constData() + dataOffset);
                pointerValue = MemoryAgent::readInferiorPointerValue(data, m_abi);
            }
        }
    } // has data
    if (pointerValue) {
        const QString msg = tr("Open Memory View at Pointer Value 0x%1")
                            .arg(pointerValue, 0, 16);
        derefPointerAction = menu->addAction(msg);
    } else {
        derefPointerAction = menu->addAction("Open Memory View at Pointer Value");
        derefPointerAction->setEnabled(false);
    }
    const QPoint globalPos = m_textEdit->mapToGlobal(pos);
    QAction *action = menu->exec(globalPos);
    if (!action)
        return;
    if (action == derefPointerAction) {
        emit openViewRequested(pointerValue, MemoryViewWidget::defaultLength, globalPos);
        return;
    }
}

// Format address as in binary editor '0000:00AB' onto a stream set up for hex output.
static inline void formatAddressToHexStream(QTextStream &hexStream, quint64 address)
{
    hexStream.setFieldWidth(4);
    hexStream << (address >> 32);
    hexStream.setFieldWidth(1);
    hexStream << ':';
    hexStream.setFieldWidth(4);
    hexStream << (address & 0xFFFF);
}

// Return formatted address for window title: Prefix + binary editor format: '0x0000:00AB'
static inline QString formattedAddress(quint64 a)
{
    QString rc = QLatin1String("0x");
    QTextStream str(&rc);
    str.setIntegerBase(16);
    str.setPadChar(QLatin1Char('0'));
    formatAddressToHexStream(str, a);
    return rc;
}

// Format data as in binary editor '0000:00AB 0A A3  ..ccc'
QString MemoryViewWidget::formatData(quint64 startAddress, const QByteArray &data)
{
    QString rc;
    rc.reserve(5 * data.size());
    const quint64 endAddress = startAddress + data.size();
    QTextStream str(&rc);
    str.setIntegerBase(16);
    str.setPadChar(QLatin1Char('0'));
    str.setFieldAlignment(QTextStream::AlignRight);
    for (quint64 address = startAddress; address < endAddress; address += 16) {
        formatAddressToHexStream(str, address);
        // Format hex bytes
        const int dataStart = int(address - startAddress);
        const int dataEnd  = qMin(dataStart + int(bytesPerLine), data.size());
        for (int i = dataStart; i < dataEnd; i++) {
            str.setFieldWidth(1);
            str << ' ';
            str.setFieldWidth(2);
            const char c = data.at(i);
            unsigned char uc = c;
            str << unsigned(uc);
        }
        // Pad for character part
        str.setFieldWidth(1);
        if (const int remainder = int(bytesPerLine) - (dataEnd - dataStart))
            str << QString(3 * remainder, QLatin1Char(' '));
        // Format characters
        str << ' ';
        for (int i = dataStart; i < dataEnd; i++) {
            const char c = data.at(i);
            str << (c >= 0 && std::isprint(c) ? c : '.'); // MSVC has an assert on c>=0.
        }
        str << '\n';
    }
    return rc;
}

void MemoryViewWidget::updateTitle()
{
    const QString title = tr("Memory at %1").arg(formattedAddress(address()));
    setTitle(title);
}

// Convert an markup range into a list of selections for the bytes,
// resulting in a rectangular selection in the bytes area (provided range
// is within data available).
bool MemoryViewWidget::markUpToSelections(const Markup &r,
                                          QList<QTextEdit::ExtraSelection> *extraSelections) const
{
    // Fully covered?
    if (r.address < m_address)
        return false;
    const quint64 rangeEnd = r.address + r.size;
    if (rangeEnd > (m_address + quint64(m_data.size())))
        return false;

    QTextCursor cursor = m_textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);

    // Goto first position
    int line;
    int column;
    quint64 lineStartAddress;

    if (!addressToLineColumn(r.address, &line, &column, &lineStartAddress))
        return false;

    if (line)
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, line);
    if (column)
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column - 1);

    quint64 current = r.address;
    // Mark rectangular area in the bytes section
    while (true) {
        // Mark in current line
        quint64 nextLineAddress = lineStartAddress + bytesPerLine;
        const int numberOfCharsToMark = 3 * int(qMin(nextLineAddress, rangeEnd) - current);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, numberOfCharsToMark);
        QTextEdit::ExtraSelection sel;
        sel.cursor = cursor;
        sel.format = r.format;
        extraSelections->push_back(sel);
        if (nextLineAddress >= rangeEnd)
            break;
        // Goto beginning of next line, past address.
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 9);
        lineStartAddress += bytesPerLine;
        current = lineStartAddress;
    }
    return true;
}

void MemoryViewWidget::clearMarkup()
{
    m_markup.clear();
    m_textEdit->setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

void MemoryViewWidget::addMarkup(quint64 begin, quint64 size,
                                 const QTextCharFormat &fmt, const QString &toolTip)
{
    m_markup.push_back(Markup(begin, size, fmt, toolTip));
}

void MemoryViewWidget::addMarkup(quint64 begin, quint64 size,
                                 const QColor &background, const QString &toolTip)
{
    QTextCharFormat format = textCharFormat();
    format.setBackground(QBrush(background));
    addMarkup(begin, size, format, toolTip);
}

QTextCharFormat MemoryViewWidget::textCharFormat() const
{
    return m_textEdit->currentCharFormat();
}

void MemoryViewWidget::setData(const QByteArray &a)
{
    if (debug)
        qDebug() << this << m_requestedAddress << m_requestedLength << "received" << a.size();

    if (quint64(a.size()) < m_requestedLength) {
        const QString msg = QString::fromLatin1("Warning: %1 received only %2 bytes of %3 at 0x%4")
                           .arg(QString::fromAscii(metaObject()->className()))
                           .arg(a.size()).arg(m_requestedLength).arg(m_requestedAddress, 0, 16);
        qWarning("%s", qPrintable(msg));
    }

    if (m_address != m_requestedAddress || m_length != m_requestedLength) {
        m_address = m_requestedAddress;
        m_length = m_requestedLength;
        updateTitle();
    }

    if (a.isEmpty()) {
        clear();
        return;
    }

    m_data = a;

    QList<QTextEdit::ExtraSelection> extra;
    m_textEdit->setExtraSelections(extra);
    m_textEdit->setPlainText(MemoryViewWidget::formatData(address(), a));
    // Do markup which is in visible range now.
    foreach (const Markup &r, m_markup)
        markUpToSelections(r, &extra);
    if (!extra.isEmpty())
        m_textEdit->setExtraSelections(extra);
    setBrowsingEnabled(true);
}

// Find markup by address.
int MemoryViewWidget::indexOfMarkup(quint64 address) const
{
    const int size = m_markup.size();
    for (int m = 0; m < size; m++)
        if (m_markup.at(m).covers(address))
            return m;
    return -1;
}

bool MemoryViewWidget::eventFilter(QObject *o, QEvent *e)
{
    if (o != m_textEdit || e->type() != QEvent::ToolTip)
        return QWidget::eventFilter(o, e);
    // ToolTip handling: is the cursor over an address range that has a tooltip
    // defined in the markup list?
    const QHelpEvent *he = static_cast<const QHelpEvent *>(e);
    if (const quint64 toolTipAddress = addressAt(he->pos())) {
        const int mIndex = indexOfMarkup(toolTipAddress);
        if (mIndex != -1) {
            m_textEdit->setToolTip(m_markup.at(mIndex).toolTip);
        } else {
            m_textEdit->setToolTip(QString());
        }
    }
    return QWidget::eventFilter(o, e);
}

/*!
    \class Debugger::Internal::LocalsMemoryViewWidget
    \brief Memory view that shows the memory at the location of a local variable.

    Refreshes whenever Debugger::InferiorStopOk is reported.

    \sa Debugger::Internal::WatchWindow
    \sa Debugger::Internal::MemoryAgent, Debugger::DebuggerEngine
*/

LocalsMemoryViewWidget::LocalsMemoryViewWidget(QWidget *parent) :
    MemoryViewWidget(parent), m_variableAddress(0)
{
    setUpdateOnInferiorStop(true);
}

void LocalsMemoryViewWidget::init(quint64 variableAddress, quint64 size, const QString &name)
{
    m_variableAddress = variableAddress;
    m_variableSize = size;
    m_variableName = name;
    // Size may be 0.
    addMarkup(variableAddress, qMax(size, quint64(1)), Qt::lightGray);
    requestMemory(m_variableAddress, qMax(size, quint64(defaultLength)));
    if (debug)
        qDebug() << this << "init" << variableAddress << m_variableName << m_variableSize;
}

void LocalsMemoryViewWidget::updateTitle()
{
    const QString variableAddress = formattedAddress(m_variableAddress);
    if (address() == m_variableAddress) {
        const QString title = tr("Memory at '%1' (%2)")
                              .arg(m_variableName, variableAddress);
        setTitle(title);
    } else if (address() > m_variableAddress) {
        const QString title = tr("Memory at '%1' (%2 + %3)")
                .arg(m_variableName, variableAddress)
                .arg(address() - m_variableAddress);
        setTitle(title);
    } else if (address() < m_variableAddress) {
        const QString title = tr("Memory at '%1' (%2 - %3)")
                .arg(m_variableName, variableAddress)
                .arg(m_variableAddress - address());
        setTitle(title);
    }
}

/*!
    \class Debugger::Internal::RegisterMemoryViewWidget
    \brief Memory view that shows the memory around the contents of a register
           (such as stack pointer, program counter),
           tracking changes of the register value.

    Connects to Debugger::Internal::RegisterHandler to listen for changes
    of the register value.

    \sa Debugger::Internal::RegisterHandler, Debugger::Internal::RegisterWindow
    \sa Debugger::Internal::MemoryAgent, Debugger::DebuggerEngine
*/

RegisterMemoryViewWidget::Markup::Markup(quint64 a, quint64 s,
                                         const QTextCharFormat &fmt, const QString &tt) :
    address(a), size(s), format(fmt), toolTip(tt)
{
}

RegisterMemoryViewWidget::RegisterMemoryViewWidget(QWidget *parent) :
    MemoryViewWidget(parent),
    m_registerIndex(-1),
    m_registerAddress(0),
    m_offset(0)
{
    setUpdateOnInferiorStop(false); // We update on register changed.
}

void RegisterMemoryViewWidget::updateTitle()
{
    const quint64 shownAddress = address() + m_offset;
    const QString registerAddress = formattedAddress(m_registerAddress);
    if (shownAddress == m_registerAddress) {
        const QString title = tr("Memory at Register '%1' (%2)")
                              .arg(m_registerName, registerAddress);
        setTitle(title);
    } else if (shownAddress > m_registerAddress) {
        const QString title = tr("Memory at Register '%1' (%2 + %3)")
                .arg(m_registerName, registerAddress)
                .arg(shownAddress - m_registerAddress);
        setTitle(title);
    } else if (shownAddress < m_registerAddress) {
        const QString title = tr("Memory at Register '%1' (%2 - %3)")
                .arg(m_registerName, registerAddress)
                .arg(m_registerAddress - shownAddress);
        setTitle(title);
    }
}

void RegisterMemoryViewWidget::setRegisterAddress(quint64 a)
{
    if (!a) { // Registers might switch to 0 (for example, 'rsi' while stepping out).
        m_offset = m_registerAddress = a;
        requestMemory(0, 0);
        return;
    }
    if (m_registerAddress == a) { // Same value: just re-fetch
        requestMemory();
        return;
    }
    // Show an area around that register
    m_registerAddress = a;
    const quint64 range = MemoryViewWidget::defaultLength / 2;
    const quint64 end = a + range;
    const quint64 begin = a >= range ? a - range : 0;
    m_offset = m_registerAddress - begin;
    // Mark one byte showing the register
    clearMarkup();
    addMarkup(m_registerAddress, 1, Qt::lightGray, tr("Register %1").arg(m_registerName));
    requestMemory(begin, end - begin);
}

void RegisterMemoryViewWidget::slotRegisterSet(const QModelIndex &index)
{
    if (m_registerIndex != index.row())
        return;
    const QVariant newAddressV = index.data(Qt::EditRole);
    if (newAddressV.type() == QVariant::ULongLong) {
        if (debug)
            qDebug() << this << m_registerIndex << m_registerName << "slotRegisterSet" << newAddressV;
        setRegisterAddress(newAddressV.toULongLong());
    }
}

void RegisterMemoryViewWidget::init(int registerIndex, RegisterHandler *h)
{
    m_registerIndex = registerIndex;
    m_registerName = QString::fromAscii(h->registerAt(registerIndex).name);
    if (debug)
        qDebug() << this << "init" << registerIndex << m_registerName;
    // Known issue: CDB might reset the model by changing the special
    // registers it reports.
    connect(h, SIGNAL(modelReset()), this, SLOT(close()));
    connect(h, SIGNAL(registerSet(QModelIndex)),
            this, SLOT(slotRegisterSet(QModelIndex)));
    setRegisterAddress(h->registerAt(m_registerIndex).editValue().toULongLong());
}

} // namespace Internal
} // namespace Debugger
