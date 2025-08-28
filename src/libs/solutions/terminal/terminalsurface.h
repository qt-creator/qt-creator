// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "terminal_global.h"

#include "celliterator.h"

#include <QKeyEvent>
#include <QSize>
#include <QTextCharFormat>

#include <memory>

namespace TerminalSolution {

class Scrollback;
class SurfaceIntegration;

struct TerminalSurfacePrivate;

enum ColorIndex { Foreground = 16, Background = 17 };

struct TerminalCell
{
    int width;
    QString text;
    bool bold{false};
    bool italic{false};
    std::variant<int, QColor> foregroundColor;
    std::variant<int, QColor> backgroundColor;
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

class TERMINAL_EXPORT TerminalSurface : public QObject
{
    Q_OBJECT;

public:
    TerminalSurface(QSize initialGridSize);
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

    void pasteFromClipboard(const QString &text);

    void sendKey(Qt::Key key);
    void sendKey(QKeyEvent *event);
    void sendKey(const QString &text);

    int invertedScrollOffset() const;

    Cursor cursor() const;

    SurfaceIntegration *surfaceIntegration() const;
    void setSurfaceIntegration(SurfaceIntegration *surfaceIntegration);

    using WriteToPty = std::function<qint64(const QByteArray &)>;
    void setWriteToPty(WriteToPty writeToPty);

    void mouseMove(QPoint pos, Qt::KeyboardModifiers modifiers);
    void mouseButton(Qt::MouseButton button, bool pressed, Qt::KeyboardModifiers modifiers);

    void sendFocus(bool hasFocus);
    bool isInAltScreen();
    void enableLiveReflow(bool enable);

signals:
    void invalidated(QRect grid);
    void fullSizeChanged(QSize newSize);
    void cursorChanged(Cursor oldCursor, Cursor newCursor);
    void altscreenChanged(bool altScreen);
    void unscroll();
    void cleared();

private:
    std::unique_ptr<TerminalSurfacePrivate> d;
};

} // namespace TerminalSolution
