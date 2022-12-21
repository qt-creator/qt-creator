// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "bineditor_global.h"
#include "markup.h"
#include "bineditorservice.h"

#include <utils/filepath.h>

#include <QAbstractScrollArea>
#include <QBasicTimer>
#include <QMap>
#include <QSet>
#include <QStack>
#include <QString>
#include <QTextDocument>
#include <QTextFormat>

#include <optional>

QT_BEGIN_NAMESPACE
class QHelpEvent;
class QMenu;
class QTextCodec;
QT_END_NAMESPACE


namespace Core { class IEditor; }

namespace TextEditor { class FontSettings; }

namespace BinEditor::Internal {

class BinEditorWidgetPrivate;

class BinEditorWidget : public QAbstractScrollArea
{
    Q_OBJECT
    Q_PROPERTY(bool modified READ isModified WRITE setModified DESIGNABLE false)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE false)
    Q_PROPERTY(QList<BinEditor::Markup> markup READ markup WRITE setMarkup DESIGNABLE false)
    Q_PROPERTY(bool newWindowRequestAllowed READ newWindowRequestAllowed WRITE setNewWindowRequestAllowed DESIGNABLE false)

public:
    BinEditorWidget(QWidget *parent = nullptr);
    ~BinEditorWidget() override;

    EditorService *editorService() const;

    quint64 baseAddress() const { return m_baseAddr; }

    void setSizes(quint64 startAddr, qint64 range, int blockSize = 4096);
    int dataBlockSize() const { return m_blockSize; }
    QByteArray contents() const { return dataMid(0, m_size); }

    void addData(quint64 addr, const QByteArray &data);

    bool newWindowRequestAllowed() const { return m_canRequestNewWindow; }

    void updateContents();
    bool save(QString *errorString, const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath);

    void zoomF(float delta);

    enum MoveMode {
        MoveAnchor,
        KeepAnchor
    };

    qint64 cursorPosition() const;
    void setCursorPosition(qint64 pos, MoveMode moveMode = MoveAnchor);
    void jumpToAddress(quint64 address);

    void setModified(bool);
    bool isModified() const;

    void setReadOnly(bool);
    bool isReadOnly() const;

    int find(const QByteArray &pattern, qint64 from = 0, QTextDocument::FindFlags findFlags = {});

    void selectAll();
    void clear();

    void undo();
    void redo();

    Core::IEditor *editor() const { return m_ieditor; }
    void setEditor(Core::IEditor *ieditor) { m_ieditor = ieditor; }

    int selectionStart() const { return qMin(m_anchorPosition, m_cursorPosition); }
    int selectionEnd() const { return qMax(m_anchorPosition, m_cursorPosition); }

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
    void setNewWindowRequestAllowed(bool c);
    void setCodec(QTextCodec *codec);

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

    friend class BinEditorWidgetPrivate;
    BinEditorWidgetPrivate *d;

    using BlockMap = QMap<qint64, QByteArray>;
    BlockMap m_data;
    BlockMap m_oldData;
    int m_blockSize = 4096;
    BlockMap m_modifiedData;
    mutable QSet<qint64> m_requests;
    QByteArray m_emptyBlock;
    QByteArray m_lowerBlock;
    qint64 m_size = 0;

    int dataIndexOf(const QByteArray &pattern, qint64 from, bool caseSensitive = true) const;
    int dataLastIndexOf(const QByteArray &pattern, qint64 from, bool caseSensitive = true) const;

    bool requestDataAt(qint64 pos) const;
    bool requestOldDataAt(qint64 pos) const;
    char dataAt(qint64 pos, bool old = false) const;
    char oldDataAt(qint64 pos) const;
    void changeDataAt(qint64 pos, char c);
    QByteArray dataMid(qint64 from, int length, bool old = false) const;
    QByteArray blockData(qint64 block, bool old = false) const;

    QPoint offsetToPos(qint64 offset) const;
    void asIntegers(qint64 offset, int count, quint64 &bigEndianValue, quint64 &littleEndianValue,
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

    void init();
    std::optional<qint64> posAt(const QPoint &pos, bool includeEmptyArea = true) const;
    bool inTextArea(const QPoint &pos) const;
    QRect cursorRect() const;
    void updateLines();
    void updateLines(int fromPosition, int toPosition);
    void ensureCursorVisible();
    void setBlinkingCursorEnabled(bool enable);

    void changeData(int position, uchar character, bool highNibble = false);

    int findPattern(const QByteArray &data, const QByteArray &dataHex,
        int from, int offset, int *match);
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

} // BinEditor::Internal
