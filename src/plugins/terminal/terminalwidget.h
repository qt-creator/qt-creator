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

    void copyToClipboard();
    void pasteFromClipboard();

    void clearSelection();

    void zoomIn();
    void zoomOut();

    void clearContents();

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

    struct LinkSelection : public Selection
    {
        Utils::Link link;

        bool operator!=(const LinkSelection &other) const
        {
            return link != other.link || Selection::operator!=(other);
        }
    };

    void setShellName(const QString &shellName);
    QString shellName() const;

    Utils::FilePath cwd() const;
    Utils::CommandLine currentCommand() const;
    std::optional<Utils::Id> identifier() const;
    QProcess::ProcessState processState() const;

    void restart(const Utils::Terminal::OpenTerminalParameters &openParameters);

signals:
    void started(qint64 pid);
    void cwdChanged(const Utils::FilePath &cwd);
    void commandChanged(const Utils::CommandLine &cmd);

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
    void paintSelectionOrBackground(QPainter &painter,
                                    const Internal::TerminalCell &cell,
                                    const QRectF &cellRect,
                                    const QPoint gridPos) const;
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

    bool setSelection(const std::optional<Selection> &selection);
    QString textFromSelection() const;

    void configBlinkTimer();

    QColor toQColor(std::variant<int, QColor> color) const;

    void updateCopyState();

private:
    std::unique_ptr<Utils::QtcProcess> m_process;
    std::unique_ptr<Internal::TerminalSurface> m_surface;
    std::unique_ptr<ShellIntegration> m_shellIntegration;

    QString m_shellName;
    Utils::Id m_identifier;

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

    QTimer m_flushDelayTimer;

    QTimer m_scrollTimer;
    int m_scrollDirection{0};

    std::array<QColor, 18> m_currentColors;

    Utils::Terminal::OpenTerminalParameters m_openParameters;

    std::chrono::system_clock::time_point m_lastFlush;
    std::chrono::system_clock::time_point m_lastDoubleClick;
    bool m_selectLineMode{false};

    Internal::Cursor m_cursor;
    QTimer m_cursorBlinkTimer;
    bool m_cursorBlinkState{true};

    Utils::FilePath m_cwd;
    Utils::CommandLine m_currentCommand;
};

} // namespace Terminal
