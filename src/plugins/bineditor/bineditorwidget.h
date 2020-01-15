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

#include "bineditor_global.h"
#include "markup.h"
#include "bineditorservice.h"

#include <utils/optional.h>

#include <QBasicTimer>
#include <QMap>
#include <QSet>
#include <QStack>
#include <QString>

#include <QAbstractScrollArea>
#include <QTextDocument>
#include <QTextFormat>

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QHelpEvent)

namespace Core { class IEditor; }

namespace TextEditor { class FontSettings; }

namespace BinEditor {
namespace Internal {

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
    bool save(QString *errorString, const QString &oldFileName, const QString &newFileName);

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

    friend class BinEditorWidgetPrivate;
    BinEditorWidgetPrivate *d;

    using BlockMap = QMap<qint64, QByteArray>;
    BlockMap m_data;
    BlockMap m_oldData;
    int m_blockSize;
    BlockMap m_modifiedData;
    mutable QSet<qint64> m_requests;
    QByteArray m_emptyBlock;
    QByteArray m_lowerBlock;
    qint64 m_size;

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

    int m_bytesPerLine;
    int m_unmodifiedState;
    int m_readOnly;
    int m_margin;
    int m_descent;
    int m_ascent;
    int m_lineHeight;
    int m_charWidth;
    int m_labelWidth;
    int m_textWidth;
    int m_columnWidth;
    qint64 m_numLines;
    qint64 m_numVisibleLines;

    quint64 m_baseAddr;

    bool m_cursorVisible;
    qint64 m_cursorPosition;
    qint64 m_anchorPosition;
    bool m_hexCursor;
    bool m_lowNibble;
    bool m_isMonospacedFont;

    QByteArray m_searchPattern;
    QByteArray m_searchPatternHex;
    bool m_caseSensitiveSearch;

    QBasicTimer m_cursorBlinkTimer;

    void init();
    Utils::optional<qint64> posAt(const QPoint &pos, bool includeEmptyArea = true) const;
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
    Core::IEditor *m_ieditor;
    QString m_addressString;
    int m_addressBytes;
    bool m_canRequestNewWindow;
    QList<Markup> m_markup;
};

} // namespace Internal
} // namespace BinEditor
