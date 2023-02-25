// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalwidget.h"
#include "celllayout.h"
#include "keys.h"
#include "terminalsettings.h"
#include "terminaltr.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/stringutils.h>

#include <ptyqt.h>
#include <vterm.h>

#include <QApplication>
#include <QClipboard>
#include <QGlyphRun>
#include <QLoggingCategory>
#include <QPaintEvent>
#include <QPainter>
#include <QRawFont>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextLayout>

Q_LOGGING_CATEGORY(terminalLog, "qtc.terminal", QtWarningMsg)

using namespace Utils;
using namespace Utils::Terminal;

namespace Terminal {

TerminalWidget::TerminalWidget(QWidget *parent, const OpenTerminalParameters &openParameters)
    : QAbstractScrollArea(parent)
    , m_vterm(vterm_new(size().height(), size().width()), vterm_free)
    , m_vtermScreen(vterm_obtain_screen(m_vterm.get()))
    , m_scrollback(std::make_unique<Internal::Scrollback>(5000))
    , m_copyAction(Tr::tr("Copy"))
    , m_pasteAction(Tr::tr("Paste"))
    , m_clearSelectionAction(Tr::tr("Clear Selection"))
    , m_zoomInAction(Tr::tr("Zoom In"))
    , m_zoomOutAction(Tr::tr("Zoom Out"))
    , m_openParameters(openParameters)
{
    setupVTerm();
    setupFont();
    setupColors();

    setAttribute(Qt::WA_InputMethodEnabled);
    setAttribute(Qt::WA_MouseTracking);

    setCursor(Qt::IBeamCursor);

    m_textLayout.setCacheEnabled(true);

    setFocus();
    setFocusPolicy(Qt::StrongFocus);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_OpaquePaintEvent);

    m_readDelayTimer.setSingleShot(true);
    m_readDelayTimer.setInterval(10);

    connect(&m_readDelayTimer, &QTimer::timeout, this, [this]() {
        m_readDelayRestarts = 0;
        onReadyRead();
    });

    connect(&m_copyAction, &QAction::triggered, this, &TerminalWidget::copyToClipboard);
    connect(&m_pasteAction, &QAction::triggered, this, &TerminalWidget::pasteFromClipboard);
    connect(&m_clearSelectionAction, &QAction::triggered, this, &TerminalWidget::clearSelection);
    connect(&m_zoomInAction, &QAction::triggered, this, &TerminalWidget::zoomIn);
    connect(&m_zoomOutAction, &QAction::triggered, this, &TerminalWidget::zoomOut);

    connect(&TerminalSettings::instance(), &AspectContainer::applied, this, [this]() {
        m_layoutVersion++;
        // Setup colors first, as setupFont will redraw the screen.
        setupColors();
        setupFont();
    });
}

void TerminalWidget::setupPty()
{
    m_ptyProcess.reset(PtyQt::createPtyProcess(IPtyProcess::PtyType::AutoPty));

    Environment env = m_openParameters.environment.value_or(Environment::systemEnvironment());

    CommandLine shellCommand = m_openParameters.shellCommand.value_or(
        CommandLine{TerminalSettings::instance().shell.filePath(), {}});

    // For git bash on Windows
    env.prependOrSetPath(shellCommand.executable().parentDir());

    QStringList envList = filtered(env.toStringList(), [](const QString &envPair) {
        return envPair != "CLINK_NOAUTORUN=1";
    });

    m_ptyProcess->startProcess(shellCommand.executable().nativePath(),
                               shellCommand.splitArguments(),
                               m_openParameters.workingDirectory
                                   .value_or(FilePath::fromString(QDir::homePath()))
                                   .nativePath(),
                               envList,
                               m_vtermSize.width(),
                               m_vtermSize.height());

    emit started(m_ptyProcess->pid());

    if (!m_ptyProcess->lastError().isEmpty()) {
        qCWarning(terminalLog) << m_ptyProcess->lastError();
        m_ptyProcess.reset();
        return;
    }

    connect(m_ptyProcess->notifier(), &QIODevice::readyRead, this, [this]() {
        if (m_readDelayTimer.isActive())
            m_readDelayRestarts++;

        if (m_readDelayRestarts > 100)
            return;

        m_readDelayTimer.start();
    });

    connect(m_ptyProcess->notifier(), &QIODevice::aboutToClose, this, [this]() {
        m_cursor.visible = false;
        if (m_ptyProcess) {
            onReadyRead();

            if (m_ptyProcess->exitCode() != 0) {
                QByteArray msg = QString("\r\n\033[31mProcess exited with code: %1")
                                     .arg(m_ptyProcess->exitCode())
                                     .toUtf8();

                vterm_input_write(m_vterm.get(), msg.constData(), msg.size());
                vterm_screen_flush_damage(m_vtermScreen);

                return;
            }
        }

        if (m_openParameters.m_exitBehavior == ExitBehavior::Restart) {
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    m_ptyProcess.reset();
                    setupPty();
                },
                Qt::QueuedConnection);
        }

        if (m_openParameters.m_exitBehavior == ExitBehavior::Close)
            deleteLater();

        if (m_openParameters.m_exitBehavior == ExitBehavior::Keep) {
            QByteArray msg = QString("\r\nProcess exited with code: %1")
                                 .arg(m_ptyProcess ? m_ptyProcess->exitCode() : -1)
                                 .toUtf8();

            vterm_input_write(m_vterm.get(), msg.constData(), msg.size());
            vterm_screen_flush_damage(m_vtermScreen);
        }
    });
}

void TerminalWidget::setupFont()
{
    QFont f;
    f.setFixedPitch(true);
    f.setFamily(TerminalSettings::instance().font.value());
    f.setPointSize(TerminalSettings::instance().fontSize.value());

    setFont(f);
}

void TerminalWidget::setupColors()
{
    // Check if the colors have changed.
    std::array<QColor, 18> newColors;
    for (int i = 0; i < 16; ++i) {
        newColors[i] = TerminalSettings::instance().colors[i].value();
    }
    newColors[16] = TerminalSettings::instance().foregroundColor.value();
    newColors[17] = TerminalSettings::instance().backgroundColor.value();

    if (m_currentColors == newColors)
        return;

    m_currentColors = newColors;

    VTermState *vts = vterm_obtain_state(m_vterm.get());

    auto setColor = [vts](int index, uint8_t r, uint8_t g, uint8_t b) {
        VTermColor col;
        vterm_color_rgb(&col, r, g, b);
        vterm_state_set_palette_color(vts, index, &col);
    };

    for (int i = 0; i < 16; ++i) {
        QColor c = TerminalSettings::instance().colors[i].value();
        setColor(i, c.red(), c.green(), c.blue());
    }

    VTermColor fg;
    VTermColor bg;

    vterm_color_rgb(&fg,
                    TerminalSettings::instance().foregroundColor.value().red(),
                    TerminalSettings::instance().foregroundColor.value().green(),
                    TerminalSettings::instance().foregroundColor.value().blue());
    vterm_color_rgb(&bg,
                    TerminalSettings::instance().backgroundColor.value().red(),
                    TerminalSettings::instance().backgroundColor.value().green(),
                    TerminalSettings::instance().backgroundColor.value().blue());

    vterm_state_set_default_colors(vts, &fg, &bg);

    clearContents();
}

void TerminalWidget::writeToPty(const QByteArray &data)
{
    if (m_ptyProcess)
        m_ptyProcess->write(data);
}

void TerminalWidget::setupVTerm()
{
    vterm_set_utf8(m_vterm.get(), true);

    static auto writeToPty = [](const char *s, size_t len, void *user) {
        auto p = static_cast<TerminalWidget *>(user);
        p->writeToPty(QByteArray(s, static_cast<int>(len)));
    };

    vterm_output_set_callback(m_vterm.get(), writeToPty, this);

    memset(&m_vtermScreenCallbacks, 0, sizeof(m_vtermScreenCallbacks));

    m_vtermScreenCallbacks.damage = [](VTermRect rect, void *user) {
        auto p = static_cast<TerminalWidget *>(user);
        p->invalidate(rect);
        return 1;
    };
    m_vtermScreenCallbacks.sb_pushline = [](int cols, const VTermScreenCell *cells, void *user) {
        auto p = static_cast<TerminalWidget *>(user);
        return p->sb_pushline(cols, cells);
    };
    m_vtermScreenCallbacks.sb_popline = [](int cols, VTermScreenCell *cells, void *user) {
        auto p = static_cast<TerminalWidget *>(user);
        return p->sb_popline(cols, cells);
    };
    m_vtermScreenCallbacks.settermprop = [](VTermProp prop, VTermValue *val, void *user) {
        auto p = static_cast<TerminalWidget *>(user);
        return p->setTerminalProperties(prop, val);
    };
    m_vtermScreenCallbacks.movecursor = [](VTermPos pos, VTermPos oldpos, int visible, void *user) {
        auto p = static_cast<TerminalWidget *>(user);
        return p->movecursor(pos, oldpos, visible);
    };

    m_vtermScreenCallbacks.sb_clear = [](void *user) {
        auto p = static_cast<TerminalWidget *>(user);
        return p->sb_clear();
    };

    vterm_screen_set_callbacks(m_vtermScreen, &m_vtermScreenCallbacks, this);
    vterm_screen_set_damage_merge(m_vtermScreen, VTERM_DAMAGE_SCROLL);
    vterm_screen_enable_altscreen(m_vtermScreen, true);

    VTermState *vts = vterm_obtain_state(m_vterm.get());
    vterm_state_set_bold_highbright(vts, true);

    vterm_screen_reset(m_vtermScreen, 1);
}

void TerminalWidget::setFont(const QFont &font)
{
    m_font = font;

    //QRawFont rawFont = QRawFont::fromFont(m_font);
    m_textLayout.setFont(m_font);

    QFontMetricsF qfm{m_font};
    const auto w = [qfm]() -> qreal {
        if (HostOsInfo::isMacHost())
            return qfm.maxWidth();
        return qfm.averageCharWidth();
    }();

    qCInfo(terminalLog) << font.family() << font.pointSize() << w << size();

    m_cellSize = {w, qfm.height()};
    m_cellBaseline = qfm.ascent();
    m_lineSpacing = qfm.height();

    QAbstractScrollArea::setFont(m_font);

    if (m_ptyProcess) {
        applySizeChange();
    }
}

QAction &TerminalWidget::copyAction()
{
    return m_copyAction;
}

QAction &TerminalWidget::pasteAction()
{
    return m_pasteAction;
}

QAction &TerminalWidget::clearSelectionAction()
{
    return m_clearSelectionAction;
}

QAction &TerminalWidget::zoomInAction()
{
    return m_zoomInAction;
}

QAction &TerminalWidget::zoomOutAction()
{
    return m_zoomOutAction;
}

void TerminalWidget::copyToClipboard() const
{
    if (m_selection) {
        const size_t startLine = qFloor(m_selection->start.y() / m_lineSpacing);
        const size_t endLine = qFloor(m_selection->end.y() / m_lineSpacing);

        QString selectedText;
        size_t row = startLine;
        for (; row < m_scrollback->size(); row++) {
            const Internal::Scrollback::Line &line = m_scrollback->line((m_scrollback->size() - 1)
                                                                        - row);
            if (row > endLine)
                break;

            const QTextLayout &layout = line.layout(m_layoutVersion, m_font, m_lineSpacing);
            const std::optional<QTextLayout::FormatRange> range
                = selectionToFormatRange(*m_selection, layout, row);
            if (range)
                selectedText.append(line.layout(m_layoutVersion, m_font, m_lineSpacing)
                                        .text()
                                        .mid(range->start, range->length)
                                        .trimmed());

            if (endLine > row)
                selectedText.append(QChar::LineFeed);
        }

        if (row <= endLine) {
            const std::optional<QTextLayout::FormatRange> range
                = selectionToFormatRange(*m_selection, m_textLayout, m_scrollback->size());
            if (range)
                selectedText.append(m_textLayout.text()
                                        .mid(range->start, range->length)
                                        .replace(QChar::LineSeparator, QChar::LineFeed)
                                        .trimmed());
        }

        setClipboardAndSelection(selectedText);
    }
}
void TerminalWidget::pasteFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    const QString clipboardText = clipboard->text(QClipboard::Clipboard);

    if (clipboardText.isEmpty())
        return;

    vterm_keyboard_start_paste(m_vterm.get());
    for (unsigned int ch : clipboardText.toUcs4())
        vterm_keyboard_unichar(m_vterm.get(), ch, VTERM_MOD_NONE);
    vterm_keyboard_end_paste(m_vterm.get());

    if (!m_altscreen && m_scrollback->offset()) {
        m_scrollback->unscroll();
        viewport()->update();
    }
}

void TerminalWidget::clearSelection()
{
    m_selection.reset();
    update();
}
void TerminalWidget::zoomIn()
{
    m_layoutVersion++;
    m_font.setPointSize(m_font.pointSize() + 1);
    setFont(m_font);
}
void TerminalWidget::zoomOut()
{
    m_layoutVersion++;
    m_font.setPointSize(qMax(m_font.pointSize() - 1, 1));
    setFont(m_font);
}

void TerminalWidget::clearContents()
{
    // Fake a scrollback clearing
    QByteArray data{"\x1b[3J"};
    vterm_input_write(m_vterm.get(), data.constData(), data.size());
    vterm_screen_flush_damage(m_vtermScreen);

    // Send Ctrl+L which will clear the screen
    writeToPty(QByteArray("\f"));
}

void TerminalWidget::onReadyRead()
{
    QByteArray data = m_ptyProcess->readAll();
    vterm_input_write(m_vterm.get(), data.constData(), data.size());
    vterm_screen_flush_damage(m_vtermScreen);
}

const VTermScreenCell *TerminalWidget::fetchCell(int x, int y) const
{
    QTC_ASSERT(y >= 0, return nullptr);
    QTC_ASSERT(y < m_vtermSize.height(), return nullptr);

    static VTermScreenCell refCell{};
    VTermPos vtp{y, x};
    vterm_screen_get_cell(m_vtermScreen, vtp, &refCell);
    vterm_screen_convert_color_to_rgb(m_vtermScreen, &refCell.fg);
    vterm_screen_convert_color_to_rgb(m_vtermScreen, &refCell.bg);
    return &refCell;
};

QPoint TerminalWidget::viewportToGlobal(QPoint p) const
{
    int y = p.y() - topMargin();
    const double offset = (m_scrollback->size() - m_scrollback->offset()) * m_lineSpacing;
    y += offset;

    return {p.x(), y};
}

void TerminalWidget::createTextLayout()
{
    QElapsedTimer t;
    t.start();

    VTermColor defaultBg;
    if (!m_altscreen) {
        VTermColor defaultFg;
        vterm_state_get_default_colors(vterm_obtain_state(m_vterm.get()), &defaultFg, &defaultBg);
        // We want to compare the cell bg against this later and cells don't
        // set DEFAULT_BG
        defaultBg.type = VTERM_COLOR_RGB;
    } else {
        // This is a slightly better guess when in an altscreen
        const VTermScreenCell *cell = fetchCell(0, 0);
        defaultBg = cell->bg;
    }

    m_textLayout.clearLayout();

    QString allText;

    Internal::createTextLayout(m_textLayout,
                               allText,
                               defaultBg,
                               QRect({0, 0}, m_vtermSize),
                               m_lineSpacing,
                               [this](int x, int y) { return fetchCell(x, y); });

    qCInfo(terminalLog) << "createTextLayout took:" << t.elapsed() << "ms";
}

qreal TerminalWidget::topMargin() const
{
    return size().height() - (m_vtermSize.height() * m_lineSpacing);
}

std::optional<QTextLayout::FormatRange> TerminalWidget::selectionToFormatRange(
    TerminalWidget::Selection selection, const QTextLayout &layout, int rowOffset) const
{
    int selectionStartLine = qFloor(selection.start.y() / m_lineSpacing) - rowOffset;
    int selectionEndLine = qFloor(selection.end.y() / m_lineSpacing) - rowOffset;

    int nRows = layout.lineCount();

    if (selectionStartLine < nRows && selectionEndLine >= 0) {
        QTextLine lStart = layout.lineAt(qMax(0, qMin(selectionStartLine, nRows)));
        QTextLine lEnd = layout.lineAt(qMin(nRows - 1, qMax(0, selectionEndLine)));

        int startPos = 0;
        int endPos = lEnd.textLength();

        if (selectionStartLine >= 0)
            startPos = lStart.xToCursor(selection.start.x());
        if (selectionEndLine < (nRows))
            endPos = lEnd.xToCursor(selection.end.x());

        QTextLayout::FormatRange range;
        range.start = startPos;
        range.length = endPos - startPos;
        range.format.setBackground(TerminalSettings::instance().selectionColor.value());
        return range;
    }

    return {};
}

void TerminalWidget::paintEvent(QPaintEvent *event)
{
    event->accept();
    QPainter p(viewport());

    p.setCompositionMode(QPainter::CompositionMode_Source);

    VTermColor defaultBg;
    if (!m_altscreen) {
        VTermColor defaultFg;
        vterm_state_get_default_colors(vterm_obtain_state(m_vterm.get()), &defaultFg, &defaultBg);
        // We want to compare the cell bg against this later and cells don't
        // set DEFAULT_BG
        defaultBg.type = VTERM_COLOR_RGB;
    } else {
        // This is a slightly better guess when in an altscreen
        const VTermScreenCell *cell = fetchCell(0, 0);
        defaultBg = cell->bg;
    }

    p.fillRect(event->rect(), Internal::toQColor(defaultBg));

    unsigned long off = m_scrollback->size() - m_scrollback->offset();

    // transform painter according to scroll offsets
    QPointF offset{0, -(off * m_lineSpacing)};

    qreal margin = topMargin();
    qreal y = offset.y() + margin;

    size_t row = qFloor((offset.y() * -1) / m_lineSpacing);
    y += row * m_lineSpacing;
    for (; row < m_scrollback->size(); row++) {
        if (y >= 0 && y < viewport()->height()) {
            const Internal::Scrollback::Line &line = m_scrollback->line((m_scrollback->size() - 1)
                                                                        - row);

            QList<QTextLayout::FormatRange> selections;

            if (m_selection) {
                const std::optional<QTextLayout::FormatRange> range
                    = selectionToFormatRange(m_selection.value(),
                                             line.layout(m_layoutVersion, m_font, m_lineSpacing),
                                             row);
                if (range) {
                    selections.append(range.value());
                }
            }
            line.layout(m_layoutVersion, m_font, m_lineSpacing).draw(&p, {0.0, y}, selections);
        }

        y += m_lineSpacing;
    }

    // Draw the live part
    if (y < m_vtermSize.height() * m_lineSpacing) {
        QList<QTextLayout::FormatRange> selections;

        if (m_selection) {
            const std::optional<QTextLayout::FormatRange> range
                = selectionToFormatRange(m_selection.value(), m_textLayout, row);
            if (range) {
                selections.append(range.value());
            }
        }

        m_textLayout.draw(&p, {0.0, y}, selections);

        if (m_cursor.visible && m_preEditString.isEmpty()) {
            p.setPen(QColor::fromRgb(0xFF, 0xFF, 0xFF));
            if (m_textLayout.lineCount() > m_cursor.row) {
                QTextLine cursorLine = m_textLayout.lineAt(m_cursor.row);
                if (cursorLine.isValid()) {
                    QFontMetricsF fm(m_font);
                    const QString text = m_textLayout.text();
                    const QList<uint> asUcs4 = text.toUcs4();
                    const int textStart = cursorLine.textStart();
                    const int cPos = textStart + m_cursor.col;
                    if (cPos >= 0 && cPos < asUcs4.size()) {
                        const unsigned int ch = asUcs4.at(cPos);
                        const qreal br = fm.horizontalAdvance(QString::fromUcs4(&ch, 1));
                        const qreal xCursor = cursorLine.cursorToX(cPos);
                        const double yCursor = cursorLine.y() + y;
                        const QRectF cursorRect = QRectF{xCursor, yCursor, br, m_lineSpacing};
                        if (hasFocus()) {
                            QPainter::CompositionMode oldMode = p.compositionMode();
                            p.setCompositionMode(QPainter::RasterOp_NotDestination);
                            p.fillRect(cursorRect, p.pen().brush());
                            p.setCompositionMode(oldMode);
                        } else {
                            p.drawRect(cursorRect);
                        }
                    }
                }
            }
        }

        if (!m_preEditString.isEmpty()) {
            // TODO: Use QTextLayout::setPreeditArea() instead ?
            QTextLine cursorLine = m_textLayout.lineAt(m_cursor.row);
            if (cursorLine.isValid()) {
                int pos = cursorLine.textStart() + m_cursor.col;
                QPointF displayPos = QPointF{cursorLine.cursorToX(pos), cursorLine.y()};

                p.fillRect(QRectF{displayPos.toPoint(), m_cellSize}, QColor::fromRgb(0, 0, 0));
                p.setPen(Qt::white);
                displayPos.setY(displayPos.y() + m_cellBaseline);
                p.drawText(displayPos, m_preEditString);
            }
        }
    }

    p.fillRect(QRectF{{0, 0}, QSizeF{(qreal) width(), margin}}, Internal::toQColor(defaultBg));
}

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    event->accept();

    if (event->modifiers() == Qt::NoModifier && event->key() == Qt::Key_Escape && m_selection) {
        clearSelectionAction().trigger();
        return;
    }

    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Plus) {
        zoomInAction().trigger();
        return;
    }

    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Minus) {
        zoomOutAction().trigger();
        return;
    }

    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_C) {
        copyAction().trigger();
        return;
    }

    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_V) {
        pasteAction().trigger();
        return;
    }

    bool keypad = event->modifiers() & Qt::KeypadModifier;
    VTermModifier mod = Internal::qtModifierToVTerm(event->modifiers());
    VTermKey key = Internal::qtKeyToVTerm(Qt::Key(event->key()), keypad);

    if (key != VTERM_KEY_NONE) {
        if (mod == VTERM_MOD_SHIFT && (key == VTERM_KEY_ESCAPE || key == VTERM_KEY_BACKSPACE))
            mod = VTERM_MOD_NONE;

        vterm_keyboard_key(m_vterm.get(), key, mod);
    } else if (event->text().length() == 1) {
        // This maps to delete word and is way to easy to mistakenly type
        //        if (event->key() == Qt::Key_Space && mod == VTERM_MOD_SHIFT)
        //            mod = VTERM_MOD_NONE;

        // Per https://github.com/justinmk/neovim/commit/317d5ca7b0f92ef42de989b3556ca9503f0a3bf6
        // libvterm prefers we send the full keycode rather than sending the
        // ctrl modifier.  This helps with ncurses applications which otherwise
        // do not recognize ctrl+<key> and in the shell for getting common control characters
        // like ctrl+i for tab or ctrl+j for newline.
        vterm_keyboard_unichar(m_vterm.get(),
                               event->text().toUcs4()[0],
                               static_cast<VTermModifier>(mod & ~VTERM_MOD_CTRL));
    } else if (mod != VTERM_MOD_NONE && event->key() == Qt::Key_C) {
        vterm_keyboard_unichar(m_vterm.get(), 'c', mod);
    }
}

void TerminalWidget::applySizeChange()
{
    m_vtermSize = {
        qFloor((qreal) size().width() / (qreal) m_cellSize.width()),
        qFloor((qreal) size().height() / m_lineSpacing),
    };

    if (m_vtermSize.height() <= 0)
        m_vtermSize.setHeight(1);

    if (m_vtermSize.width() <= 0)
        m_vtermSize.setWidth(1);

    if (m_ptyProcess)
        m_ptyProcess->resize(m_vtermSize.width(), m_vtermSize.height());

    vterm_set_size(m_vterm.get(), m_vtermSize.height(), m_vtermSize.width());
    vterm_screen_flush_damage(m_vtermScreen);
}

void TerminalWidget::updateScrollBars()
{
    verticalScrollBar()->setRange(0, static_cast<int>(m_scrollback->size()));
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void TerminalWidget::resizeEvent(QResizeEvent *event)
{
    event->accept();

    // If increasing in size, we'll trigger libvterm to call sb_popline in
    // order to pull lines out of the history.  This will cause the scrollback
    // to decrease in size which reduces the size of the verticalScrollBar.
    // That will trigger a scroll offset increase which we want to ignore.
    m_ignoreScroll = true;

    applySizeChange();

    m_selection.reset();
    m_ignoreScroll = false;
}

void TerminalWidget::invalidate(VTermRect rect)
{
    Q_UNUSED(rect);
    createTextLayout();

    viewport()->update();
}

int TerminalWidget::sb_pushline(int cols, const VTermScreenCell *cells)
{
    m_scrollback->emplace(cols, cells, vterm_obtain_state(m_vterm.get()));

    updateScrollBars();

    return 1;
}

int TerminalWidget::sb_popline(int cols, VTermScreenCell *cells)
{
    if (m_scrollback->size() == 0)
        return 0;

    m_scrollback->popto(cols, cells);

    updateScrollBars();

    return 1;
}

int TerminalWidget::sb_clear()
{
    m_scrollback->clear();
    updateScrollBars();

    return 1;
}

void TerminalWidget::wheelEvent(QWheelEvent *event)
{
    event->accept();

    QPoint delta = event->angleDelta();
    scrollContentsBy(0, delta.y() / 24);
}

void TerminalWidget::focusInEvent(QFocusEvent *)
{
    viewport()->update();
}
void TerminalWidget::focusOutEvent(QFocusEvent *)
{
    viewport()->update();
}

void TerminalWidget::inputMethodEvent(QInputMethodEvent *event)
{
    m_preEditString = event->preeditString();

    if (event->commitString().isEmpty()) {
        viewport()->update();
        return;
    }

    for (const unsigned int ch : event->commitString().toUcs4()) {
        vterm_keyboard_unichar(m_vterm.get(), ch, VTERM_MOD_NONE);
    }
}

void TerminalWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_selectionStartPos = event->pos();

        QPoint pos = viewportToGlobal(event->pos());
        m_selection = Selection{pos, pos};
        viewport()->update();
    }
}
void TerminalWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selection && event->buttons() & Qt::LeftButton) {
        QPoint start = viewportToGlobal(m_selectionStartPos);
        QPoint newEnd = viewportToGlobal(event->pos());

        if (start.y() > newEnd.y() || (start.y() == newEnd.y() && start.x() > newEnd.x()))
            std::swap(start, newEnd);

        m_selection->start = start;
        m_selection->end = newEnd;

        viewport()->update();
    }
}

void TerminalWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_selection && event->button() != Qt::LeftButton) {
        if ((m_selectionStartPos - event->pos()).manhattanLength() < 2) {
            m_selection.reset();
            viewport()->update();
        }
    }
}

void TerminalWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    // TODO :(
    Q_UNUSED(event);
    viewport()->update();
}

void TerminalWidget::scrollContentsBy(int dx, int dy)
{
    Q_UNUSED(dx);

    if (m_ignoreScroll)
        return;

    if (m_altscreen)
        return;

    size_t orig = m_scrollback->offset();
    size_t offset = m_scrollback->scroll(dy);
    if (orig == offset)
        return;

    m_cursor.visible = (offset == 0);
    viewport()->update();
}

void TerminalWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);

    if (!m_ptyProcess)
        setupPty();

    QAbstractScrollArea::showEvent(event);
}

bool TerminalWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        if (hasFocus()) {
            event->accept();
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *k = (QKeyEvent *) event;
        keyPressEvent(k);
        return true;
    }
    if (event->type() == QEvent::KeyRelease) {
        QKeyEvent *k = (QKeyEvent *) event;
        keyReleaseEvent(k);
        return true;
    }

    return QAbstractScrollArea::event(event);
}

int TerminalWidget::setTerminalProperties(VTermProp prop, VTermValue *val)
{
    switch (prop) {
    case VTERM_PROP_CURSORVISIBLE:
        m_cursor.visible = val->boolean;
        break;
    case VTERM_PROP_CURSORBLINK:
        qCDebug(terminalLog) << "Ignoring VTERM_PROP_CURSORBLINK" << val->boolean;
        break;
    case VTERM_PROP_CURSORSHAPE:
        qCDebug(terminalLog) << "Ignoring VTERM_PROP_CURSORSHAPE" << val->number;
        break;
    case VTERM_PROP_ICONNAME:
        //emit iconTextChanged(val->string);
        break;
    case VTERM_PROP_TITLE:
        //emit titleChanged(val->string);
        setWindowTitle(QString::fromUtf8(val->string.str, val->string.len));
        break;
    case VTERM_PROP_ALTSCREEN:
        m_altscreen = val->boolean;
        m_selection.reset();
        break;
    case VTERM_PROP_MOUSE:
        qCDebug(terminalLog) << "Ignoring VTERM_PROP_MOUSE" << val->number;
        break;
    case VTERM_PROP_REVERSE:
        qCDebug(terminalLog) << "Ignoring VTERM_PROP_REVERSE" << val->boolean;
        break;
    case VTERM_N_PROPS:
        break;
    }
    return 1;
}

int TerminalWidget::movecursor(VTermPos pos, VTermPos oldpos, int visible)
{
    Q_UNUSED(oldpos);
    viewport()->update();
    m_cursor.row = pos.row;
    m_cursor.col = pos.col;
    m_cursor.visible = visible;

    return 1;
}

} // namespace Terminal
