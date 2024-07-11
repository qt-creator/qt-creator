// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bineditorconstants.h"
#include "bineditorservice.h"
#include "bineditortr.h"
#include "markup.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <texteditor/behaviorsettings.h>
#include <texteditor/codecchooser.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/fadingindicator.h>
#include <utils/filepath.h>
#include <utils/fileutils.h>
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>
#include <utils/qtcassert.h>
#include <utils/reloadpromptutils.h>
#include <utils/stringutils.h>

#include <QAbstractScrollArea>
#include <QAction>
#include <QAction>
#include <QApplication>
#include <QBasicTimer>
#include <QByteArrayMatcher>
#include <QDebug>
#include <QFile>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QMenu>
#include <QMessageBox>
#include <QMessageBox>
#include <QPainter>
#include <QPointer>
#include <QRegularExpressionValidator>
#include <QScrollBar>
#include <QSet>
#include <QStack>
#include <QString>
#include <QTemporaryFile>
#include <QTextCodec>
#include <QTextCodec>
#include <QTextDocument>
#include <QTextFormat>
#include <QToolBar>
#include <QToolTip>
#include <QVariant>
#include <QWheelEvent>
#include <optional>

using namespace Utils;
using namespace Core;

namespace BinEditor::Internal {

class BinEditorWidget;

class BinEditorDocument : public IDocument
{
    Q_OBJECT
public:
    BinEditorDocument(BinEditorWidget *parent);

    QByteArray contents() const final;
    bool setContents(const QByteArray &contents) final;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const final
    {
        return type == TypeRemoved ? BehaviorSilent : IDocument::reloadBehavior(state, type);
    }

    OpenResult open(QString *errorString, const FilePath &filePath,
                    const FilePath &realFilePath) final
    {
        QTC_CHECK(filePath == realFilePath); // The bineditor can do no autosaving
        return openImpl(errorString, filePath);
    }

    OpenResult openImpl(QString *errorString, const FilePath &filePath, quint64 offset = 0);

    void provideData(quint64 address);

    void provideNewRange(quint64 offset)
    {
        if (filePath().exists())
            openImpl(nullptr, filePath(), offset);
    }

    bool isModified() const final;

    bool isSaveAsAllowed() const final { return true; }

    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) final;
    bool saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave) final;

private:
    BinEditorWidget *m_widget;
};

class BinEditorWidget final : public QAbstractScrollArea, public EditorService
{
    Q_OBJECT
    Q_PROPERTY(bool modified READ isModified WRITE setModified DESIGNABLE false)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE false)
    Q_PROPERTY(QList<BinEditor::Markup> markup READ markup WRITE setMarkup DESIGNABLE false)
    Q_PROPERTY(bool newWindowRequestAllowed READ newWindowRequestAllowed WRITE setNewWindowRequestAllowed DESIGNABLE false)

public:
    BinEditorWidget(QWidget *parent = nullptr);

    ~BinEditorWidget() final
    {
        if (m_aboutToBeDestroyedHandler)
            m_aboutToBeDestroyedHandler();
    }

    void init();

    EditorService *editorService();

    quint64 baseAddress() const { return m_baseAddr; }

    void setSizes(quint64 startAddr, qint64 range, int blockSize = 4096) final;
    int dataBlockSize() const { return m_blockSize; }
    QByteArray contents() const { return dataMid(0, m_size); }

    void addData(quint64 addr, const QByteArray &data) final;

    bool newWindowRequestAllowed() const { return m_canRequestNewWindow; }

    void updateContents() final;
    bool save(QString *errorString, const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath);

    void zoomF(float delta);

    qint64 cursorPosition() const;
    void setCursorPosition(qint64 pos, MoveMode moveMode = MoveAnchor) final;
    void jumpToAddress(quint64 address);

    void setModified(bool);
    bool isModified() const;

    void setReadOnly(bool) final;
    bool isReadOnly() const;

    qint64 find(const QByteArray &pattern, qint64 from = 0, QTextDocument::FindFlags findFlags = {});

    void selectAll();
    void clear();

    void undo();
    void redo();

    Core::IEditor *editor() final { return m_ieditor; }
    void setEditor(Core::IEditor *ieditor) { m_ieditor = ieditor; }

    QWidget *widget() final { return this; }

    qint64 selectionStart() const { return qMin(m_anchorPosition, m_cursorPosition); }
    qint64 selectionEnd() const { return qMax(m_anchorPosition, m_cursorPosition); }

    bool event(QEvent*) override;

    bool isUndoAvailable() const { return !m_undoStack.isEmpty(); }
    bool isRedoAvailable() const { return !m_redoStack.isEmpty(); }

    QString addressString(quint64 address);

    static const int SearchStride = 1024 * 1024;

    QList<Markup> markup() const { return m_markup; }

    void setFontSettings(const TextEditor::FontSettings &fs);
    void highlightSearchResults(const QByteArray &pattern, QTextDocument::FindFlags findFlags = {});
    void copy(bool raw = false);
    void setMarkup(const QList<Markup> &markup);
    void setNewWindowRequestAllowed(bool c) final;
    void setCodec(QTextCodec *codec);

    void setFinished() override
    {
        setReadOnly(true);
        m_fetchDataHandler = {};
        m_newWindowRequestHandler = {};
        m_newRangeRequestHandler = {};
        m_dataChangedHandler = {};
        m_watchPointRequestHandler = {};
    }

    void clearMarkup() override { m_markup.clear(); }
    void addMarkup(quint64 a, quint64 l, const QColor &c, const QString &t) override { m_markup.append(Markup(a, l, c, t)); }
    void commitMarkup() override { setMarkup(m_markup); }

    void setFetchDataHandler(const std::function<void(quint64)> &cb) override { m_fetchDataHandler = cb; }
    void setNewWindowRequestHandler(const std::function<void(quint64)> &cb) override { m_newWindowRequestHandler = cb; }
    void setNewRangeRequestHandler(const std::function<void(quint64)> &cb) override { m_newRangeRequestHandler = cb; }
    void setDataChangedHandler(const std::function<void(quint64, const QByteArray &)> &cb) override { m_dataChangedHandler = cb; }
    void setWatchPointRequestHandler(const std::function<void(quint64, uint)> &cb) override { m_watchPointRequestHandler = cb; }
    void setAboutToBeDestroyedHandler(const std::function<void()> & cb) override { m_aboutToBeDestroyedHandler = cb; }

    void fetchData(quint64 address) { if (m_fetchDataHandler) m_fetchDataHandler(address); }
    void requestNewWindow(quint64 address) { if (m_newWindowRequestHandler) m_newWindowRequestHandler(address); }
    void requestWatchPoint(quint64 address, int size) { if (m_watchPointRequestHandler) m_watchPointRequestHandler(address, size); }
    void requestNewRange(quint64 address) { if (m_newRangeRequestHandler) m_newRangeRequestHandler(address); }
    void announceChangedData(quint64 address, const QByteArray &ba) { if (m_dataChangedHandler) m_dataChangedHandler(address, ba); }

signals:
    void modificationChanged(bool modified);
    void undoAvailable(bool);
    void redoAvailable(bool);
    void cursorPositionChanged(int position);

private:
    void scrollContentsBy(int dx, int dy) override;
    void paintEvent(QPaintEvent *e) override;
    void resizeEvent(QResizeEvent *) override;
    void changeEvent(QEvent *) override;
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *) override;
    void focusOutEvent(QFocusEvent *) override;
    void timerEvent(QTimerEvent *) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    QChar displayChar(char ch) const;

    using BlockMap = QMap<qint64, QByteArray>;
    BlockMap m_data;
    BlockMap m_oldData;
    int m_blockSize = 4096;
    BlockMap m_modifiedData;
    mutable QSet<qint64> m_requests;
    QByteArray m_emptyBlock;
    QByteArray m_lowerBlock;
    qint64 m_size = 0;

    std::function<void(quint64)> m_fetchDataHandler;
    std::function<void(quint64)> m_newWindowRequestHandler;
    std::function<void(quint64)> m_newRangeRequestHandler;
    std::function<void(quint64, const QByteArray &)> m_dataChangedHandler;
    std::function<void(quint64, uint)> m_watchPointRequestHandler;
    std::function<void()> m_aboutToBeDestroyedHandler;

    QTextCodec *m_codec = nullptr;

    qint64 dataIndexOf(const QByteArray &pattern, qint64 from, bool caseSensitive = true) const;
    qint64 dataLastIndexOf(const QByteArray &pattern, qint64 from, bool caseSensitive = true) const;

    bool requestDataAt(qint64 pos) const;
    bool requestOldDataAt(qint64 pos) const;
    char dataAt(qint64 pos, bool old = false) const;
    char oldDataAt(qint64 pos) const;
    void changeDataAt(qint64 pos, char c);
    QByteArray dataMid(qint64 from, qint64 length, bool old = false) const;
    QByteArray blockData(qint64 block, bool old = false) const;

    QPoint offsetToPos(qint64 offset) const;
    void asIntegers(qint64 offset, qint64 count, quint64 &bigEndianValue, quint64 &littleEndianValue,
                    bool old = false) const;
    void asFloat(qint64 offset, float &value, bool old) const;
    void asDouble(qint64 offset, double &value, bool old) const;
    QString toolTip(const QHelpEvent *helpEvent) const;

    int m_bytesPerLine = 16;
    int m_unmodifiedState = 0;
    int m_readOnly = false;
    int m_margin = 0;
    int m_descent = 0;
    int m_ascent = 0;
    int m_lineHeight = 0;
    int m_charWidth = 0;
    int m_labelWidth = 0;
    int m_textWidth = 0;
    int m_columnWidth = 0;
    qint64 m_numLines = 0;
    qint64 m_numVisibleLines = 0;

    quint64 m_baseAddr = 0;

    qint64 m_cursorPosition = 0;
    qint64 m_anchorPosition = 0;
    bool m_cursorVisible = false;
    bool m_hexCursor = true;
    bool m_lowNibble = false;
    bool m_isMonospacedFont = true;
    bool m_caseSensitiveSearch = false;

    QByteArray m_searchPattern;
    QByteArray m_searchPatternHex;

    QBasicTimer m_cursorBlinkTimer;

    std::optional<qint64> posAt(const QPoint &pos, bool includeEmptyArea = true) const;
    bool inTextArea(const QPoint &pos) const;
    QRect cursorRect() const;
    void updateLines();
    void updateLines(qint64 fromPosition, qint64 toPosition);
    void ensureCursorVisible();
    void setBlinkingCursorEnabled(bool enable);

    void changeData(qint64 position, uchar character, bool highNibble = false);

    qint64 findPattern(const QByteArray &data, const QByteArray &dataHex,
                       qint64 from, qint64 offset, qint64 *match);
    void drawItems(QPainter *painter, int x, int y, const QString &itemString);
    void drawChanges(QPainter *painter, int x, int y, const char *changes);

    void setupJumpToMenuAction(QMenu *menu, QAction *actionHere, QAction *actionNew,
                               quint64 addr);

    struct BinEditorEditCommand {
        int position;
        uchar character;
        bool highNibble;
    };
    QStack<BinEditorEditCommand> m_undoStack, m_redoStack;

    QBasicTimer m_autoScrollTimer;
    Core::IEditor *m_ieditor = nullptr;
    QString m_addressString;
    int m_addressBytes = 4;
    bool m_canRequestNewWindow = false;
    QList<Markup> m_markup;
};

const QChar MidpointChar(u'\u00B7');

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

BinEditorWidget::BinEditorWidget(QWidget *parent)
    : QAbstractScrollArea(parent)
{
    setFocusPolicy(Qt::WheelFocus);
    setFrameStyle(QFrame::Plain);

    // Font settings
    setFontSettings(TextEditor::TextEditorSettings::fontSettings());
    connect(TextEditor::TextEditorSettings::instance(),
            &TextEditor::TextEditorSettings::fontSettingsChanged,
            this, &BinEditorWidget::setFontSettings);

    const QByteArray setting = ICore::settings()->value(Constants::C_ENCODING_SETTING).toByteArray();
    if (!setting.isEmpty())
        setCodec(QTextCodec::codecForName(setting));
}

EditorService *BinEditorWidget::editorService()
{
    return this;
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
    m_charWidth = fm.horizontalAdvance(QChar(QLatin1Char('M')));
    m_margin = m_charWidth;
    m_columnWidth = 2 * m_charWidth + fm.horizontalAdvance(QChar(QLatin1Char(' ')));
    m_numLines = m_size / m_bytesPerLine + 1;
    m_numVisibleLines = viewport()->height() / m_lineHeight;
    m_textWidth = m_bytesPerLine * m_charWidth + m_charWidth;
    int numberWidth = fm.horizontalAdvance(QChar(QLatin1Char('9')));
    m_labelWidth = 2*m_addressBytes * numberWidth + (m_addressBytes - 1)/2 * m_charWidth;

    int expectedCharWidth = m_columnWidth / 3;
    const char *hex = "0123456789abcdef";
    m_isMonospacedFont = true;
    while (*hex) {
        if (fm.horizontalAdvance(QLatin1Char(*hex)) != expectedCharWidth) {
            m_isMonospacedFont = false;
            break;
        }
        ++hex;
    }

    if (m_isMonospacedFont && fm.horizontalAdvance(QLatin1String("M M ")) != m_charWidth * 4) {
        // On Qt/Mac, monospace font widths may have a fractional component
        // This breaks the assumption that width("MMM") == width('M') * 3

        m_isMonospacedFont = false;
        m_columnWidth = fm.horizontalAdvance(QLatin1String("MMM"));
        m_labelWidth = m_addressBytes == 4
            ? fm.horizontalAdvance(QLatin1String("MMMM:MMMM"))
            : fm.horizontalAdvance(QLatin1String("MMMM:MMMM:MMMM:MMMM"));
    }

    horizontalScrollBar()->setRange(0, 2 * m_margin + m_bytesPerLine * m_columnWidth
                                    + m_labelWidth + m_textWidth - viewport()->width());
    horizontalScrollBar()->setPageStep(viewport()->width());
    verticalScrollBar()->setRange(0, m_numLines - m_numVisibleLines);
    verticalScrollBar()->setPageStep(m_numVisibleLines);
    ensureCursorVisible();
}


void BinEditorWidget::addData(quint64 addr, const QByteArray &data)
{
    QTC_ASSERT(data.size() == m_blockSize, return);
    if (addr >= m_baseAddr && addr <= m_baseAddr + m_size - 1) {
        if (m_data.size() * m_blockSize >= 64 * 1024 * 1024)
            m_data.clear();
        const qint64 translatedBlock = (addr - m_baseAddr) / m_blockSize;
        m_data.insert(translatedBlock, data);
        m_requests.remove(translatedBlock);
        viewport()->update();
    }
}

bool BinEditorWidget::requestDataAt(qint64 pos) const
{
    qint64 block = pos / m_blockSize;
    BlockMap::const_iterator it = m_modifiedData.find(block);
    if (it != m_modifiedData.constEnd())
        return true;
    it = m_data.find(block);
    if (it != m_data.end())
        return true;
    if (!Utils::insert(m_requests, block))
        return false;
    const_cast<BinEditorWidget *>(this)->fetchData((m_baseAddr / m_blockSize + block) * m_blockSize);
    return true;
}

bool BinEditorWidget::requestOldDataAt(qint64 pos) const
{
    qint64 block = pos / m_blockSize;
    BlockMap::const_iterator it = m_oldData.find(block);
    return it != m_oldData.end();
}

char BinEditorWidget::dataAt(qint64 pos, bool old) const
{
    const qint64 block = pos / m_blockSize;
    const qint64 offset = pos - block * m_blockSize;
    return blockData(block, old).at(offset);
}

void BinEditorWidget::changeDataAt(qint64 pos, char c)
{
    const qint64 block = pos / m_blockSize;
    BlockMap::iterator it = m_modifiedData.find(block);
    const qint64 offset = pos - block * m_blockSize;
    if (it != m_modifiedData.end()) {
        it.value()[offset] = c;
    } else {
        it = m_data.find(block);
        if (it != m_data.end()) {
            QByteArray data = it.value();
            data[offset] = c;
            m_modifiedData.insert(block, data);
        }
    }

    announceChangedData(m_baseAddr + pos, QByteArray(1, c));
}

QByteArray BinEditorWidget::dataMid(qint64 from, qint64 length, bool old) const
{
    qint64 end = from + length;
    qint64 block = from / m_blockSize;

    QByteArray data;
    data.reserve(length);
    do {
        data += blockData(block++, old);
    } while (block * m_blockSize < end);

    return data.mid(from - ((from / m_blockSize) * m_blockSize), length);
}

QByteArray BinEditorWidget::blockData(qint64 block, bool old) const
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

bool BinEditorWidget::save(QString *errorString, const FilePath &oldFilePath, const FilePath &newFilePath)
{
    if (oldFilePath != newFilePath) {
        FilePath tmpName;
        {
            QTemporaryFile tmp(newFilePath.toString() + QLatin1String("_XXXXXX.new"));
            if (!tmp.open())
                return false;
            tmpName = FilePath::fromString(tmp.fileName());
        }
        if (!oldFilePath.copyFile(tmpName))
            return false;
        if (newFilePath.exists() && !newFilePath.removeFile())
            return false;
        if (!tmpName.renameFile(newFilePath))
            return false;
    }

    FileSaver saver(newFilePath, QIODevice::ReadWrite); // QtBug: WriteOnly truncates.
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

void BinEditorWidget::setSizes(quint64 startAddr, qint64 range, int blockSize)
{
    int newBlockSize = blockSize;
    QTC_ASSERT(blockSize, return);
    QTC_ASSERT((blockSize/m_bytesPerLine) * m_bytesPerLine == blockSize,
               blockSize = (blockSize/m_bytesPerLine + 1) * m_bytesPerLine);
    // Users can edit data in the range
    // [startAddr - range/2, startAddr + range/2].
    quint64 newBaseAddr = quint64(range/2) > startAddr ? 0 : startAddr - range/2;
    newBaseAddr = (newBaseAddr / blockSize) * blockSize;

    const quint64 maxRange = Q_UINT64_C(0xffffffffffffffff) - newBaseAddr + 1;
    qint64 newSize = newBaseAddr != 0 && quint64(range) >= maxRange
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
    m_data.clear();
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
        requestNewRange(baseAddress() + m_size);
    else if (dy >= 0 && scrollPos == scrollBar->minimum())
        requestNewRange(baseAddress());
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
        if (!TextEditor::globalBehaviorSettings().m_scrollWheelZooming) {
            // When the setting is disabled globally,
            // we have to skip calling QAbstractScrollArea::wheelEvent()
            // that changes zoom in it.
            return;
        }

        const float delta = e->angleDelta().y() / 120.f;
        if (delta != 0)
            zoomF(delta);
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

QChar BinEditorWidget::displayChar(char ch) const
{
    const QChar qc = QLatin1Char(ch);
    if (qc.isPrint() && qc.unicode() < 128)
        return qc;
    if (!m_codec || qc.unicode() < 32)
        return MidpointChar;
    const QString uc = m_codec->toUnicode(&ch, 1);
    if (uc.isEmpty() || !uc.at(0).isLetterOrNumber())
        return MidpointChar;
    return uc.at(0);
}

std::optional<qint64> BinEditorWidget::posAt(const QPoint &pos, bool includeEmptyArea) const
{
    const int xoffset = horizontalScrollBar()->value();
    int x = xoffset + pos.x() - m_margin - m_labelWidth;
    if (!includeEmptyArea && x < 0)
        return std::nullopt;
    int column = qMin(15, qMax(0,x) / m_columnWidth);
    const qint64 topLine = verticalScrollBar()->value();
    const qint64 line = topLine + pos.y() / m_lineHeight;

    // "clear text" area
    if (x > m_bytesPerLine * m_columnWidth + m_charWidth/2) {
        x -= m_bytesPerLine * m_columnWidth + m_charWidth;
        for (column = 0; column < 16; ++column) {
            const qint64 dataPos = line * m_bytesPerLine + column;
            if (dataPos < 0 || dataPos >= m_size)
                break;
            const QChar qc = displayChar(dataAt(dataPos));
            x -= fontMetrics().horizontalAdvance(qc);
            if (x <= 0)
                break;
        }
        if (!includeEmptyArea && x > 0) // right of the text area
            return std::nullopt;
    }

    const qint64 bytePos = line * m_bytesPerLine + column;
    if (!includeEmptyArea && bytePos >= m_size)
        return std::nullopt;
    return qMin(m_size - 1, bytePos);
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

void BinEditorWidget::updateLines(qint64 fromPosition, qint64 toPosition)
{
    const qint64 topLine = verticalScrollBar()->value();
    const qint64 firstLine = qMin(fromPosition, toPosition) / m_bytesPerLine;
    const qint64 lastLine = qMax(fromPosition, toPosition) / m_bytesPerLine;
    const int y = (firstLine - topLine) * m_lineHeight;
    const int h = (lastLine - firstLine + 1 ) * m_lineHeight;

    viewport()->update(0, y, viewport()->width(), h);
}

qint64 BinEditorWidget::dataIndexOf(const QByteArray &pattern, qint64 from, bool caseSensitive) const
{
    qint64 trailing = pattern.size();
    if (trailing > m_blockSize)
        return -1;

    QByteArray buffer;
    buffer.resize(m_blockSize + trailing);
    QByteArrayMatcher matcher(pattern);

    qint64 block = from / m_blockSize;
    const qint64 end = qMin<qint64>(from + SearchStride, m_size);
    while (from < end) {
        if (!requestDataAt(block * m_blockSize))
            return -1;
        QByteArray data = blockData(block);
        char *b = buffer.data();
        ::memcpy(b, b + m_blockSize, trailing);
        ::memcpy(b + trailing, data.constData(), m_blockSize);

        if (!caseSensitive)
            buffer = buffer.toLower();

        qint64 pos = matcher.indexIn(buffer, from - (block * m_blockSize) + trailing);
        if (pos >= 0)
            return pos + block * m_blockSize - trailing;
        ++block;
        from = block * m_blockSize - trailing;
    }
    return end == m_size ? -1 : -2;
}

qint64 BinEditorWidget::dataLastIndexOf(const QByteArray &pattern, qint64 from, bool caseSensitive) const
{
    qint64 trailing = pattern.size();
    if (trailing > m_blockSize)
        return -1;

    QByteArray buffer;
    buffer.resize(m_blockSize + trailing);

    if (from == -1)
        from = m_size;
    qint64 block = from / m_blockSize;
    const qint64 lowerBound = qMax<qint64>(0, from - SearchStride);
    while (from > lowerBound) {
        if (!requestDataAt(block * m_blockSize))
            return -1;
        QByteArray data = blockData(block);
        char *b = buffer.data();
        ::memcpy(b + m_blockSize, b, trailing);
        ::memcpy(b, data.constData(), m_blockSize);

        if (!caseSensitive)
            buffer = buffer.toLower();

        qint64 pos = buffer.lastIndexOf(pattern, from - (block * m_blockSize));
        if (pos >= 0)
            return pos + block * m_blockSize;
        --block;
        from = block * m_blockSize + (m_blockSize-1) + trailing;
    }
    return lowerBound == 0 ? -1 : -2;
}


qint64 BinEditorWidget::find(const QByteArray &pattern_arg,
                             qint64 from,
                             QTextDocument::FindFlags findFlags)
{
    if (pattern_arg.isEmpty())
        return 0;

    QByteArray pattern = pattern_arg;

    bool caseSensitiveSearch = (findFlags & QTextDocument::FindCaseSensitively);

    if (!caseSensitiveSearch)
        pattern = pattern.toLower();

    bool backwards = (findFlags & QTextDocument::FindBackward);
    qint64 found = backwards ? dataLastIndexOf(pattern, from, caseSensitiveSearch)
                             : dataIndexOf(pattern, from, caseSensitiveSearch);

    qint64 foundHex = -1;
    QByteArray hexPattern = calculateHexPattern(pattern_arg);
    if (!hexPattern.isEmpty()) {
        foundHex = backwards ? dataLastIndexOf(hexPattern, from)
                             : dataIndexOf(hexPattern, from);
    }

    qint64 pos = foundHex == -1 || (found >= 0 && (foundHex == -2 || found < foundHex))
              ? found : foundHex;

    if (pos >= m_size)
        pos = -1;

    if (pos >= 0) {
        setCursorPosition(pos);
        setCursorPosition(pos + (found == pos ? pattern.size() : hexPattern.size()) - 1, KeepAnchor);
    }
    return pos;
}

qint64 BinEditorWidget::findPattern(const QByteArray &data, const QByteArray &dataHex,
                                    qint64 from, qint64 offset, qint64 *match)
{
    if (m_searchPattern.isEmpty())
        return -1;

    qint64 normal = m_searchPattern.isEmpty()
                        ? -1 : data.indexOf(m_searchPattern, from - offset);
    qint64 hex = m_searchPatternHex.isEmpty()
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

    qint64 matchLength = 0;

    QByteArray patternData, patternDataHex;
    qint64 patternOffset = qMax(0, topLine * m_bytesPerLine - m_searchPattern.size());
    if (!m_searchPattern.isEmpty()) {
        patternData = dataMid(patternOffset, m_numVisibleLines * m_bytesPerLine + (topLine*m_bytesPerLine - patternOffset));
        patternDataHex = patternData;
        if (!m_caseSensitiveSearch)
            patternData = patternData.toLower();
    }

    qint64 foundPatternAt = findPattern(patternData, patternDataHex, patternOffset, patternOffset, &matchLength);

    qint64 selStart, selEnd;
    if (m_cursorPosition >= m_anchorPosition) {
        selStart = m_anchorPosition;
        selEnd = m_cursorPosition;
    } else {
        selStart = m_cursorPosition;
        selEnd = m_anchorPosition;
    }

    QString itemString(m_bytesPerLine*3, QLatin1Char(' '));
    QChar *itemStringData = itemString.data();
    char changedString[160] = {false};
    QTC_ASSERT((size_t)m_bytesPerLine < sizeof(changedString), return);
    const char *hex = "0123456789abcdef";

    painter.setPen(palette().text().color());
    const QFontMetrics &fm = painter.fontMetrics();
    for (qint64 i = 0; i <= m_numVisibleLines; ++i) {
        qint64 line = topLine + i;
        if (line >= m_numLines)
            break;

        const quint64 lineAddress = m_baseAddr + line * m_bytesPerLine;
        qint64 y = i * m_lineHeight + m_ascent;
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
        QString printableDisp;

        if (hasData || hasOldData) {
            for (int c = 0; c < m_bytesPerLine; ++c) {
                qint64 pos = line * m_bytesPerLine + c;
                if (pos >= m_size)
                    break;
                const QChar qc = displayChar(dataAt(pos, isOld));
                printable += qc;
                printableDisp += qc;
                if (qc.direction() == QChar::Direction::DirR)
                    printableDisp += QChar(0x200E); // Add LRM to avoid reversing RTL text
            }
        } else {
            printableDisp = printable = QString(m_bytesPerLine, QLatin1Char(' '));
        }

        QRect selectionRect;
        QRect printableSelectionRect;

        bool isFullySelected = (selStart < selEnd && selStart <= line*m_bytesPerLine && (line+1)*m_bytesPerLine <= selEnd);
        bool somethingChanged = false;

        if (hasData || hasOldData) {
            for (int c = 0; c < m_bytesPerLine; ++c) {
                qint64 pos = line * m_bytesPerLine + c;
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
                for (const Markup &m : std::as_const(m_markup)) {
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
                                           + fm.horizontalAdvance(printable.left(c));
                    painter.fillRect(printable_item_x, y-m_ascent,
                                     fm.horizontalAdvance(printable.at(c)),
                                     m_lineHeight, color);
                }

                if (!isFullySelected && pos >= selStart && pos <= selEnd) {
                    selectionRect |= QRect(item_x - m_charWidth/2, y-m_ascent, m_columnWidth, m_lineHeight);
                    int printable_item_x = -xoffset + m_margin + m_labelWidth + m_bytesPerLine * m_columnWidth + m_charWidth
                                           + fm.horizontalAdvance(printable.left(c));
                    printableSelectionRect |= QRect(printable_item_x, y-m_ascent,
                                                    fm.horizontalAdvance(printable.at(c)),
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
                    cursorRect.adjust(fm.horizontalAdvance(itemString.left(1)), 0, 0, 0);
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
                painter.fillRect(text_x, y-m_ascent, fm.horizontalAdvance(printable), m_lineHeight,
                                 palette().highlight());
                painter.setPen(palette().highlightedText().color());
                painter.drawText(text_x, y, printableDisp);
                painter.restore();
        } else {
            painter.drawText(text_x, y, printableDisp);
            if (!printableSelectionRect.isEmpty()) {
                painter.save();
                painter.fillRect(printableSelectionRect, palette().highlight());
                painter.setPen(palette().highlightedText().color());
                painter.setClipRect(printableSelectionRect);
                painter.drawText(text_x, y, printableDisp);
                painter.restore();
            }
        }

        if (cursor >= 0 && !printable.isEmpty()) {
            QRect cursorRect(text_x + fm.horizontalAdvance(printable.left(cursor)),
                             y-m_ascent,
                             fm.horizontalAdvance(printable.at(cursor)),
                             m_lineHeight);
            if (m_hexCursor || !m_cursorVisible) {
                paintCursorBorder(&painter, cursorRect);
            } else {
                painter.save();
                painter.setClipRect(cursorRect);
                painter.fillRect(cursorRect, Qt::red);
                painter.setPen(Qt::white);
                painter.drawText(text_x, y, printableDisp);
                painter.restore();
            }
        }
    }
}


qint64 BinEditorWidget::cursorPosition() const
{
    return m_cursorPosition;
}

void BinEditorWidget::setCursorPosition(qint64 pos, MoveMode moveMode)
{
    pos = qMin(m_size - 1, qMax(qint64(0), pos));
    qint64 oldCursorPosition = m_cursorPosition;

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
    setCursorPosition(posAt(e->pos()).value(), moveMode);
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
    setCursorPosition(posAt(e->pos()).value(), KeepAnchor);
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
                requestNewRange(baseAddress() + m_size);
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
    qint64 selStart = selectionStart();
    qint64 selEnd = selectionEnd();
    qint64 byteCount = std::min(8LL, selEnd - selStart + 1);

    // check even position against selection line by line
    bool insideSelection = false;
    qint64 startInLine = selStart;
    do {
        const qint64 lineIndex = startInLine / m_bytesPerLine;
        const qint64 endOfLine = (lineIndex + 1) * m_bytesPerLine - 1;
        const qint64 endInLine = std::min(selEnd, endOfLine);
        const QPoint &startPoint = offsetToPos(startInLine);
        const QPoint &endPoint = offsetToPos(endInLine) + QPoint(m_columnWidth, 0);
        QRect selectionLineRect(startPoint, endPoint);
        selectionLineRect.setHeight(m_lineHeight);
        if (selectionLineRect.contains(helpEvent->pos())) {
            insideSelection = true;
            break;
        }
        startInLine = endInLine + 1;
    } while (startInLine <= selEnd);
    if (!insideSelection) {
        // show popup for byte under cursor
        std::optional<qint64> pos = posAt(helpEvent->pos(), /*includeEmptyArea*/false);
        if (!pos)
            return QString();
        selStart = pos.value();
        byteCount = 1;
    }

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
        << Tr::tr("Memory at 0x%1").arg(address, 0, 16) << "</b></p>";

    for (const Markup &m : std::as_const(m_markup)) {
        if (m.covers(address) && !m.toolTip.isEmpty()) {
            str << "<p>" <<  m.toolTip << "</p><br>";
            break;
        }
    }
    const QString msgDecimalUnsigned = Tr::tr("Decimal&nbsp;unsigned&nbsp;value:");
    const QString msgDecimalSigned = Tr::tr("Decimal&nbsp;signed&nbsp;value:");
    const QString msgOldDecimalUnsigned = Tr::tr("Previous&nbsp;decimal&nbsp;unsigned&nbsp;value:");
    const QString msgOldDecimalSigned = Tr::tr("Previous&nbsp;decimal&nbsp;signed&nbsp;value:");

    // Table showing little vs. big endian integers for multi-byte
    if (intSize > 1) {
        str << "<table><tr><th>"
            << Tr::tr("%1-bit&nbsp;Integer&nbsp;Type").arg(8 * intSize) << "</th><th>"
            << Tr::tr("Little Endian") << "</th><th>" << Tr::tr("Big Endian") << "</th></tr>";
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
        str << tableRowStartC << Tr::tr("Binary&nbsp;value:") << numericTableRowSepC;
        str.setIntegerBase(2);
        str.setFieldWidth(8);
        str.setPadChar(QLatin1Char('0'));
        str << littleEndianValue;
        str.setFieldWidth(0);
        str << tableRowEndC << tableRowStartC
            << Tr::tr("Octal&nbsp;value:") << numericTableRowSepC;
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
            str << tableRowStartC << Tr::tr("Previous&nbsp;binary&nbsp;value:")
                << numericTableRowSepC;
            str.setIntegerBase(2);
            str.setFieldWidth(8);
            str << littleEndianValueOld;
            str.setFieldWidth(0);
            str << tableRowEndC << tableRowStartC << Tr::tr("Previous&nbsp;octal&nbsp;value:")
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
        str << tableRowStartC << Tr::tr("<i>double</i>&nbsp;value:") << numericTableRowSepC
            << doubleValue << tableRowEndC;
        if (doubleValue != doubleValueOld)
            str << tableRowStartC << Tr::tr("Previous <i>double</i>&nbsp;value:") << numericTableRowSepC
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
        str << tableRowStartC << Tr::tr("<i>float</i>&nbsp;value:") << numericTableRowSepC
            << floatValue << tableRowEndC;
        if (floatValue != floatValueOld)
            str << tableRowStartC << Tr::tr("Previous <i>float</i>&nbsp;value:") << numericTableRowSepC
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
        qint64 line = qMax(qint64(0), m_cursorPosition / m_bytesPerLine - verticalScrollBar()->value());
        verticalScrollBar()->triggerAction(e->key() == Qt::Key_PageUp ?
                                           QScrollBar::SliderPageStepSub : QScrollBar::SliderPageStepAdd);
        if (!ctrlPressed)
            setCursorPosition((verticalScrollBar()->value() + line) * m_bytesPerLine + m_cursorPosition % m_bytesPerLine, moveMode);
        break;
    }
    case Qt::Key_Home: {
        qint64 pos;
        if (ctrlPressed)
            pos = 0;
        else
            pos = m_cursorPosition / m_bytesPerLine * m_bytesPerLine;
        setCursorPosition(pos, moveMode);
        break;
    }
    case Qt::Key_End: {
        qint64 pos;
        if (ctrlPressed)
            pos = m_size;
        else
            pos = m_cursorPosition / m_bytesPerLine * m_bytesPerLine + 15;
        setCursorPosition(pos, moveMode);
        break;
    }
    default: {
        if (m_readOnly)
            break;
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

static void showZoomIndicator(QWidget *editor, const int newZoom)
{
    Utils::FadingIndicator::showText(editor,
                                     Tr::tr("Zoom: %1%").arg(newZoom),
                                     Utils::FadingIndicator::SmallText);
}

void BinEditorWidget::zoomF(float delta)
{
    float step = 10.f * delta;
    // Ensure we always zoom a minimal step in-case the resolution is more than 16x
    if (step > 0 && step < 1)
        step = 1;
    else if (step < 0 && step > -1)
        step = -1;

    const int newZoom = TextEditor::TextEditorSettings::increaseFontZoom(int(step));
    showZoomIndicator(this, newZoom);
}

void BinEditorWidget::copy(bool raw)
{
    const qint64 selStart = selectionStart();
    const qint64 selEnd = selectionEnd();
    const qint64 selectionLength = selEnd - selStart + 1;
    if (selectionLength >> 22) {
        QMessageBox::warning(this, Tr::tr("Copying Failed"),
                             Tr::tr("You cannot copy more than 4 MB of binary data."));
        return;
    }
    QByteArray data = dataMid(selStart, selectionLength);
    if (raw) {
        data.replace(0, ' ');
        QTextCodec *codec = m_codec ? m_codec : QTextCodec::codecForName("latin1");
        setClipboardAndSelection(codec->toUnicode(data));
        return;
    }
    QString hexString;
    const char * const hex = "0123456789abcdef";
    hexString.reserve(3 * data.size());
    for (qint64 i = 0; i < data.size(); ++i) {
        const uchar val = static_cast<uchar>(data[i]);
        hexString.append(QLatin1Char(hex[val >> 4])).append(QLatin1Char(hex[val & 0xf])).append(QLatin1Char(' '));
    }
    hexString.chop(1);
    setClipboardAndSelection(hexString);
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

void BinEditorWidget::changeData(qint64 position, uchar character, bool highNibble)
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
    if (m_undoStack.isEmpty())
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
    if (m_redoStack.isEmpty())
        emit redoAvailable(false);
}

void BinEditorWidget::contextMenuEvent(QContextMenuEvent *event)
{
    const qint64 selStart = selectionStart();
    const qint64 byteCount = selectionEnd() - selStart + 1;

    QPointer<QMenu> contextMenu(new QMenu(this));

    auto copyAsciiAction = new QAction(Tr::tr("Copy Selection as ASCII Characters"), contextMenu);
    auto copyHexAction = new QAction(Tr::tr("Copy Selection as Hex Values"), contextMenu);
    auto copyBeValue = new QAction(contextMenu);
    auto copyLeValue = new QAction(contextMenu);
    auto jumpToBeAddressHereAction = new QAction(contextMenu);
    auto jumpToBeAddressNewWindowAction = new QAction(contextMenu);
    auto jumpToLeAddressHereAction = new QAction(contextMenu);
    auto jumpToLeAddressNewWindowAction = new QAction(contextMenu);
    auto addWatchpointAction = new QAction(Tr::tr("Set Data Breakpoint on Selection"), contextMenu);
    contextMenu->addAction(copyAsciiAction);
    contextMenu->addAction(copyHexAction);
    contextMenu->addAction(addWatchpointAction);

    addWatchpointAction->setEnabled(byteCount > 0 && byteCount <= 32);

    quint64 beAddress = 0;
    quint64 leAddress = 0;
    if (byteCount <= 8) {
        asIntegers(selStart, byteCount, beAddress, leAddress);
        copyBeValue->setText(Tr::tr("Copy 0x%1").arg(QString::number(beAddress, 16)));
        contextMenu->addAction(copyBeValue);
        // If the menu entries would be identical, show only one of them.
        if (beAddress != leAddress) {
            copyLeValue->setText(Tr::tr("Copy 0x%1").arg(QString::number(leAddress, 16)));
            contextMenu->addAction(copyLeValue);
        }
        setupJumpToMenuAction(contextMenu, jumpToBeAddressHereAction,
                              jumpToBeAddressNewWindowAction, beAddress);

        if (beAddress != leAddress) {
            setupJumpToMenuAction(contextMenu, jumpToLeAddressHereAction,
                                  jumpToLeAddressNewWindowAction, leAddress);
        }
    } else {
        jumpToBeAddressHereAction->setText(Tr::tr("Jump to Address in This Window"));
        jumpToBeAddressNewWindowAction->setText(Tr::tr("Jump to Address in New Window"));
        copyBeValue->setText(Tr::tr("Copy Value"));
        jumpToBeAddressHereAction->setEnabled(false);
        jumpToBeAddressNewWindowAction->setEnabled(false);
        copyBeValue->setEnabled(false);
        contextMenu->addAction(copyBeValue);
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
    else if (action == copyBeValue)
        setClipboardAndSelection("0x" + QString::number(beAddress, 16));
    else if (action == copyLeValue)
        setClipboardAndSelection("0x" + QString::number(leAddress, 16));
    else if (action == jumpToBeAddressHereAction)
        jumpToAddress(beAddress);
    else if (action == jumpToLeAddressHereAction)
        jumpToAddress(leAddress);
    else if (action == jumpToBeAddressNewWindowAction)
        requestNewWindow(beAddress);
    else if (action == jumpToLeAddressNewWindowAction)
        requestNewWindow(leAddress);
    else if (action == addWatchpointAction)
        requestWatchPoint(m_baseAddr + selStart, byteCount);
    delete contextMenu;
}

void BinEditorWidget::setupJumpToMenuAction(QMenu *menu, QAction *actionHere,
                                      QAction *actionNew, quint64 addr)
{
    actionHere->setText(Tr::tr("Jump to Address 0x%1 in This Window")
                        .arg(QString::number(addr, 16)));
    actionNew->setText(Tr::tr("Jump to Address 0x%1 in New Window")
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
        requestNewRange(address);
}

void BinEditorWidget::setNewWindowRequestAllowed(bool c)
{
    m_canRequestNewWindow = c;
}

void BinEditorWidget::setCodec(QTextCodec *codec)
{
    if (codec == m_codec)
        return;
    m_codec = codec;
    ICore::settings()->setValue(Constants::C_ENCODING_SETTING, codec ? codec->name() : QByteArray());
    viewport()->update();
}

void BinEditorWidget::updateContents()
{
    m_oldData = m_data;
    m_data.clear();
    m_modifiedData.clear();
    m_requests.clear();
    for (auto it = m_oldData.constBegin(), et = m_oldData.constEnd(); it != et; ++it)
        fetchData(m_baseAddr + it.key());
}

QPoint BinEditorWidget::offsetToPos(qint64 offset) const
{
    const int x = m_labelWidth + (offset % m_bytesPerLine) * m_columnWidth;
    const int y = (offset / m_bytesPerLine  - verticalScrollBar()->value()) * m_lineHeight;
    return QPoint(x, y);
}

void BinEditorWidget::asFloat(qint64 offset, float &value, bool old) const
{
    value = 0;
    const QByteArray data = dataMid(offset, sizeof(float), old);
    QTC_ASSERT(data.size() ==  sizeof(float), return);
    const float *f = reinterpret_cast<const float *>(data.constData());
    value = *f;
}

void BinEditorWidget::asDouble(qint64 offset, double &value, bool old) const
{
    value = 0;
    const QByteArray data = dataMid(offset, sizeof(double), old);
    QTC_ASSERT(data.size() ==  sizeof(double), return);
    const double *f = reinterpret_cast<const double *>(data.constData());
    value = *f;
}

void BinEditorWidget::asIntegers(qint64 offset, qint64 count, quint64 &bigEndianValue,
                                 quint64 &littleEndianValue, bool old) const
{
    bigEndianValue = littleEndianValue = 0;
    const QByteArray &data = dataMid(offset, count, old);
    for (qint64 pos = 0; pos < data.size(); ++pos) {
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

class BinEditorFind : public IFindSupport
{
public:
    BinEditorFind(BinEditorWidget *widget)
    {
        m_widget = widget;
    }

    bool supportsReplace() const override { return false; }
    QString currentFindString() const override { return QString(); }
    QString completedFindString() const override { return QString(); }

    FindFlags supportedFindFlags() const override
    {
        return FindBackward | FindCaseSensitively;
    }

    void resetIncrementalSearch() override
    {
        m_incrementalStartPos = m_contPos = -1;
        m_incrementalWrappedState = false;
    }

    void highlightAll(const QString &txt, FindFlags findFlags) override
    {
        m_widget->highlightSearchResults(txt.toLatin1(),
                                         Utils::textDocumentFlagsForFindFlags(findFlags));
    }

    void clearHighlights() override
    {
        m_widget->highlightSearchResults(QByteArray());
    }

    qint64 find(const QByteArray &pattern, qint64 pos, FindFlags findFlags, bool *wrapped)
    {
        if (wrapped)
            *wrapped = false;
        if (pattern.isEmpty()) {
            m_widget->setCursorPosition(pos);
            return pos;
        }

        qint64 res = m_widget->find(pattern, pos, Utils::textDocumentFlagsForFindFlags(findFlags));
        if (res < 0) {
            pos = (findFlags & FindBackward) ? -1 : 0;
            res = m_widget->find(pattern, pos, Utils::textDocumentFlagsForFindFlags(findFlags));
            if (res < 0)
                return res;
            if (wrapped)
                *wrapped = true;
        }
        return res;
    }

    Result findIncremental(const QString &txt, FindFlags findFlags) override
    {
        QByteArray pattern = txt.toLatin1();
        if (pattern != m_lastPattern)
            resetIncrementalSearch(); // Because we don't search for nibbles.
        m_lastPattern = pattern;
        if (m_incrementalStartPos < 0)
            m_incrementalStartPos = m_widget->selectionStart();
        if (m_contPos == -1)
            m_contPos = m_incrementalStartPos;
        bool wrapped;
        qint64 found = find(pattern, m_contPos, findFlags, &wrapped);
        if (wrapped != m_incrementalWrappedState && (found >= 0)) {
            m_incrementalWrappedState = wrapped;
            showWrapIndicator(m_widget);
        }
        Result result;
        if (found >= 0) {
            result = Found;
            m_widget->highlightSearchResults(pattern,
                                             Utils::textDocumentFlagsForFindFlags(findFlags));
            m_contPos = -1;
        } else {
            if (found == -2) {
                result = NotYetFound;
                m_contPos +=
                        findFlags & FindBackward
                        ? -BinEditorWidget::SearchStride : BinEditorWidget::SearchStride;
            } else {
                result = NotFound;
                m_contPos = -1;
                m_widget->highlightSearchResults(QByteArray(), {});
            }
        }
        return result;
    }

    Result findStep(const QString &txt, FindFlags findFlags) override
    {
        QByteArray pattern = txt.toLatin1();
        bool wasReset = (m_incrementalStartPos < 0);
        if (m_contPos == -1) {
            m_contPos = m_widget->cursorPosition() + 1;
            if (findFlags & FindBackward)
                m_contPos = m_widget->selectionStart()-1;
        }
        bool wrapped;
        qint64 found = find(pattern, m_contPos, findFlags, &wrapped);
        if (wrapped)
            showWrapIndicator(m_widget);
        Result result;
        if (found >= 0) {
            result = Found;
            m_incrementalStartPos = found;
            m_contPos = -1;
            if (wasReset) {
                m_widget->highlightSearchResults(pattern,
                                                 Utils::textDocumentFlagsForFindFlags(findFlags));
            }
        } else if (found == -2) {
            result = NotYetFound;
            m_contPos += findFlags & FindBackward
                         ? -BinEditorWidget::SearchStride : BinEditorWidget::SearchStride;
        } else {
            result = NotFound;
            m_contPos = -1;
        }

        return result;
    }

private:
    BinEditorWidget *m_widget;
    qint64 m_incrementalStartPos = -1;
    qint64 m_contPos = -1; // Only valid if last result was NotYetFound.
    bool m_incrementalWrappedState = false;
    QByteArray m_lastPattern;
};


// BinEditorDocument

BinEditorDocument::BinEditorDocument(BinEditorWidget *parent)
    : IDocument(parent)
{
    setId(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
    setMimeType(Utils::Constants::OCTET_STREAM_MIMETYPE);
    m_widget = parent;
    EditorService *es = m_widget->editorService();
    es->setFetchDataHandler([this](quint64 address) { provideData(address); });
    es->setNewRangeRequestHandler([this](quint64 offset) { provideNewRange(offset); });
    es->setDataChangedHandler([this](quint64, const QByteArray &) { contentsChanged(); });
}

QByteArray BinEditorDocument::contents() const
{
    return m_widget->contents();
}

bool BinEditorDocument::setContents(const QByteArray &contents)
{
    m_widget->clear();
    if (!contents.isEmpty()) {
        m_widget->setSizes(0, contents.length(), contents.length());
        m_widget->addData(0, contents);
    }
    return true;
}

IDocument::OpenResult BinEditorDocument::openImpl(QString *errorString, const FilePath &filePath, quint64 offset)
{
    const qint64 size = filePath.fileSize();
    if (size < 0) {
        QString msg = Tr::tr("Cannot open %1: %2").arg(filePath.toUserOutput(), Tr::tr("File Error"));
        // FIXME: Was: file.errorString(), but we don't have a file anymore.
        if (errorString)
            *errorString = msg;
        else
            QMessageBox::critical(ICore::dialogParent(), Tr::tr("File Error"), msg);
        return OpenResult::ReadError;
    }

    if (size == 0) {
        QString msg = Tr::tr("The Binary Editor cannot open empty files.");
        if (errorString)
            *errorString = msg;
        else
            QMessageBox::critical(ICore::dialogParent(), Tr::tr("File Error"), msg);
        return OpenResult::CannotHandle;
    }

    if (size / 16 >= qint64(1) << 31) {
        // The limit is 2^31 lines (due to QText* interfaces) * 16 bytes per line.
        QString msg = Tr::tr("The file is too big for the Binary Editor (max. 32GB).");
        if (errorString)
            *errorString = msg;
        else
            QMessageBox::critical(ICore::dialogParent(), Tr::tr("File Error"), msg);
        return OpenResult::CannotHandle;
    }

    if (offset >= quint64(size))
        return OpenResult::CannotHandle;

    setFilePath(filePath);
    m_widget->setSizes(offset, size);
    return OpenResult::Success;
}

void BinEditorDocument::provideData(quint64 address)
{
    const FilePath fn = filePath();
    if (fn.isEmpty())
        return;
    const int blockSize = m_widget->dataBlockSize();
    QByteArray data = fn.fileContents(blockSize, address).value_or(QByteArray());
    const int dataSize = data.size();
    if (dataSize != blockSize)
        data += QByteArray(blockSize - dataSize, 0);
    m_widget->addData(address, data);
//       QMessageBox::critical(ICore::dialogParent(), Tr::tr("File Error"),
//                             Tr::tr("Cannot open %1: %2").arg(
//                                   fn.toUserOutput(), file.errorString()));
}

bool BinEditorDocument::isModified() const
{
    return isTemporary()/*e.g. memory view*/ ? false
                                             : m_widget->isModified();
}

bool BinEditorDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(type)
    if (flag == FlagIgnore)
        return true;
    emit aboutToReload();
    int cPos = m_widget->cursorPosition();
    m_widget->clear();
    const bool success = (openImpl(errorString, filePath()) == OpenResult::Success);
    m_widget->setCursorPosition(cPos);
    emit reloadFinished(success);
    return success;
}

bool BinEditorDocument::saveImpl(QString *errorString, const Utils::FilePath &filePath, bool autoSave)
{
    QTC_ASSERT(!autoSave, return true); // bineditor does not support autosave - it would be a bit expensive
    if (m_widget->save(errorString, this->filePath(), filePath)) {
        setFilePath(filePath);
        return true;
    }
    return false;
}

class BinEditorImpl: public IEditor
{
    Q_OBJECT
public:
    BinEditorImpl(BinEditorWidget *widget, BinEditorDocument *doc)
        : m_document(doc)
    {
        using namespace TextEditor;
        setWidget(widget);
        m_addressEdit = new QLineEdit;
        auto addressValidator = new QRegularExpressionValidator(QRegularExpression("[0-9a-fA-F]{1,16}"), m_addressEdit);
        m_addressEdit->setValidator(addressValidator);
        m_codecChooser = new CodecChooser(CodecChooser::Filter::SingleByte);
        m_codecChooser->prependNone();

        auto l = new QHBoxLayout;
        auto w = new QWidget;
        l->setContentsMargins(0, 0, 5, 0);
        l->addStretch(1);
        l->addWidget(m_codecChooser);
        l->addWidget(m_addressEdit);
        w->setLayout(l);

        m_toolBar = new QToolBar;
        m_toolBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        m_toolBar->addWidget(w);

        widget->setEditor(this);

        connect(widget, &BinEditorWidget::cursorPositionChanged,
                this, &BinEditorImpl::updateCursorPosition);
        connect(m_addressEdit, &QLineEdit::editingFinished,
                this, &BinEditorImpl::jumpToAddress);
        connect(m_codecChooser, &CodecChooser::codecChanged,
                widget, &BinEditorWidget::setCodec);
        connect(widget, &BinEditorWidget::modificationChanged,
                m_document, &IDocument::changed);
        updateCursorPosition(widget->cursorPosition());
        const QVariant setting = ICore::settings()->value(Constants::C_ENCODING_SETTING);
        if (!setting.isNull())
            m_codecChooser->setAssignedCodec(QTextCodec::codecForName(setting.toByteArray()));
    }

    ~BinEditorImpl() override
    {
        delete m_widget;
    }

    IDocument *document() const override { return m_document; }

    QWidget *toolBar() override { return m_toolBar; }

private:
    void updateCursorPosition(qint64 position) {
        m_addressEdit->setText(QString::number(editorWidget()->baseAddress() + position, 16));
    }

    void jumpToAddress() {
        editorWidget()->jumpToAddress(m_addressEdit->text().toULongLong(nullptr, 16));
        updateCursorPosition(editorWidget()->cursorPosition());
    }

    BinEditorWidget *editorWidget() const
    {
        QTC_ASSERT(qobject_cast<BinEditorWidget *>(m_widget.data()), return nullptr);
        return static_cast<BinEditorWidget *>(m_widget.data());
    }

private:
    BinEditorDocument *m_document;
    QToolBar *m_toolBar;
    QLineEdit *m_addressEdit;
    TextEditor::CodecChooser *m_codecChooser;
};

///////////////////////////////// BinEditor Services //////////////////////////////////

class FactoryServiceImpl final : public QObject, public FactoryService
{
    Q_OBJECT
    Q_INTERFACES(BinEditor::FactoryService)

public:
    EditorService *createEditorService(const QString &title0, bool wantsEditor) final
    {
        BinEditorWidget *widget = nullptr;
        if (wantsEditor) {
            QString title = title0;
            IEditor *editor = EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_BINARY_EDITOR_ID, &title);
            if (!editor)
                return nullptr;
            widget = qobject_cast<BinEditorWidget *>(editor->widget());
            widget->setEditor(editor);
        } else {
            widget = new BinEditorWidget;
            widget->setWindowTitle(title0);
        }
        return widget->editorService();
    }
};

static FactoryServiceImpl &binEditorService()
{
    static FactoryServiceImpl theFactoryService;
    return theFactoryService;
}

///////////////////////////////// BinEditorFactory //////////////////////////////////

class BinEditorFactory final : public QObject, public IEditorFactory
{
public:
    BinEditorFactory()
    {
        setId(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
        setDisplayName(::Core::Tr::tr("Binary Editor"));
        addMimeType(Utils::Constants::OCTET_STREAM_MIMETYPE);

        m_undoAction = new QAction(Tr::tr("&Undo"), this);
        m_redoAction = new QAction(Tr::tr("&Redo"), this);
        m_copyAction = new QAction(this);
        m_selectAllAction = new QAction(this);

        Context context;
        context.add(Core::Constants::K_DEFAULT_BINARY_EDITOR_ID);
        context.add(Constants::C_BINEDITOR);

        ActionManager::registerAction(m_undoAction, Core::Constants::UNDO, context);
        ActionManager::registerAction(m_redoAction, Core::Constants::REDO, context);
        ActionManager::registerAction(m_copyAction, Core::Constants::COPY, context);
        ActionManager::registerAction(m_selectAllAction, Core::Constants::SELECTALL, context);

        setEditorCreator([this] {
            auto widget = new BinEditorWidget();
            widget->init();
            auto doc = new BinEditorDocument(widget);
            auto editor = new BinEditorImpl(widget, doc);

            connect(m_undoAction, &QAction::triggered, widget, &BinEditorWidget::undo);
            connect(m_redoAction, &QAction::triggered, widget, &BinEditorWidget::redo);
            connect(m_copyAction, &QAction::triggered, widget, &BinEditorWidget::copy);
            connect(m_selectAllAction, &QAction::triggered, widget, &BinEditorWidget::selectAll);

            auto updateActions = [this, widget] {
                m_selectAllAction->setEnabled(true);
                m_undoAction->setEnabled(widget->isUndoAvailable());
                m_redoAction->setEnabled(widget->isRedoAvailable());
            };

            connect(widget, &BinEditorWidget::undoAvailable, widget, updateActions);
            connect(widget, &BinEditorWidget::redoAvailable, widget, updateActions);

            auto aggregate = new Aggregation::Aggregate;
            auto binEditorFind = new BinEditorFind(widget);
            aggregate->add(binEditorFind);
            aggregate->add(widget);

            return editor;
        });
    }

    QAction *m_undoAction = nullptr;
    QAction *m_redoAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_selectAllAction = nullptr;
};

static BinEditorFactory &binEditorFactory()
{
    static BinEditorFactory theBinEditorFactory;
    return theBinEditorFactory;
}


///////////////////////////////// BinEditorPlugin //////////////////////////////////

class BinEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "BinEditor.json")

    ~BinEditorPlugin() final
    {
        ExtensionSystem::PluginManager::removeObject(&binEditorService());
        ExtensionSystem::PluginManager::removeObject(&binEditorFactory());
    }

    void initialize() final
    {
        ExtensionSystem::PluginManager::addObject(&binEditorService());
        ExtensionSystem::PluginManager::addObject(&binEditorFactory());
    }
};

} // BinEditor::Internal

#include "bineditorplugin.moc"
