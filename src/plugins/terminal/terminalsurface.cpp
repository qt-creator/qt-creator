// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalsurface.h"

#include "keys.h"
#include "scrollback.h"

#include <utils/qtcassert.h>

#include <vterm.h>

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(log, "qtc.terminal.surface", QtWarningMsg);

namespace Terminal::Internal {

QColor toQColor(const VTermColor &c)
{
    return QColor(qRgb(c.rgb.red, c.rgb.green, c.rgb.blue));
};

struct TerminalSurfacePrivate
{
    TerminalSurfacePrivate(TerminalSurface *surface,
                           const QSize &initialGridSize,
                           ShellIntegration *shellIntegration)
        : m_vterm(vterm_new(initialGridSize.height(), initialGridSize.width()), vterm_free)
        , m_vtermScreen(vterm_obtain_screen(m_vterm.get()))
        , m_scrollback(std::make_unique<Internal::Scrollback>(5000))
        , m_shellIntegration(shellIntegration)
        , q(surface)
    {}

    void init()
    {
        vterm_set_utf8(m_vterm.get(), true);

        static auto writeToPty = [](const char *s, size_t len, void *user) {
            auto p = static_cast<TerminalSurfacePrivate *>(user);
            emit p->q->writeToPty(QByteArray(s, static_cast<int>(len)));
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
            return p->sb_clear();
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
        vterm_state_set_bold_highbright(vts, true);

        vterm_screen_reset(m_vtermScreen, 1);
    }

    QSize liveSize() const
    {
        int rows;
        int cols;
        vterm_get_size(m_vterm.get(), &rows, &cols);

        return QSize(cols, rows);
    }

    TerminalCell toCell(const VTermScreenCell &cell)
    {
        TerminalCell result;
        result.width = cell.width;
        result.text = QString::fromUcs4(cell.chars);

        const VTermColor *bg = &cell.bg;
        const VTermColor *fg = &cell.fg;

        if (static_cast<bool>(cell.attrs.reverse))
            std::swap(fg, bg);

        const QColor cellBgColor = toQColor(*bg);
        const QColor cellFgColor = toQColor(*fg);

        if (cellBgColor != m_defaultBgColor)
            result.background = toQColor(*bg);

        result.foreground = cellFgColor;

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

    VTermColor defaultBgColor() const
    {
        VTermColor defaultBg;
        if (!m_altscreen) {
            VTermColor defaultFg;
            vterm_state_get_default_colors(vterm_obtain_state(m_vterm.get()),
                                           &defaultFg,
                                           &defaultBg);
            // We want to compare the cell bg against this later and cells don't
            // set DEFAULT_BG
            defaultBg.type = VTERM_COLOR_RGB;
            return defaultBg;
        } // This is a slightly better guess when in an altscreen

        VTermPos vtp{0, 0};
        static VTermScreenCell refCell{};
        vterm_screen_get_cell(m_vtermScreen, vtp, &refCell);
        return refCell.bg;
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
        m_scrollback->emplace(cols, cells, vterm_obtain_state(m_vterm.get()));
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
        if (m_shellIntegration)
            m_shellIntegration->onOsc(cmd, fragment);

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
        }
        return 1;
    }
    int movecursor(VTermPos pos, VTermPos oldpos, int visible)
    {
        Q_UNUSED(oldpos);
        Cursor oldCursor = q->cursor();
        m_cursor.position = {pos.col, pos.row};
        m_cursor.visible = visible > 0;
        q->cursorChanged(oldCursor, q->cursor());
        return 1;
    }

    const VTermScreenCell *cellAt(int x, int y)
    {
        QTC_ASSERT(y >= 0 && x >= 0, return nullptr);
        QTC_ASSERT(y < q->fullSize().height() && x < liveSize().width(), return nullptr);

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
        vterm_screen_convert_color_to_rgb(m_vtermScreen, &refCell.fg);
        vterm_screen_convert_color_to_rgb(m_vtermScreen, &refCell.bg);

        return &refCell;
    }

    std::unique_ptr<VTerm, void (*)(VTerm *)> m_vterm;
    VTermScreen *m_vtermScreen;
    VTermScreenCallbacks m_vtermScreenCallbacks;
    VTermStateFallbacks m_vtermStateFallbacks;

    QColor m_defaultBgColor;
    Cursor m_cursor;
    QString m_currentCommand;

    bool m_altscreen{false};

    std::unique_ptr<Internal::Scrollback> m_scrollback;

    ShellIntegration *m_shellIntegration{nullptr};

    TerminalSurface *q;
};

TerminalSurface::TerminalSurface(QSize initialGridSize, ShellIntegration *shellIntegration)
    : d(std::make_unique<TerminalSurfacePrivate>(this, initialGridSize, shellIntegration))
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

    QString s = QString::fromUcs4(cell->chars, 6).normalized(QString::NormalizationForm_C);
    const QList<uint> ucs4 = s.toUcs4();
    return std::u32string(ucs4.begin(), ucs4.end()).front();
}

TerminalCell TerminalSurface::fetchCell(int x, int y) const
{
    static TerminalCell
        emptyCell{1, {}, {}, false, {}, std::nullopt, QTextCharFormat::NoUnderline, false};

    QTC_ASSERT(y >= 0, return emptyCell);
    QTC_ASSERT(y < fullSize().height() && x < fullSize().width(), return emptyCell);

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
    emit writeToPty(QByteArray("\f"));
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

void TerminalSurface::setColors(QColor foreground, QColor background)
{
    VTermState *vts = vterm_obtain_state(d->m_vterm.get());

    VTermColor fg;
    VTermColor bg;

    vterm_color_rgb(&fg, foreground.red(), foreground.green(), foreground.blue());
    vterm_color_rgb(&bg, background.red(), background.green(), background.blue());

    d->m_defaultBgColor = background;

    vterm_state_set_default_colors(vts, &fg, &bg);
    vterm_screen_reset(d->m_vtermScreen, 1);
}

void TerminalSurface::setAnsiColor(int index, QColor color)
{
    VTermState *vts = vterm_obtain_state(d->m_vterm.get());

    VTermColor col;
    vterm_color_rgb(&col, color.red(), color.green(), color.blue());
    vterm_state_set_palette_color(vts, index, &col);

    vterm_screen_reset(d->m_vtermScreen, 1);
}

void TerminalSurface::pasteFromClipboard(const QString &clipboardText)
{
    if (clipboardText.isEmpty())
        return;

    vterm_keyboard_start_paste(d->m_vterm.get());
    for (unsigned int ch : clipboardText.toUcs4())
        vterm_keyboard_unichar(d->m_vterm.get(), ch, VTERM_MOD_NONE);
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
    VTermModifier mod = Internal::qtModifierToVTerm(event->modifiers());
    VTermKey key = Internal::qtKeyToVTerm(Qt::Key(event->key()), keypad);

    if (key != VTERM_KEY_NONE) {
        if (mod == VTERM_MOD_SHIFT && (key == VTERM_KEY_ESCAPE || key == VTERM_KEY_BACKSPACE))
            mod = VTERM_MOD_NONE;

        vterm_keyboard_key(d->m_vterm.get(), key, mod);
    } else if (event->text().length() == 1) {
        // This maps to delete word and is way to easy to mistakenly type
        //        if (event->key() == Qt::Key_Space && mod == VTERM_MOD_SHIFT)
        //            mod = VTERM_MOD_NONE;

        // Per https://github.com/justinmk/neovim/commit/317d5ca7b0f92ef42de989b3556ca9503f0a3bf6
        // libvterm prefers we send the full keycode rather than sending the
        // ctrl modifier.  This helps with ncurses applications which otherwise
        // do not recognize ctrl+<key> and in the shell for getting common control characters
        // like ctrl+i for tab or ctrl+j for newline.

        // Workaround for "ALT+SHIFT+/" (\ on german mac keyboards)
        if (mod == (VTERM_MOD_SHIFT | VTERM_MOD_ALT) && event->key() == Qt::Key_Slash) {
            mod = VTERM_MOD_NONE;
        }

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

QColor TerminalSurface::defaultBgColor() const
{
    return toQColor(d->defaultBgColor());
}

ShellIntegration *TerminalSurface::shellIntegration() const
{
    return d->m_shellIntegration;
}

CellIterator TerminalSurface::begin() const
{
    auto res = CellIterator(this, {0, 0});
    res.m_state = CellIterator::State::BEGIN;
    return res;
}

CellIterator TerminalSurface::end() const
{
    return CellIterator(this, CellIterator::State::END);
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

} // namespace Terminal::Internal
