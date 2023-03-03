// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "scrollback.h"

#include <utils/qtcprocess.h>
#include <utils/terminalhooks.h>

#include <QAbstractScrollArea>
#include <QAction>
#include <QTextLayout>
#include <QTimer>

#include <vterm.h>

#include <chrono>
#include <memory>

namespace Terminal {

class TerminalWidget : public QAbstractScrollArea
{
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
        QPoint start;
        QPoint end;
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
    void setupVTerm();
    void setupFont();
    void setupPty();
    void setupColors();
    void setupActions();

    void writeToPty(const QByteArray &data);

    void createTextLayout();

    // Callbacks from vterm
    void invalidate(VTermRect rect);
    int sb_pushline(int cols, const VTermScreenCell *cells);
    int sb_popline(int cols, VTermScreenCell *cells);
    int sb_clear();
    int setTerminalProperties(VTermProp prop, VTermValue *val);
    int movecursor(VTermPos pos, VTermPos oldpos, int visible);

    const VTermScreenCell *fetchCell(int x, int y) const;

    qreal topMargin() const;

    QPoint viewportToGlobal(QPoint p) const;
    QPoint globalToViewport(QPoint p) const;
    QPoint globalToGrid(QPoint p) const;

    int textLineFromPixel(int y) const;
    std::optional<int> textPosFromPoint(const QTextLayout &textLayout, QPoint p) const;

    std::optional<QTextLayout::FormatRange> selectionToFormatRange(
        TerminalWidget::Selection selection, const QTextLayout &layout, int rowOffset) const;

    void applySizeChange();

    void updateScrollBars();

    void flushVTerm(bool force);

    void setSelection(const std::optional<Selection> &selection);

private:
    std::unique_ptr<Utils::QtcProcess> m_process;

    QString m_shellName;

    std::unique_ptr<VTerm, void (*)(VTerm *)> m_vterm;
    VTermScreen *m_vtermScreen;
    QSize m_vtermSize;

    QFont m_font;
    QSizeF m_cellSize;
    qreal m_cellBaseline;
    qreal m_lineSpacing;

    bool m_altscreen{false};
    bool m_ignoreScroll{false};

    QString m_preEditString;

    std::optional<Selection> m_selection;
    QPoint m_selectionStartPos;

    std::unique_ptr<Internal::Scrollback> m_scrollback;

    QTextLayout m_textLayout;

    struct
    {
        int row{0};
        int col{0};
        bool visible{false};
    } m_cursor;

    VTermScreenCallbacks m_vtermScreenCallbacks;

    QAction m_copyAction;
    QAction m_pasteAction;

    QAction m_clearSelectionAction;

    QAction m_zoomInAction;
    QAction m_zoomOutAction;

    QTimer m_flushDelayTimer;

    int m_layoutVersion{0};

    std::array<QColor, 18> m_currentColors;

    Utils::Terminal::OpenTerminalParameters m_openParameters;

    std::chrono::system_clock::time_point m_lastFlush;
    std::chrono::system_clock::time_point m_lastDoubleClick;
    bool m_selectLineMode{false};

    std::u32string m_currentLiveText;
};

} // namespace Terminal
