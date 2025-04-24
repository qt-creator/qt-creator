// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "terminal_global.h"
#include "terminalsurface.h"

#include <QAbstractScrollArea>
#include <QAction>
#include <QTextLayout>
#include <QTimer>

#include <chrono>
#include <memory>
#include <optional>

namespace TerminalSolution {

class SurfaceIntegration;
class TerminalViewPrivate;

struct SearchHit
{
    int start{-1};
    int end{-1};

    bool operator!=(const SearchHit &other) const
    {
        return start != other.start || end != other.end;
    }
    bool operator==(const SearchHit &other) const { return !operator!=(other); }
};

QString TERMINAL_EXPORT defaultFontFamily();
int TERMINAL_EXPORT defaultFontSize();

class TERMINAL_EXPORT TerminalView : public QAbstractScrollArea
{
    friend class CellIterator;
    Q_OBJECT
public:
    enum class WidgetColorIdx {
        Foreground = ColorIndex::Foreground,
        Background = ColorIndex::Background,
        Selection,
        FindMatch,
    };

    TerminalView(QWidget *parent = nullptr);
    ~TerminalView() override;

    void setAllowBlinkingCursor(bool allow);
    bool allowBlinkingCursor() const;

    void setFont(const QFont &font);

    void enableMouseTracking(bool enable);

    void copyToClipboard();
    void pasteFromClipboard();
    void copyLinkToClipboard();

    struct Selection
    {
        int start;
        int end;
        bool final{false};

        bool operator!=(const Selection &other) const
        {
            return start != other.start || end != other.end || final != other.final;
        }

        bool operator==(const Selection &other) const { return !operator!=(other); }
    };

    std::optional<Selection> selection() const;
    void clearSelection();
    void selectAll();

    void zoomIn();
    void zoomOut();

    void moveCursorWordLeft();
    void moveCursorWordRight();

    void clearContents();

    void setSurfaceIntegration(SurfaceIntegration *surfaceIntegration);
    void setColors(const std::array<QColor, 20> &colors);

    void setPasswordMode(bool passwordMode);

    struct Link
    {
        QString text;
        int targetLine = 0;
        int targetColumn = 0;
    };

    struct LinkSelection : public Selection
    {
        Link link;

        bool operator!=(const LinkSelection &other) const
        {
            return link.text != other.link.text || link.targetLine != other.link.targetLine
                   || link.targetColumn != other.link.targetColumn || Selection::operator!=(other);
        }
    };

    virtual qint64 writeToPty(const QByteArray &data)
    {
        Q_UNUSED(data)
        return 0;
    }
    void writeToTerminal(const QByteArray &data, bool forceFlush);

    void restart();

    virtual const QList<SearchHit> &searchHits() const
    {
        static QList<SearchHit> noHits;
        return noHits;
    }

    virtual bool resizePty(QSize newSize)
    {
        Q_UNUSED(newSize)
        return true;
    }

    virtual void setClipboard(const QString &text) { Q_UNUSED(text) }
    virtual std::optional<Link> toLink(const QString &text)
    {
        Q_UNUSED(text)
        return std::nullopt;
    }

    virtual void selectionChanged(const std::optional<Selection> &newSelection)
    {
        Q_UNUSED(newSelection)
    }

    virtual void linkActivated(const Link &link) { Q_UNUSED(link) }

    virtual void contextMenuRequested(const QPoint &pos) { Q_UNUSED(pos) }

    virtual void surfaceChanged(){};

    TerminalSurface *surface() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void inputMethodEvent(QInputMethodEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    bool event(QEvent *event) override;

protected:
    void setupSurface();

    int paintCell(QPainter &p,
                  const QRectF &cellRect,
                  QPoint gridPos,
                  const TerminalCell &cell,
                  QFont &f,
                  QList<SearchHit>::const_iterator &searchIt) const;
    void paintCells(QPainter &painter, QPaintEvent *event) const;
    void paintCursor(QPainter &painter) const;
    void paintPreedit(QPainter &painter) const;
    bool paintFindMatches(QPainter &painter,
                          QList<SearchHit>::const_iterator &searchIt,
                          const QRectF &cellRect,
                          const QPoint gridPos) const;

    bool paintSelection(QPainter &painter, const QRectF &cellRect, const QPoint gridPos) const;
    void paintDebugSelection(QPainter &painter, const Selection &selection) const;

    qreal topMargin() const;

    QPoint viewportToGlobal(QPoint p) const;
    QPoint globalToViewport(QPoint p) const;
    QPoint globalToGrid(QPointF p) const;
    QPointF gridToGlobal(QPoint p, bool bottom = false, bool right = false) const;
    QRect gridToViewport(QRect rect) const;
    QPoint toGridPos(QMouseEvent *event) const;

    void updateViewport();
    void updateViewportRect(const QRect &rect);

    int textLineFromPixel(int y) const;
    std::optional<int> textPosFromPoint(const QTextLayout &textLayout, QPoint p) const;

    std::optional<QTextLayout::FormatRange> selectionToFormatRange(Selection selection,
                                                                   const QTextLayout &layout,
                                                                   int rowOffset) const;

    bool checkLinkAt(const QPoint &pos);

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

    bool setSelection(const std::optional<Selection> &selection, bool scroll = true);
    QString textFromSelection() const;

    void configBlinkTimer();

    QColor toQColor(std::variant<int, QColor> color) const;

private:
    void scheduleViewportUpdate();

signals:
    void cleared();

private:
    std::unique_ptr<TerminalViewPrivate> d;
};

} // namespace TerminalSolution
