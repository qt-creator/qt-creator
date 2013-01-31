/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BINEDITOR_H
#define BINEDITOR_H

#include "markup.h"

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

namespace Core {
class IEditor;
}

namespace TextEditor {
class FontSettings;
}

namespace BINEditor {

class BinEditor : public QAbstractScrollArea
{
    Q_OBJECT
    Q_PROPERTY(bool modified READ isModified WRITE setModified DESIGNABLE false)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE false)
    Q_PROPERTY(QList<BINEditor::Markup> markup READ markup WRITE setMarkup DESIGNABLE false)
    Q_PROPERTY(bool newWindowRequestAllowed READ newWindowRequestAllowed WRITE setNewWindowRequestAllowed DESIGNABLE false)
public:
    BinEditor(QWidget *parent = 0);
    ~BinEditor();

    quint64 baseAddress() const { return m_baseAddr; }

    Q_INVOKABLE void setSizes(quint64 startAddr, int range, int blockSize = 4096);
    int dataBlockSize() const { return m_blockSize; }
    Q_INVOKABLE void addData(quint64 block, const QByteArray &data);

    bool newWindowRequestAllowed() const { return m_canRequestNewWindow; }

    Q_INVOKABLE void updateContents();
    bool save(QString *errorString, const QString &oldFileName, const QString &newFileName);

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);

    enum MoveMode {
        MoveAnchor,
        KeepAnchor
    };

    int cursorPosition() const;
    Q_INVOKABLE void setCursorPosition(int pos, MoveMode moveMode = MoveAnchor);
    void jumpToAddress(quint64 address);

    void setModified(bool);
    bool isModified() const;

    void setReadOnly(bool);
    bool isReadOnly() const;

    int find(const QByteArray &pattern, int from = 0,
             QTextDocument::FindFlags findFlags = 0);

    void selectAll();
    void clear();

    void undo();
    void redo();

    Core::IEditor *editor() const { return m_ieditor; }
    void setEditor(Core::IEditor *ieditor) { m_ieditor = ieditor; }

    bool hasSelection() const { return m_cursorPosition != m_anchorPosition; }
    int selectionStart() const { return qMin(m_anchorPosition, m_cursorPosition); }
    int selectionEnd() const { return qMax(m_anchorPosition, m_cursorPosition); }

    bool event(QEvent*);

    bool isUndoAvailable() const { return m_undoStack.size(); }
    bool isRedoAvailable() const { return m_redoStack.size(); }

    QString addressString(quint64 address);

    bool isMemoryView() const; // Is a debugger memory view without file?

    static const int SearchStride = 1024 * 1024;

    QList<Markup> markup() const { return m_markup; }

public Q_SLOTS:
    void setFontSettings(const TextEditor::FontSettings &fs);
    void highlightSearchResults(const QByteArray &pattern,
        QTextDocument::FindFlags findFlags = 0);
    void copy(bool raw = false);
    void setMarkup(const QList<Markup> &markup);
    void setNewWindowRequestAllowed(bool c);

Q_SIGNALS:
    void modificationChanged(bool modified);
    void undoAvailable(bool);
    void redoAvailable(bool);
    void cursorPositionChanged(int position);

    void dataRequested(Core::IEditor *editor, quint64 block);
    void newWindowRequested(quint64 address);
    void newRangeRequested(Core::IEditor *, quint64 address);
    void addWatchpointRequested(quint64 address, uint size);
    void dataChanged(Core::IEditor *, quint64 address, const QByteArray &data);

protected:
    void scrollContentsBy(int dx, int dy);
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *);
    void changeEvent(QEvent *);
    void wheelEvent(QWheelEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);
    void timerEvent(QTimerEvent *);
    void contextMenuEvent(QContextMenuEvent *event);

private:
    typedef QMap<int, QByteArray> BlockMap;
    BlockMap m_data;
    BlockMap m_oldData;
    int m_blockSize;
    BlockMap m_modifiedData;
    mutable QSet<int> m_requests;
    QByteArray m_emptyBlock;
    QByteArray m_lowerBlock;
    int m_size;

    int dataIndexOf(const QByteArray &pattern, int from, bool caseSensitive = true) const;
    int dataLastIndexOf(const QByteArray &pattern, int from, bool caseSensitive = true) const;

    bool requestDataAt(int pos) const;
    bool requestOldDataAt(int pos) const;
    char dataAt(int pos, bool old = false) const;
    char oldDataAt(int pos) const;
    void changeDataAt(int pos, char c);
    QByteArray dataMid(int from, int length, bool old = false) const;
    QByteArray blockData(int block, bool old = false) const;

    QPoint offsetToPos(int offset) const;
    void asIntegers(int offset, int count, quint64 &bigEndianValue, quint64 &littleEndianValue,
        bool old = false) const;
    void asFloat(int offset, float &value, bool old) const;
    void asDouble(int offset, double &value, bool old) const;
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
    int m_numLines;
    int m_numVisibleLines;

    quint64 m_baseAddr;

    bool m_cursorVisible;
    int m_cursorPosition;
    int m_anchorPosition;
    bool m_hexCursor;
    bool m_lowNibble;
    bool m_isMonospacedFont;

    QByteArray m_searchPattern;
    QByteArray m_searchPatternHex;
    bool m_caseSensitiveSearch;

    QBasicTimer m_cursorBlinkTimer;

    void init();
    int posAt(const QPoint &pos) const;
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

} // namespace BINEditor

#endif // BINEDITOR_H
