// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalsurface.h"
#include "surfaceintegration.h"

#include "keys.h"
#include "scrollback.h"

#include <vterm.h>

#include <QLoggingCategory>
#include <QTimer>

namespace TerminalSolution {

Q_LOGGING_CATEGORY(log, "qtc.terminal.surface", QtWarningMsg);

QColor toQColor(const VTermColor &c)
{
    return QColor(qRgb(c.rgb.red, c.rgb.green, c.rgb.blue));
};

constexpr int batchFlushSize = 256;

struct TerminalSurfacePrivate
{
    TerminalSurfacePrivate(TerminalSurface *surface, const QSize &initialGridSize)
        : m_vterm(vterm_new(initialGridSize.height(), initialGridSize.width()), vterm_free)
        , m_vtermScreen(vterm_obtain_screen(m_vterm.get()))
        , m_scrollback(std::make_unique<Scrollback>(100'000'000))
        , q(surface)
    {}

    void flush()
    {
        if (m_writeBuffer.isEmpty())
            return;

        QByteArray data = m_writeBuffer.left(batchFlushSize);
        qint64 result = m_writeToPty(data);

        if (result != data.size()) {
            // Not all data was written, remove the unwritten data from the array
            data.resize(qMax(0, result));
        }

        // Remove the written data from the buffer
        if (data.size() > 0)
            m_writeBuffer = m_writeBuffer.mid(data.size());

        if (!m_writeBuffer.isEmpty())
            m_delayWriteTimer.start();
    }

    void init()
    {
        m_delayWriteTimer.setInterval(1);
        m_delayWriteTimer.setSingleShot(true);

        QObject::connect(&m_delayWriteTimer, &QTimer::timeout, &m_delayWriteTimer, [this] {
            flush();
        });

        vterm_set_utf8(m_vterm.get(), true);

        static auto writeToPty = [](const char *s, size_t len, void *user) {
            auto p = static_cast<TerminalSurfacePrivate *>(user);
            QByteArray d(s, len);

            // If its just a couple of chars, or we already have data in the writeBuffer,
            // add the new data to the write buffer and start the delay timer
            if (d.size() < batchFlushSize || !p->m_writeBuffer.isEmpty()) {
                p->m_writeBuffer.append(d);
                p->m_delayWriteTimer.start();
                return;
            }

            // Try to write the data ...
            qint64 result = p->m_writeToPty(d);

            if (result != d.size()) {
                // if writing failed, append the data to the writeBuffer and start the delay timer

                // Check if partial data may have already been written ...
                if (result <= 0)
                    p->m_writeBuffer.append(d);
                else
                    p->m_writeBuffer.append(d.mid(result));

                p->m_delayWriteTimer.start();
            }
        };

        vterm_output_set_callback(m_vterm.get(), writeToPty, this);

        memset(&m_vtermScreenCallbacks, 0, sizeof(m_vtermScreenCallbacks));

        m_vtermScreenCallbacks.damage = [](VTermRect rect, void *user) {
            auto p = static_cast<TerminalSurfacePrivate *>(user);
            p->invalidate(rect);
            return 1;
        };
        m_vtermScreenCallbacks.sb_pushline = [](int cols, const VTermScreenCell *cells, void *user) {
            auto p = static_cast<TerminalSurfacePrivate *>(user);
            return p->sb_pushline(cols, cells);
        };
        m_vtermScreenCallbacks.sb_popline = [](int cols, VTermScreenCell *cells, void *user) {
            auto p = static_cast<TerminalSurfacePrivate *>(user);
            return p->sb_popline(cols, cells);
        };
        m_vtermScreenCallbacks.settermprop = [](VTermProp prop, VTermValue *val, void *user) {
            auto p = static_cast<TerminalSurfacePrivate *>(user);
            return p->setTerminalProperties(prop, val);
        };
        m_vtermScreenCallbacks.movecursor =
            [](VTermPos pos, VTermPos oldpos, int visible, void *user) {
                auto p = static_cast<TerminalSurfacePrivate *>(user);
                return p->movecursor(pos, oldpos, visible);
            };
        m_vtermScreenCallbacks.sb_clear = [](void *user) {
            auto p = static_cast<TerminalSurfacePrivate *>(user);
            emit p->q->cleared();
            return p->sb_clear();
        };
        m_vtermScreenCallbacks.bell = [](void *user) {
            auto p = static_cast<TerminalSurfacePrivate *>(user);
            if (p->m_surfaceIntegration)
                p->m_surfaceIntegration->onBell();
            return 1;
        };

        vterm_screen_set_callbacks(m_vtermScreen, &m_vtermScreenCallbacks, this);
        vterm_screen_set_damage_merge(m_vtermScreen, VTERM_DAMAGE_SCROLL);
        vterm_screen_enable_altscreen(m_vtermScreen, true);

        memset(&m_vtermStateFallbacks, 0, sizeof(m_vtermStateFallbacks));

        m_vtermStateFallbacks.osc = [](int cmd, VTermStringFragment fragment, void *user) {
            auto p = static_cast<TerminalSurfacePrivate *>(user);
            return p->osc(cmd, fragment);
        };

        VTermState *vts = vterm_obtain_state(m_vterm.get());
        vterm_state_set_unrecognised_fallbacks(vts, &m_vtermStateFallbacks, this);

        memset(&m_vtermSelectionCallbacks, 0, sizeof(m_vtermSelectionCallbacks));

        m_vtermSelectionCallbacks.query = [](VTermSelectionMask mask, void *user) {
            if (!(mask & 0xF))
                return 0;

            auto p = static_cast<TerminalSurfacePrivate *>(user);
            if (p->m_surfaceIntegration)
                p->m_surfaceIntegration->onGetClipboard();

            return 0;
        };

        m_vtermSelectionCallbacks.set =
            [](VTermSelectionMask mask, VTermStringFragment frag, void *user) {
                if (!(mask & 0xF))
                    return 0;

                auto p = static_cast<TerminalSurfacePrivate *>(user);
                if (frag.initial)
                    p->m_selectionBuffer.clear();

                p->m_selectionBuffer.append(frag.str, frag.len);
                if (!frag.final)
                    return 1;

                if (p->m_surfaceIntegration)
                    p->m_surfaceIntegration->onSetClipboard(p->m_selectionBuffer);

                return 1;
            };

        vterm_state_set_selection_callbacks(vts, &m_vtermSelectionCallbacks, this, nullptr, 256);

        vterm_state_set_bold_highbright(vts, true);

        VTermColor fg;
        VTermColor bg;
        vterm_color_indexed(&fg, ColorIndex::Foreground);
        vterm_color_indexed(&bg, ColorIndex::Background);
        vterm_state_set_default_colors(vts, &fg, &bg);

        for (int i = 0; i < 16; ++i) {
            VTermColor col;
            vterm_color_indexed(&col, i);
            vterm_state_set_palette_color(vts, i, &col);
        }

        vterm_screen_reset(m_vtermScreen, 1);
    }

    QSize liveSize() const
    {
        int rows;
        int cols;
        vterm_get_size(m_vterm.get(), &rows, &cols);

        return QSize(cols, rows);
    }

    std::variant<int, QColor> toVariantColor(const VTermColor &color)
    {
        if (color.type & VTERM_COLOR_DEFAULT_BG)
            return ColorIndex::Background;
        else if (color.type & VTERM_COLOR_DEFAULT_FG)
            return ColorIndex::Foreground;
        else if (color.type & VTERM_COLOR_INDEXED) {
            if (color.indexed.idx >= 16) {
                VTermColor c = color;
                vterm_state_convert_color_to_rgb(vterm_obtain_state(m_vterm.get()), &c);
                return toQColor(c);
            }
            return color.indexed.idx;
        } else if (color.type == VTERM_COLOR_RGB)
            return toQColor(color);
        else
            return -1;
    }

    TerminalCell toCell(const VTermScreenCell &cell)
    {
        TerminalCell result;
        result.width = cell.width;
        result.text = QString::fromUcs4(reinterpret_cast<const char32_t *>(cell.chars));

        const VTermColor *bg = &cell.bg;
        const VTermColor *fg = &cell.fg;

        if (static_cast<bool>(cell.attrs.reverse))
            std::swap(fg, bg);

        result.backgroundColor = toVariantColor(*bg);
        result.foregroundColor = toVariantColor(*fg);

        result.bold = cell.attrs.bold;
        result.strikeOut = cell.attrs.strike;

        if (cell.attrs.underline > 0) {
            result.underlineStyle = QTextCharFormat::NoUnderline;
            switch (cell.attrs.underline) {
            case VTERM_UNDERLINE_SINGLE:
                result.underlineStyle = QTextCharFormat::SingleUnderline;
                break;
            case VTERM_UNDERLINE_DOUBLE:
                // TODO: Double underline
                result.underlineStyle = QTextCharFormat::SingleUnderline;
                break;
            case VTERM_UNDERLINE_CURLY:
                result.underlineStyle = QTextCharFormat::WaveUnderline;
                break;
            case VTERM_UNDERLINE_DASHED:
                result.underlineStyle = QTextCharFormat::DashUnderline;
                break;
            case VTERM_UNDERLINE_DOTTED:
                result.underlineStyle = QTextCharFormat::DotLine;
                break;
            }
        }

        result.strikeOut = cell.attrs.strike;

        return result;
    }

    // Callbacks from vterm
    void invalidate(VTermRect rect)
    {
        if (!m_altscreen) {
            rect.start_row += m_scrollback->size();
            rect.end_row += m_scrollback->size();
        }

        emit q->invalidated(
            QRect{QPoint{rect.start_col, rect.start_row}, QPoint{rect.end_col, rect.end_row - 1}});
    }

    int sb_pushline(int cols, const VTermScreenCell *cells)
    {
        auto oldSize = m_scrollback->size();
        m_scrollback->emplace(cols, cells);
        if (m_scrollback->size() != oldSize)
            emit q->fullSizeChanged(q->fullSize());
        return 1;
    }

    int sb_popline(int cols, VTermScreenCell *cells)
    {
        if (m_scrollback->size() == 0)
            return 0;

        m_scrollback->popto(cols, cells);
        emit q->fullSizeChanged(q->fullSize());
        return 1;
    }

    int sb_clear()
    {
        m_scrollback->clear();
        emit q->fullSizeChanged(q->fullSize());
        return 1;
    }

    int osc(int cmd, const VTermStringFragment &fragment)
    {
        if (m_surfaceIntegration) {
            m_surfaceIntegration->onOsc(cmd,
                                        {fragment.str, fragment.len},
                                        fragment.initial,
                                        fragment.final);
        }

        return 1;
    }

    int setTerminalProperties(VTermProp prop, VTermValue *val)
    {
        switch (prop) {
        case VTERM_PROP_CURSORVISIBLE: {
            Cursor old = q->cursor();
            m_cursor.visible = val->boolean;
            q->cursorChanged(old, q->cursor());
            break;
        }
        case VTERM_PROP_CURSORBLINK: {
            Cursor old = q->cursor();
            m_cursor.blink = val->boolean;
            emit q->cursorChanged(old, q->cursor());
            break;
        }
        case VTERM_PROP_CURSORSHAPE: {
            Cursor old = q->cursor();
            m_cursor.shape = (Cursor::Shape) val->number;
            emit q->cursorChanged(old, q->cursor());
            break;
        }
        case VTERM_PROP_ICONNAME:
            break;
        case VTERM_PROP_TITLE:
            if (m_surfaceIntegration)
                m_surfaceIntegration->onTitle(QString::fromUtf8(val->string.str, val->string.len));
            break;
        case VTERM_PROP_ALTSCREEN:
            m_altscreen = val->boolean;
            emit q->altscreenChanged(m_altscreen);
            break;
        case VTERM_PROP_MOUSE:
            qCDebug(log) << "Ignoring VTERM_PROP_MOUSE" << val->number;
            break;
        case VTERM_PROP_REVERSE:
            qCDebug(log) << "Ignoring VTERM_PROP_REVERSE" << val->boolean;
            break;
        case VTERM_N_PROPS:
            break;
        case VTERM_PROP_FOCUSREPORT:
            break;
        }
        return 1;
    }
    int movecursor(VTermPos pos, VTermPos oldpos, int visible)
    {
        Q_UNUSED(oldpos)
        Cursor oldCursor = q->cursor();
        m_cursor.position = {pos.col, pos.row};
        m_cursor.visible = visible > 0;
        q->cursorChanged(oldCursor, q->cursor());
        return 1;
    }

    const VTermScreenCell *cellAt(int x, int y)
    {
        if (y < 0 || x < 0 || y >= q->fullSize().height() || x >= liveSize().width()) {
            qCWarning(log) << "Invalid Parameter for cellAt:" << x << y << "liveSize:" << liveSize()
                           << "fullSize:" << q->fullSize();
            return nullptr;
        }

        if (!m_altscreen && y < m_scrollback->size()) {
            const auto &sbl = m_scrollback->line((m_scrollback->size() - 1) - y);
            if (x < sbl.cols()) {
                return sbl.cell(x);
            }
            return nullptr;
        }

        if (!m_altscreen)
            y -= m_scrollback->size();

        static VTermScreenCell refCell{};
        VTermPos vtp{y, x};
        vterm_screen_get_cell(m_vtermScreen, vtp, &refCell);

        return &refCell;
    }

    std::unique_ptr<VTerm, void (*)(VTerm *)> m_vterm;
    VTermScreen *m_vtermScreen;
    VTermScreenCallbacks m_vtermScreenCallbacks;
    VTermStateFallbacks m_vtermStateFallbacks;

    VTermSelectionCallbacks m_vtermSelectionCallbacks;

    Cursor m_cursor;
    QString m_currentCommand;

    bool m_altscreen{false};

    std::unique_ptr<Scrollback> m_scrollback;

    SurfaceIntegration *m_surfaceIntegration{nullptr};

    TerminalSurface *q;
    QTimer m_delayWriteTimer;
    QByteArray m_writeBuffer;
    QByteArray m_selectionBuffer;

    TerminalSurface::WriteToPty m_writeToPty;
};

TerminalSurface::TerminalSurface(QSize initialGridSize)
    : d(std::make_unique<TerminalSurfacePrivate>(this, initialGridSize))
{
    d->init();
}

TerminalSurface::~TerminalSurface() = default;

int TerminalSurface::cellWidthAt(int x, int y) const
{
    const VTermScreenCell *cell = d->cellAt(x, y);
    if (!cell)
        return 0;
    return cell->width;
}

QSize TerminalSurface::liveSize() const
{
    return d->liveSize();
}

QSize TerminalSurface::fullSize() const
{
    if (d->m_altscreen)
        return liveSize();
    return QSize{d->liveSize().width(), d->liveSize().height() + d->m_scrollback->size()};
}

std::u32string::value_type TerminalSurface::fetchCharAt(int x, int y) const
{
    const VTermScreenCell *cell = d->cellAt(x, y);
    if (!cell)
        return 0;

    if (cell->width == 0)
        return 0;

    if (cell->chars[0] == 0xffffffff)
        return 0;

    QString s = QString::fromUcs4(reinterpret_cast<const char32_t *>(cell->chars), 6)
                    .normalized(QString::NormalizationForm_C);
    const QList<uint> ucs4 = s.toUcs4();
    return std::u32string(ucs4.begin(), ucs4.end()).front();
}

TerminalCell TerminalSurface::fetchCell(int x, int y) const
{
    static TerminalCell emptyCell{1,
                                  {},
                                  {},
                                  false,
                                  ColorIndex::Foreground,
                                  ColorIndex::Background,
                                  QTextCharFormat::NoUnderline,
                                  false};

    if (y < 0 || y >= fullSize().height() || x >= fullSize().width()) {
        qCWarning(log) << "Invalid Parameter for fetchCell:" << x << y << "fullSize:" << fullSize();
        return emptyCell;
    }

    const VTermScreenCell *refCell = d->cellAt(x, y);
    if (!refCell)
        return emptyCell;

    return d->toCell(*refCell);
}

void TerminalSurface::clearAll()
{
    // Fake a scrollback clearing
    QByteArray data{"\x1b[3J"};
    vterm_input_write(d->m_vterm.get(), data.constData(), data.size());

    // Send Ctrl+L which will clear the screen
    d->m_writeToPty(QByteArray("\f"));
}

void TerminalSurface::resize(QSize newSize)
{
    vterm_set_size(d->m_vterm.get(), newSize.height(), newSize.width());
}

QPoint TerminalSurface::posToGrid(int pos) const
{
    return {pos % d->liveSize().width(), pos / d->liveSize().width()};
}
int TerminalSurface::gridToPos(QPoint gridPos) const
{
    return gridPos.y() * d->liveSize().width() + gridPos.x();
}

void TerminalSurface::dataFromPty(const QByteArray &data)
{
    vterm_input_write(d->m_vterm.get(), data.constData(), data.size());
    vterm_screen_flush_damage(d->m_vtermScreen);
}

void TerminalSurface::flush()
{
    vterm_screen_flush_damage(d->m_vtermScreen);
}

void TerminalSurface::pasteFromClipboard(const QString &clipboardText)
{
    if (clipboardText.isEmpty())
        return;

    vterm_keyboard_start_paste(d->m_vterm.get());
    for (unsigned int ch : clipboardText.toUcs4()) {
        // Workaround for weird nano behavior to correctly paste newlines
        // see: http://savannah.gnu.org/bugs/?49176
        // and: https://github.com/kovidgoyal/kitty/issues/994
        if (ch == '\n')
            ch = '\r';
        vterm_keyboard_unichar(d->m_vterm.get(), ch, VTERM_MOD_NONE);
    }
    vterm_keyboard_end_paste(d->m_vterm.get());

    if (!d->m_altscreen) {
        emit unscroll();
    }
}

void TerminalSurface::sendKey(Qt::Key key)
{
    if (key == Qt::Key_Escape)
        vterm_keyboard_key(d->m_vterm.get(), VTERM_KEY_ESCAPE, VTERM_MOD_NONE);
}

void TerminalSurface::sendKey(const QString &text)
{
    for (const unsigned int ch : text.toUcs4())
        vterm_keyboard_unichar(d->m_vterm.get(), ch, VTERM_MOD_NONE);
}

void TerminalSurface::sendKey(QKeyEvent *event)
{
    bool keypad = event->modifiers() & Qt::KeypadModifier;
    VTermModifier mod = qtModifierToVTerm(event->modifiers());
    VTermKey key = qtKeyToVTerm(Qt::Key(event->key()), keypad);

    if (key != VTERM_KEY_NONE) {
        if (mod == VTERM_MOD_SHIFT && (key == VTERM_KEY_ESCAPE || key == VTERM_KEY_BACKSPACE))
            mod = VTERM_MOD_NONE;

        vterm_keyboard_key(d->m_vterm.get(), key, mod);
    } else if (event->text().length() == 1) {
        // event->text() already contains the correct unicode character based on the modifiers
        // used, so we can simply convert it to Ucs4 and send it to the terminal.
        vterm_keyboard_unichar(d->m_vterm.get(), event->text().toUcs4()[0], VTERM_MOD_NONE);
    } else if (mod == VTERM_MOD_CTRL && event->key() >= Qt::Key_A && event->key() < Qt::Key_Z) {
        vterm_keyboard_unichar(d->m_vterm.get(), 'a' + (event->key() - Qt::Key_A), mod);
    }
}

Cursor TerminalSurface::cursor() const
{
    Cursor cursor = d->m_cursor;
    if (!d->m_altscreen)
        cursor.position.setY(cursor.position.y() + d->m_scrollback->size());

    return cursor;
}

SurfaceIntegration *TerminalSurface::surfaceIntegration() const
{
    return d->m_surfaceIntegration;
}

void TerminalSurface::setSurfaceIntegration(SurfaceIntegration *surfaceIntegration)
{
    d->m_surfaceIntegration = surfaceIntegration;
}

void TerminalSurface::mouseMove(QPoint pos, Qt::KeyboardModifiers modifiers)
{
    vterm_mouse_move(d->m_vterm.get(), pos.y(), pos.x(), qtModifierToVTerm(modifiers));
}

void TerminalSurface::mouseButton(Qt::MouseButton button,
                                  bool pressed,
                                  Qt::KeyboardModifiers modifiers)
{
    int btnIdx = 0;
    switch (button) {
    case Qt::LeftButton:
        btnIdx = 1;
        break;
    case Qt::RightButton:
        btnIdx = 3;
        break;
    case Qt::MiddleButton:
        btnIdx = 2;
        break;
    case Qt::ExtraButton1:
        btnIdx = 4;
        break;
    case Qt::ExtraButton2:
        btnIdx = 5;
        break;
    case Qt::ExtraButton3:
        btnIdx = 6;
        break;
    case Qt::ExtraButton4:
        btnIdx = 7;
        break;
    default:
        return;
    }

    vterm_mouse_button(d->m_vterm.get(), btnIdx, pressed, qtModifierToVTerm(modifiers));
}

void TerminalSurface::sendFocus(bool hasFocus)
{
    VTermState *vts = vterm_obtain_state(d->m_vterm.get());

    if (hasFocus)
        vterm_state_focus_in(vts);
    else
        vterm_state_focus_out(vts);
}

bool TerminalSurface::isInAltScreen()
{
    return d->m_altscreen;
}

void TerminalSurface::setWriteToPty(WriteToPty writeToPty)
{
    d->m_writeToPty = writeToPty;
}

CellIterator TerminalSurface::begin() const
{
    auto res = CellIterator(this, {0, 0});
    res.m_state = CellIterator::State::BEGIN;
    return res;
}

CellIterator TerminalSurface::end() const
{
    return CellIterator(this);
}

std::reverse_iterator<CellIterator> TerminalSurface::rbegin() const
{
    return std::make_reverse_iterator(end());
}

std::reverse_iterator<CellIterator> TerminalSurface::rend() const
{
    return std::make_reverse_iterator(begin());
}

CellIterator TerminalSurface::iteratorAt(QPoint pos) const
{
    return CellIterator(this, pos);
}
CellIterator TerminalSurface::iteratorAt(int pos) const
{
    return CellIterator(this, pos);
}

std::reverse_iterator<CellIterator> TerminalSurface::rIteratorAt(QPoint pos) const
{
    return std::make_reverse_iterator(iteratorAt(pos));
}

std::reverse_iterator<CellIterator> TerminalSurface::rIteratorAt(int pos) const
{
    return std::make_reverse_iterator(iteratorAt(pos));
}

void TerminalSurface::enableLiveReflow(bool enable)
{
    vterm_screen_enable_reflow(d->m_vtermScreen, enable);
}

} // namespace TerminalSolution
