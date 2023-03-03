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
#include <utils/processinterface.h>
#include <utils/stringutils.h>

#include <vterm.h>

#include <QApplication>
#include <QClipboard>
#include <QElapsedTimer>
#include <QGlyphRun>
#include <QLoggingCategory>
#include <QPaintEvent>
#include <QPainter>
#include <QRawFont>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextLayout>

Q_LOGGING_CATEGORY(terminalLog, "qtc.terminal", QtWarningMsg)
Q_LOGGING_CATEGORY(selectionLog, "qtc.terminal.selection", QtWarningMsg)

using namespace Utils;
using namespace Utils::Terminal;

namespace Terminal {

using namespace std::chrono_literals;

// Minimum time between two refreshes. (30fps)
static constexpr std::chrono::milliseconds minRefreshInterval = 1s / 30;

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
    , m_lastFlush(std::chrono::system_clock::now())
    , m_lastDoubleClick(std::chrono::system_clock::now())
{
    setupVTerm();
    setupFont();
    setupColors();
    setupActions();

    setAttribute(Qt::WA_InputMethodEnabled);
    setAttribute(Qt::WA_MouseTracking);

    setCursor(Qt::IBeamCursor);

    setViewportMargins(1, 1, 1, 1);

    m_textLayout.setCacheEnabled(true);

    setFocus();
    setFocusPolicy(Qt::StrongFocus);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    m_flushDelayTimer.setSingleShot(true);
    m_flushDelayTimer.setInterval(minRefreshInterval);

    connect(&m_flushDelayTimer, &QTimer::timeout, this, [this]() { flushVTerm(true); });

    connect(&TerminalSettings::instance(), &AspectContainer::applied, this, [this] {
        m_layoutVersion++;
        // Setup colors first, as setupFont will redraw the screen.
        setupColors();
        setupFont();
    });
}

void TerminalWidget::setupPty()
{
    m_process = std::make_unique<QtcProcess>();

    Environment env = m_openParameters.environment.value_or(Environment::systemEnvironment());

    CommandLine shellCommand = m_openParameters.shellCommand.value_or(
        CommandLine{TerminalSettings::instance().shell.filePath(), {}});

    // For git bash on Windows
    env.prependOrSetPath(shellCommand.executable().parentDir());
    if (env.hasKey("CLINK_NOAUTORUN"))
        env.unset("CLINK_NOAUTORUN");

    m_process->setProcessMode(ProcessMode::Writer);
    m_process->setTerminalMode(TerminalMode::Pty);
    m_process->setCommand(shellCommand);
    if (m_openParameters.workingDirectory.has_value())
        m_process->setWorkingDirectory(*m_openParameters.workingDirectory);
    m_process->setEnvironment(env);

    connect(m_process.get(), &QtcProcess::readyReadStandardOutput, this, [this]() {
        onReadyRead(false);
    });

    connect(m_process.get(), &QtcProcess::done, this, [this] {
        m_cursor.visible = false;
        if (m_process) {
            if (m_process->exitCode() != 0) {
                QByteArray msg = QString("\r\n\033[31mProcess exited with code: %1")
                                     .arg(m_process->exitCode())
                                     .toUtf8();

                if (!m_process->errorString().isEmpty())
                    msg += QString(" (%1)").arg(m_process->errorString()).toUtf8();

                vterm_input_write(m_vterm.get(), msg.constData(), msg.size());
                vterm_screen_flush_damage(m_vtermScreen);

                return;
            }
        }

        if (m_openParameters.m_exitBehavior == ExitBehavior::Restart) {
            QMetaObject::invokeMethod(
                this,
                [this] {
                    m_process.reset();
                    setupPty();
                },
                Qt::QueuedConnection);
        }

        if (m_openParameters.m_exitBehavior == ExitBehavior::Close)
            deleteLater();

        if (m_openParameters.m_exitBehavior == ExitBehavior::Keep) {
            QByteArray msg = QString("\r\nProcess exited with code: %1")
                                 .arg(m_process ? m_process->exitCode() : -1)
                                 .toUtf8();

            vterm_input_write(m_vterm.get(), msg.constData(), msg.size());
            vterm_screen_flush_damage(m_vtermScreen);
        }
    });

    connect(m_process.get(), &QtcProcess::started, this, [this] {
        m_shellName = m_process->commandLine().executable().fileName();
        if (HostOsInfo::isWindowsHost() && m_shellName.endsWith(QTC_WIN_EXE_SUFFIX))
            m_shellName.chop(QStringLiteral(QTC_WIN_EXE_SUFFIX).size());

        applySizeChange();
        emit started(m_process->processId());
    });

    m_process->start();
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

void TerminalWidget::setupActions()
{
    m_copyAction.setEnabled(false);
    m_copyAction.setShortcuts(
        {QKeySequence(HostOsInfo::isMacHost() ? QLatin1String("Ctrl+C")
                                              : QLatin1String("Ctrl+Shift+C")),
         QKeySequence(Qt::Key_Return)});
    m_pasteAction.setShortcut(QKeySequence(
        HostOsInfo::isMacHost() ? QLatin1String("Ctrl+V") : QLatin1String("Ctrl+Shift+V")));

    m_clearSelectionAction.setShortcut(QKeySequence("Esc"));

    m_zoomInAction.setShortcuts({QKeySequence("Ctrl++"), QKeySequence("Ctrl+Shift++")});
    m_zoomOutAction.setShortcut(QKeySequence("Ctrl+-"));

    connect(&m_copyAction, &QAction::triggered, this, &TerminalWidget::copyToClipboard);
    connect(&m_pasteAction, &QAction::triggered, this, &TerminalWidget::pasteFromClipboard);
    connect(&m_clearSelectionAction, &QAction::triggered, this, &TerminalWidget::clearSelection);
    connect(&m_zoomInAction, &QAction::triggered, this, &TerminalWidget::zoomIn);
    connect(&m_zoomOutAction, &QAction::triggered, this, &TerminalWidget::zoomOut);

    addActions({&m_copyAction, &m_pasteAction, &m_clearSelectionAction, &m_zoomInAction, &m_zoomOutAction});
}

void TerminalWidget::writeToPty(const QByteArray &data)
{
    if (m_process)
        m_process->writeRaw(data);
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

    qCInfo(terminalLog) << font.family() << font.pointSize() << w << viewport()->size();

    m_cellSize = {w, qfm.height()};
    m_cellBaseline = qfm.ascent();
    m_lineSpacing = qfm.height();

    QAbstractScrollArea::setFont(m_font);

    if (m_process) {
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
    setSelection(std::nullopt);
    viewport()->update();
    vterm_keyboard_key(m_vterm.get(), VTERM_KEY_ESCAPE, VTERM_MOD_NONE);
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

    // Send Ctrl+L which will clear the screen
    writeToPty(QByteArray("\f"));
}

void TerminalWidget::onReadyRead(bool forceFlush)
{
    QByteArray data = m_process->readAllRawStandardOutput();
    vterm_input_write(m_vterm.get(), data.constData(), data.size());

    flushVTerm(forceFlush);
}

void TerminalWidget::flushVTerm(bool force)
{
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    const std::chrono::milliseconds timeSinceLastFlush
        = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFlush);

    const bool shouldFlushImmediately = timeSinceLastFlush > minRefreshInterval;
    if (force || shouldFlushImmediately) {
        if (m_flushDelayTimer.isActive())
            m_flushDelayTimer.stop();

        m_lastFlush = now;
        vterm_screen_flush_damage(m_vtermScreen);
        return;
    }

    if (!m_flushDelayTimer.isActive()) {
        const std::chrono::milliseconds timeToNextFlush = (minRefreshInterval - timeSinceLastFlush);
        m_flushDelayTimer.start(timeToNextFlush.count());
    }
}

void TerminalWidget::setSelection(const std::optional<Selection> &selection)
{
    if (selection.has_value() != m_selection.has_value()) {
        qCDebug(selectionLog) << "Copy enabled:" << selection.has_value();
        m_copyAction.setEnabled(selection.has_value());
    }
    m_selection = selection;
}

QString TerminalWidget::shellName() const
{
    return m_shellName;
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

QPoint TerminalWidget::globalToViewport(QPoint p) const
{
    int y = p.y() + topMargin();
    const double offset = (m_scrollback->size() - m_scrollback->offset()) * m_lineSpacing;
    y -= offset;

    return {p.x(), y};
}

QPoint TerminalWidget::globalToGrid(QPoint p) const
{
    return QPoint(p.x() / m_cellSize.width(), p.y() / m_cellSize.height());
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

    Internal::createTextLayout(m_textLayout,
                               &m_currentLiveText,
                               defaultBg,
                               QRect({0, 0}, m_vtermSize),
                               m_lineSpacing,
                               [this](int x, int y) { return fetchCell(x, y); });

    qCInfo(terminalLog) << "createTextLayout took:" << t.elapsed() << "ms";
}

qreal TerminalWidget::topMargin() const
{
    return (qreal) viewport()->size().height() - ((qreal) m_vtermSize.height() * m_lineSpacing);
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
                        const QRectF cursorRect
                            = QRectF{xCursor, yCursor + 1, br, m_lineSpacing - 2};
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
            QTextLine cursorLine = m_textLayout.lineAt(m_cursor.row);
            if (cursorLine.isValid()) {
                int pos = cursorLine.textStart() + m_cursor.col;
                QPointF displayPos = QPointF{cursorLine.cursorToX(pos), cursorLine.y() + y};

                p.fillRect(QRectF{displayPos.toPoint(), m_cellSize}, QColor::fromRgb(0, 0, 0));
                p.setPen(Qt::white);
                displayPos.setY(displayPos.y() + m_cellBaseline);
                p.drawText(displayPos, m_preEditString);
            }
        }
    }

    p.fillRect(QRectF{{0, 0}, QSizeF{(qreal) width(), margin}}, Internal::toQColor(defaultBg));

    if (selectionLog().isDebugEnabled() && m_selection) {
        const auto s = globalToViewport(m_selection->start);
        const auto e = globalToViewport(m_selection->end);

        p.setPen(QPen(Qt::green, 1, Qt::DashLine));
        p.drawLine(s.x(), 0, s.x(), height());
        p.drawLine(0, s.y(), width(), s.y());

        p.setPen(QPen(Qt::red, 1, Qt::DashLine));

        p.drawLine(e.x(), 0, e.x(), height());
        p.drawLine(0, e.y(), width(), e.y());
    }
}

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    bool actionTriggered = false;
    for (const auto &action : actions()) {
        if (!action->isEnabled())
            continue;

        for (const auto &shortcut : action->shortcuts()) {
            const auto result = shortcut.matches(QKeySequence(event->keyCombination()));
            if (result == QKeySequence::ExactMatch) {
                action->trigger();
                actionTriggered = true;
                break;
            }
        }

        if (actionTriggered)
            break;
    }

    if (actionTriggered) {
        setSelection(std::nullopt);
        viewport()->update();
        return;
    }

    event->accept();

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

        // Workaround for "ALT+SHIFT+/" (\ on german mac keyboards)
        if (mod == (VTERM_MOD_SHIFT | VTERM_MOD_ALT) && event->key() == Qt::Key_Slash) {
            mod = VTERM_MOD_NONE;
        }

        vterm_keyboard_unichar(m_vterm.get(),
                               event->text().toUcs4()[0],
                               static_cast<VTermModifier>(mod & ~VTERM_MOD_CTRL));

        setSelection(std::nullopt);
    } else if (mod != VTERM_MOD_NONE && event->key() == Qt::Key_C) {
        vterm_keyboard_unichar(m_vterm.get(), 'c', mod);
    }
}

void TerminalWidget::applySizeChange()
{
    m_vtermSize = {
        qFloor((qreal) (viewport()->size().width()) / (qreal) m_cellSize.width()),
        qFloor((qreal) (viewport()->size().height()) / m_lineSpacing),
    };

    if (m_vtermSize.height() <= 0)
        m_vtermSize.setHeight(1);

    if (m_vtermSize.width() <= 0)
        m_vtermSize.setWidth(1);

    if (m_process)
        m_process->ptyData().resize(m_vtermSize);

    vterm_set_size(m_vterm.get(), m_vtermSize.height(), m_vtermSize.width());
    flushVTerm(true);
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

    setSelection(std::nullopt);
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
    verticalScrollBar()->event(event);
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
    m_selectionStartPos = event->pos();

    if (event->button() == Qt::LeftButton) {
        if (std::chrono::system_clock::now() - m_lastDoubleClick < 500ms) {
            m_selectLineMode = true;
            m_selection->start.setX(0);
            m_selection->end.setX(viewport()->width());
        } else {
            m_selectLineMode = false;
            QPoint pos = viewportToGlobal(event->pos());
            setSelection(Selection{pos, pos});
        }
        event->accept();
        viewport()->update();
    } else if (event->button() == Qt::RightButton) {
        if (m_selection) {
            m_copyAction.trigger();
            setSelection(std::nullopt);
            viewport()->update();
        } else {
            m_pasteAction.trigger();
        }
    }
}
void TerminalWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selection && event->buttons() & Qt::LeftButton) {
        const std::array<QPoint, 2> oldGrid = {globalToGrid(m_selection->start),
                                               globalToGrid(m_selection->end)};

        QPoint start = viewportToGlobal(m_selectionStartPos);
        QPoint newEnd = viewportToGlobal(event->pos());

        const auto startInGrid = globalToGrid(start);
        const auto endInGrid = globalToGrid(newEnd);

        if (startInGrid.y() > endInGrid.y())
            std::swap(start, newEnd);
        else if (startInGrid.y() == endInGrid.y()) {
            if (startInGrid.x() > endInGrid.x()) {
                const auto s = start.x();
                start.setX(newEnd.x());
                newEnd.setX(s);
            }
        }

        if (start.y() > newEnd.y()) {
            const auto s = start.y();
            start.setY(newEnd.y());
            newEnd.setY(s);
        }

        if (m_selectLineMode) {
            start.setX(0);
            newEnd.setX(viewport()->width());
        }

        m_selection->start = start;
        m_selection->end = newEnd;

        const std::array<QPoint, 2> newGrid = {globalToGrid(m_selection->start),
                                               globalToGrid(m_selection->end)};

        if (newGrid != oldGrid || selectionLog().isDebugEnabled())
            viewport()->update();
    }
}

void TerminalWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_selection && event->button() == Qt::LeftButton) {
        if ((m_selection->end - m_selection->start).manhattanLength() < 2) {
            setSelection(std::nullopt);
            viewport()->update();
        }
    }
}

void TerminalWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    std::u32string text = m_scrollback->currentText() + m_currentLiveText;

    const QPoint clickPos = viewportToGlobal(event->pos());
    const QPoint clickPosInGrid = globalToGrid(clickPos);

    std::u32string::size_type chIdx = (clickPosInGrid.x())
                                      + (clickPosInGrid.y()) * m_vtermSize.width();

    if (chIdx >= text.length())
        return;

    std::u32string whiteSpaces = U" \t\x00a0";

    const bool inverted = whiteSpaces.find(text[chIdx]) != std::u32string::npos;

    const std::u32string::size_type leftEnd = inverted
                                                  ? text.find_last_not_of(whiteSpaces, chIdx) + 1
                                                  : text.find_last_of(whiteSpaces, chIdx) + 1;
    std::u32string::size_type rightEnd = inverted ? text.find_first_not_of(whiteSpaces, chIdx)
                                                  : text.find_first_of(whiteSpaces, chIdx);
    if (rightEnd == std::u32string::npos)
        rightEnd = text.length();

    const auto found = text.substr(leftEnd, rightEnd - leftEnd);

    const QPoint selectionStart((leftEnd % m_vtermSize.width()) * m_cellSize.width()
                                    + (m_cellSize.width() / 4),
                                (leftEnd / m_vtermSize.width()) * m_cellSize.height()
                                    + (m_cellSize.height() / 4));
    const QPoint selectionEnd((rightEnd % m_vtermSize.width()) * m_cellSize.width(),
                              (rightEnd / m_vtermSize.width()) * m_cellSize.height()
                                  + m_cellSize.height());

    setSelection(Selection{selectionStart, selectionEnd});

    m_lastDoubleClick = std::chrono::system_clock::now();

    viewport()->update();
    event->accept();
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

    if (!m_process)
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

    if (event->type() == QEvent::Paint) {
        QPainter p(this);
        p.fillRect(QRect(QPoint(0, 0), size()),
                   TerminalSettings::instance().backgroundColor.value());
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
        setSelection(std::nullopt);
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
