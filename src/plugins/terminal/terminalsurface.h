// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "celliterator.h"
#include "shellintegration.h"

#include <QKeyEvent>
#include <QSize>
#include <QTextCharFormat>

#include <memory>

namespace Terminal::Internal {

class Scrollback;

struct TerminalSurfacePrivate;

struct TerminalCell
{
    int width;
    QString text;
    bool bold{false};
    bool italic{false};
    QColor foreground;
    std::optional<QColor> background;
    QTextCharFormat::UnderlineStyle underlineStyle{QTextCharFormat::NoUnderline};
    bool strikeOut{false};
};

struct Cursor
{
    enum class Shape {
        Block = 1,
        Underline,
        LeftBar,
    };
    QPoint position;
    bool visible;
    Shape shape;
    bool blink{false};
};

class TerminalSurface : public QObject
{
    Q_OBJECT;

public:
    TerminalSurface(QSize initialGridSize, ShellIntegration *shellIntegration);
    ~TerminalSurface();

public:
    CellIterator begin() const;
    CellIterator end() const;
    std::reverse_iterator<CellIterator> rbegin() const;
    std::reverse_iterator<CellIterator> rend() const;

    CellIterator iteratorAt(QPoint pos) const;
    CellIterator iteratorAt(int pos) const;

    std::reverse_iterator<CellIterator> rIteratorAt(QPoint pos) const;
    std::reverse_iterator<CellIterator> rIteratorAt(int pos) const;

public:
    void clearAll();

    void resize(QSize newSize);

    TerminalCell fetchCell(int x, int y) const;
    std::u32string::value_type fetchCharAt(int x, int y) const;
    int cellWidthAt(int x, int y) const;

    QSize liveSize() const;
    QSize fullSize() const;

    QPoint posToGrid(int pos) const;
    int gridToPos(QPoint gridPos) const;

    void dataFromPty(const QByteArray &data);
    void flush();

    void setColors(QColor foreground, QColor background);
    void setAnsiColor(int index, QColor color);

    void pasteFromClipboard(const QString &text);

    void sendKey(Qt::Key key);
    void sendKey(QKeyEvent *event);
    void sendKey(const QString &text);

    int invertedScrollOffset() const;

    Cursor cursor() const;

    QColor defaultBgColor() const;

    ShellIntegration *shellIntegration() const;

signals:
    void writeToPty(const QByteArray &data);
    void invalidated(QRect grid);
    void fullSizeChanged(QSize newSize);
    void cursorChanged(Cursor oldCursor, Cursor newCursor);
    void altscreenChanged(bool altScreen);
    void unscroll();

private:
    std::unique_ptr<TerminalSurfacePrivate> d;
};

} // namespace Terminal::Internal
