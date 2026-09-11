// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "terminal_global.h"

#include "celliterator.h"

#include <QImage>
#include <QKeyEvent>
#include <QRectF>
#include <QSize>
#include <QTextCharFormat>

#include <memory>
#include <optional>

namespace TerminalSolution {

class Scrollback;
class SurfaceIntegration;

struct TerminalSurfacePrivate;

enum ColorIndex { Foreground = 16, Background = 17 };

// A cell that shows part of an image names the image and the tile of it the
// cell covers, packed into the one number a cell carries.
namespace ImageCell {
constexpr int columnBits = 10;
constexpr int rowBits = 10;

constexpr int maxId = (1 << (32 - columnBits - rowBits)) - 1;
constexpr int maxColumn = (1 << columnBits) - 1;
constexpr int maxRow = (1 << rowBits) - 1;

inline quint32 tag(int id, int column, int row)
{
    return (quint32(id) << (columnBits + rowBits)) | (quint32(column) << rowBits) | quint32(row);
}
inline int id(quint32 tag) { return int(tag >> (columnBits + rowBits)); }
inline int column(quint32 tag) { return int((tag >> rowBits) & maxColumn); }
inline int row(quint32 tag) { return int(tag & maxRow); }
} // namespace ImageCell

struct ImageTile
{
    QImage image;
    // The pixels of the image the cell shows, which may reach past its edge
    QRectF source;
};

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
    quint32 image{0};
};

struct Hyperlink
{
    QString url;
    int start{0};
    int end{0};
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

    std::optional<Hyperlink> hyperlinkAt(QPoint gridPos) const;

    // The size of a cell in device pixels, which decides how many cells an
    // image takes up and is what an application is told when it asks how much
    // room it has. Device pixels, so that an application draws an image at the
    // resolution of the screen rather than at a fraction of it.
    void setCellSize(QSizeF cellSize);

    // The image the cell tag refers to, if it is still around.
    std::optional<ImageTile> imageTile(quint32 tag) const;

    QSize liveSize() const;
    QSize fullSize() const;

    QPoint posToGrid(int pos) const;
    int gridToPos(QPoint gridPos) const;

    void dataFromPty(const QByteArray &data);
    void flush();

    void pasteFromClipboard(const QString &pastedText);

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
    bool isBracketedPasteEnabled() const;

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
