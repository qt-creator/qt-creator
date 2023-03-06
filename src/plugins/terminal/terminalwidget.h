// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "terminalsurface.h"

#include <utils/link.h>
#include <utils/qtcprocess.h>
#include <utils/terminalhooks.h>

#include <QAbstractScrollArea>
#include <QAction>
#include <QTextLayout>
#include <QTimer>

#include <chrono>
#include <memory>

namespace Terminal {

class TerminalWidget : public QAbstractScrollArea
{
    friend class CellIterator;
    Q_OBJECT
public:
    TerminalWidget(QWidget *parent = nullptr,
                   const Utils::Terminal::OpenTerminalParameters &openParameters = {});

    void setFont(const QFont &font);

    QAction &copyAction();
    QAction &pasteAction();

    QAction &clearSelectionAction();

    QAction &zoomInAction();
    QAction &zoomOutAction();

    void copyToClipboard() const;
    void pasteFromClipboard();

    void clearSelection();

    void zoomIn();
    void zoomOut();

    void clearContents();

    struct Selection
    {
        int start;
        int end;

        bool operator!=(const Selection &other) const
        {
            return start != other.start || end != other.end;
        }
    };

    struct LinkSelection : public Selection
    {
        Utils::Link link;

        bool operator!=(const LinkSelection &other) const
        {
            return link != other.link || Selection::operator!=(other);
        }
    };

    QString shellName() const;

signals:
    void started(qint64 pid);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    void scrollContentsBy(int dx, int dy) override;

    void showEvent(QShowEvent *event) override;

    bool event(QEvent *event) override;

protected:
    void onReadyRead(bool forceFlush);
    void setupSurface();
    void setupFont();
    void setupPty();
    void setupColors();
    void setupActions();

    void writeToPty(const QByteArray &data);

    int paintCell(QPainter &p,
                  const QRectF &cellRect,
                  QPoint gridPos,
                  const Internal::TerminalCell &cell,
                  QFont &f) const;
    void paintCells(QPainter &painter, QPaintEvent *event) const;
    void paintCursor(QPainter &painter) const;
    void paintPreedit(QPainter &painter) const;
    void paintSelection(QPainter &painter) const;
    void paintDebugSelection(QPainter &painter, const Selection &selection) const;

    qreal topMargin() const;

    QPoint viewportToGlobal(QPoint p) const;
    QPoint globalToViewport(QPoint p) const;
    QPoint globalToGrid(QPointF p) const;
    QPointF gridToGlobal(QPoint p, bool bottom = false, bool right = false) const;
    QRect gridToViewport(QRect rect) const;

    void updateViewport();
    void updateViewport(const QRect &rect);

    int textLineFromPixel(int y) const;
    std::optional<int> textPosFromPoint(const QTextLayout &textLayout, QPoint p) const;

    std::optional<QTextLayout::FormatRange> selectionToFormatRange(
        TerminalWidget::Selection selection, const QTextLayout &layout, int rowOffset) const;

    void checkLinkAt(const QPoint &pos);

    struct TextAndOffsets
    {
        int start;
        int end;
        std::u32string text;
    };

    TextAndOffsets textAt(const QPoint &pos) const;

    void applySizeChange();

    void updateScrollBars();

    void flushVTerm(bool force);

    void setSelection(const std::optional<Selection> &selection);

    void configBlinkTimer();

private:
    std::unique_ptr<Utils::QtcProcess> m_process;
    std::unique_ptr<Internal::TerminalSurface> m_surface;

    QString m_shellName;

    QFont m_font;
    QSizeF m_cellSize;

    bool m_ignoreScroll{false};

    QString m_preEditString;

    std::optional<Selection> m_selection;
    std::optional<LinkSelection> m_linkSelection;

    struct
    {
        QPoint start;
        QPoint end;
    } m_activeMouseSelect;

    QAction m_copyAction;
    QAction m_pasteAction;

    QAction m_clearSelectionAction;

    QAction m_zoomInAction;
    QAction m_zoomOutAction;

    QTimer m_flushDelayTimer;

    std::array<QColor, 18> m_currentColors;

    Utils::Terminal::OpenTerminalParameters m_openParameters;

    std::chrono::system_clock::time_point m_lastFlush;
    std::chrono::system_clock::time_point m_lastDoubleClick;
    bool m_selectLineMode{false};

    Internal::Cursor m_cursor;
    QTimer m_cursorBlinkTimer;
    bool m_cursorBlinkState{true};
};

} // namespace Terminal
