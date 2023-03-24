// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalwidget.h"
#include "glyphcache.h"
#include "terminalcommands.h"
#include "terminalsettings.h"
#include "terminalsurface.h"

#include <aggregation/aggregate.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/stringutils.h>

#include <vterm.h>

#include <QApplication>
#include <QCache>
#include <QClipboard>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QGlyphRun>
#include <QLoggingCategory>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QRawFont>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextItem>
#include <QTextLayout>
#include <QToolTip>

Q_LOGGING_CATEGORY(terminalLog, "qtc.terminal", QtWarningMsg)
Q_LOGGING_CATEGORY(selectionLog, "qtc.terminal.selection", QtWarningMsg)
Q_LOGGING_CATEGORY(paintLog, "qtc.terminal.paint", QtWarningMsg)

using namespace Utils;
using namespace Utils::Terminal;

namespace Terminal {

namespace ColorIndex {
enum Indices {
    Foreground = Internal::ColorIndex::Foreground,
    Background = Internal::ColorIndex::Background,
    Selection,
    FindMatch,
};
}

using namespace std::chrono_literals;

// Minimum time between two refreshes. (30fps)
static constexpr std::chrono::milliseconds minRefreshInterval = 1s / 30;

TerminalWidget::TerminalWidget(QWidget *parent, const OpenTerminalParameters &openParameters)
    : QAbstractScrollArea(parent)
    , m_openParameters(openParameters)
    , m_lastFlush(std::chrono::system_clock::now())
    , m_lastDoubleClick(std::chrono::system_clock::now())
{
    setupSurface();
    setupFont();
    setupColors();
    setupActions();

    m_cursorBlinkTimer.setInterval(750);
    m_cursorBlinkTimer.setSingleShot(false);

    connect(&m_cursorBlinkTimer, &QTimer::timeout, this, [this]() {
        if (hasFocus())
            m_cursorBlinkState = !m_cursorBlinkState;
        else
            m_cursorBlinkState = true;
        updateViewportRect(gridToViewport(QRect{m_cursor.position, m_cursor.position}));
    });

    setAttribute(Qt::WA_InputMethodEnabled);
    setAttribute(Qt::WA_MouseTracking);

    setCursor(Qt::IBeamCursor);

    setViewportMargins(1, 1, 1, 1);

    setFocus();
    setFocusPolicy(Qt::StrongFocus);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    m_flushDelayTimer.setSingleShot(true);
    m_flushDelayTimer.setInterval(minRefreshInterval);

    connect(&m_flushDelayTimer, &QTimer::timeout, this, [this]() { flushVTerm(true); });

    m_scrollTimer.setSingleShot(false);
    m_scrollTimer.setInterval(1s / 2);
    connect(&m_scrollTimer, &QTimer::timeout, this, [this] {
        if (m_scrollDirection < 0)
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
        else if (m_scrollDirection > 0)
            verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
    });

    connect(&TerminalSettings::instance(), &AspectContainer::applied, this, [this] {
        // Setup colors first, as setupFont will redraw the screen.
        setupColors();
        setupFont();
        configBlinkTimer();
    });

    m_aggregate = new Aggregation::Aggregate(this);
    m_aggregate->add(this);
    m_aggregate->add(m_search.get());
}

TerminalWidget::~TerminalWidget()
{
    // The Aggregate stuff tries to do clever deletion of the children, but we
    // we don't want that.
    m_aggregate->remove(this);
    m_aggregate->remove(m_search.get());
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
    m_process->setPtyData(Utils::Pty::Data());
    m_process->setCommand(shellCommand);
    if (m_openParameters.workingDirectory.has_value())
        m_process->setWorkingDirectory(*m_openParameters.workingDirectory);
    m_process->setEnvironment(env);

    if (m_surface->shellIntegration()) {
        m_surface->shellIntegration()->prepareProcess(*m_process.get());
    }

    connect(m_process.get(), &QtcProcess::readyReadStandardOutput, this, [this]() {
        onReadyRead(false);
    });

    connect(m_process.get(), &QtcProcess::done, this, [this] {
        if (m_process) {
            if (m_process->exitCode() != 0) {
                QByteArray msg = QString("\r\n\033[31mProcess exited with code: %1")
                                     .arg(m_process->exitCode())
                                     .toUtf8();

                if (!m_process->errorString().isEmpty())
                    msg += QString(" (%1)").arg(m_process->errorString()).toUtf8();

                m_surface->dataFromPty(msg);

                return;
            }
        }

        if (m_openParameters.m_exitBehavior == ExitBehavior::Restart) {
            QMetaObject::invokeMethod(
                this,
                [this] {
                    m_process.reset();
                    setupSurface();
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

            m_surface->dataFromPty(msg);
        }
    });

    connect(m_process.get(), &QtcProcess::started, this, [this] {
        if (m_shellName.isEmpty())
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
    std::array<QColor, 20> newColors;
    for (int i = 0; i < 16; ++i) {
        newColors[i] = TerminalSettings::instance().colors[i].value();
    }
    newColors[ColorIndex::Background] = TerminalSettings::instance().backgroundColor.value();
    newColors[ColorIndex::Foreground] = TerminalSettings::instance().foregroundColor.value();
    newColors[ColorIndex::Selection] = TerminalSettings::instance().selectionColor.value();
    newColors[ColorIndex::FindMatch] = TerminalSettings::instance().findMatchColor.value();

    if (m_currentColors == newColors)
        return;

    m_currentColors = newColors;

    updateViewport();
    update();
}

void TerminalWidget::setupActions()
{
    WidgetActions &a = TerminalCommands::widgetActions();

    auto ifHasFocus = [this](void (TerminalWidget::*f)()) {
        return [this, f] {
            if (hasFocus())
                (this->*f)();
        };
    };

    // clang-format off
    connect(&a.copy, &QAction::triggered, this, ifHasFocus(&TerminalWidget::copyToClipboard));
    connect(&a.paste, &QAction::triggered, this, ifHasFocus(&TerminalWidget::pasteFromClipboard));
    connect(&a.clearSelection, &QAction::triggered, this, ifHasFocus(&TerminalWidget::clearSelection));
    connect(&a.clearTerminal, &QAction::triggered, this, ifHasFocus(&TerminalWidget::clearContents));
    connect(&a.moveCursorWordLeft, &QAction::triggered, this, ifHasFocus(&TerminalWidget::moveCursorWordLeft));
    connect(&a.moveCursorWordRight, &QAction::triggered, this, ifHasFocus(&TerminalWidget::moveCursorWordRight));
    // clang-format on
}

void TerminalWidget::writeToPty(const QByteArray &data)
{
    if (m_process && m_process->isRunning())
        m_process->writeRaw(data);
}

void TerminalWidget::setupSurface()
{
    m_shellIntegration.reset(new ShellIntegration());
    m_surface = std::make_unique<Internal::TerminalSurface>(QSize{80, 60}, m_shellIntegration.get());
    m_search = std::make_unique<TerminalSearch>(m_surface.get());

    connect(m_search.get(), &TerminalSearch::hitsChanged, this, &TerminalWidget::updateViewport);
    connect(m_search.get(), &TerminalSearch::currentHitChanged, this, [this] {
        SearchHit hit = m_search->currentHit();
        if (hit.start >= 0) {
            setSelection(Selection{hit.start, hit.end, true}, hit != m_lastSelectedHit);
            m_lastSelectedHit = hit;
        }
    });

    connect(m_surface.get(),
            &Internal::TerminalSurface::writeToPty,
            this,
            &TerminalWidget::writeToPty);
    connect(m_surface.get(), &Internal::TerminalSurface::fullSizeChanged, this, [this] {
        updateScrollBars();
    });
    connect(m_surface.get(),
            &Internal::TerminalSurface::invalidated,
            this,
            [this](const QRect &rect) {
                setSelection(std::nullopt);
                updateViewportRect(gridToViewport(rect));
                verticalScrollBar()->setValue(m_surface->fullSize().height());
            });
    connect(m_surface.get(),
            &Internal::TerminalSurface::cursorChanged,
            this,
            [this](const Internal::Cursor &oldCursor, const Internal::Cursor &newCursor) {
                int startX = oldCursor.position.x();
                int endX = newCursor.position.x();

                if (startX > endX)
                    std::swap(startX, endX);

                int startY = oldCursor.position.y();
                int endY = newCursor.position.y();
                if (startY > endY)
                    std::swap(startY, endY);

                m_cursor = newCursor;

                updateViewportRect(
                    gridToViewport(QRect{QPoint{startX, startY}, QPoint{endX, endY}}));
                configBlinkTimer();
            });
    connect(m_surface.get(), &Internal::TerminalSurface::altscreenChanged, this, [this] {
        updateScrollBars();
        if (!setSelection(std::nullopt))
            updateViewport();
    });
    connect(m_surface.get(), &Internal::TerminalSurface::unscroll, this, [this] {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    });
    connect(m_surface.get(), &Internal::TerminalSurface::bell, this, [] { QApplication::beep(); });
    if (m_shellIntegration) {
        connect(m_shellIntegration.get(),
                &ShellIntegration::commandChanged,
                this,
                [this](const CommandLine &command) {
                    m_currentCommand = command;
                    emit commandChanged(m_currentCommand);
                });
        connect(m_shellIntegration.get(),
                &ShellIntegration::currentDirChanged,
                this,
                [this](const QString &currentDir) {
                    m_cwd = FilePath::fromUserInput(currentDir);
                    emit cwdChanged(m_cwd);
                });
    }
}

void TerminalWidget::configBlinkTimer()
{
    bool shouldRun = m_cursor.visible && m_cursor.blink && hasFocus()
                     && TerminalSettings::instance().allowBlinkingCursor.value();
    if (shouldRun != m_cursorBlinkTimer.isActive()) {
        if (shouldRun)
            m_cursorBlinkTimer.start();
        else
            m_cursorBlinkTimer.stop();
    }
}

QColor TerminalWidget::toQColor(std::variant<int, QColor> color) const
{
    if (std::holds_alternative<int>(color)) {
        int idx = std::get<int>(color);
        if (idx >= 0 && idx < 18)
            return m_currentColors[idx];

        return m_currentColors[ColorIndex::Background];
    }
    return std::get<QColor>(color);
}

void TerminalWidget::updateCopyState()
{
    if (!hasFocus())
        return;

    TerminalCommands::widgetActions().copy.setEnabled(m_selection.has_value());
}

void TerminalWidget::setFont(const QFont &font)
{
    m_font = font;

    QFontMetricsF qfm{m_font};
    const qreal w = [qfm]() -> qreal {
        if (HostOsInfo::isMacHost())
            return qfm.maxWidth();
        return qfm.averageCharWidth();
    }();

    qCInfo(terminalLog) << font.family() << font.pointSize() << w << viewport()->size();

    m_cellSize = {w, (double) qCeil(qfm.height())};

    QAbstractScrollArea::setFont(m_font);

    if (m_process) {
        applySizeChange();
    }
}

void TerminalWidget::copyToClipboard()
{
    QTC_ASSERT(m_selection.has_value(), return);

    QString text = textFromSelection();

    qCDebug(selectionLog) << "Copied to clipboard: " << text;

    setClipboardAndSelection(text);
}

void TerminalWidget::pasteFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    const QString clipboardText = clipboard->text(QClipboard::Clipboard);

    if (clipboardText.isEmpty())
        return;

    m_surface->pasteFromClipboard(clipboardText);
}

void TerminalWidget::clearSelection()
{
    setSelection(std::nullopt);
    m_surface->sendKey(Qt::Key_Escape);
}

void TerminalWidget::zoomIn()
{
    m_font.setPointSize(m_font.pointSize() + 1);
    setFont(m_font);
}

void TerminalWidget::zoomOut()
{
    m_font.setPointSize(qMax(m_font.pointSize() - 1, 1));
    setFont(m_font);
}

void TerminalWidget::moveCursorWordLeft()
{
    writeToPty("\x1b\x62");
}

void TerminalWidget::moveCursorWordRight()
{
    writeToPty("\x1b\x66");
}

void TerminalWidget::clearContents()
{
    m_surface->clearAll();
}

void TerminalWidget::onReadyRead(bool forceFlush)
{
    QByteArray data = m_process->readAllRawStandardOutput();

    m_surface->dataFromPty(data);

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
        m_surface->flush();
        return;
    }

    if (!m_flushDelayTimer.isActive()) {
        const std::chrono::milliseconds timeToNextFlush = (minRefreshInterval - timeSinceLastFlush);
        m_flushDelayTimer.start(timeToNextFlush.count());
    }
}

QString TerminalWidget::textFromSelection() const
{
    if (!m_selection)
        return {};

    Internal::CellIterator it = m_surface->iteratorAt(m_selection->start);
    Internal::CellIterator end = m_surface->iteratorAt(m_selection->end);

    std::u32string s;
    bool previousWasZero = false;
    for (; it != end; ++it) {
        if (it.gridPos().x() == 0 && !s.empty() && previousWasZero)
            s += U'\n';

        if (*it != 0) {
            previousWasZero = false;
            s += *it;
        } else {
            previousWasZero = true;
        }
    }

    return QString::fromUcs4(s.data(), static_cast<int>(s.size()));
}

bool TerminalWidget::setSelection(const std::optional<Selection> &selection, bool scroll)
{
    qCDebug(selectionLog) << "setSelection" << selection.has_value();
    if (selection.has_value())
        qCDebug(selectionLog) << "start:" << selection->start << "end:" << selection->end
                              << "final:" << selection->final;

    if (selectionLog().isDebugEnabled())
        updateViewport();

    if (selection == m_selection)
        return false;

    m_selection = selection;

    updateCopyState();

    if (m_selection && m_selection->final) {
        qCDebug(selectionLog) << "Copy enabled:" << selection.has_value();
        QString text = textFromSelection();

        QClipboard *clipboard = QApplication::clipboard();
        if (clipboard->supportsSelection()) {
            qCDebug(selectionLog) << "Selection set to clipboard: " << text;
            clipboard->setText(text, QClipboard::Selection);
        }

        if (scroll) {
            QPoint start = m_surface->posToGrid(m_selection->start);
            QPoint end = m_surface->posToGrid(m_selection->end);
            QRect viewRect = gridToViewport(QRect{start, end});
            if (viewRect.y() >= viewport()->height() || viewRect.y() < 0) {
                // Selection is outside of the viewport, scroll to it.
                verticalScrollBar()->setValue(start.y());
            }
        }

        m_search->setCurrentSelection(SearchHitWithText{{selection->start, selection->end}, text});
    }

    if (!selectionLog().isDebugEnabled())
        updateViewport();

    return true;
}

void TerminalWidget::setShellName(const QString &shellName)
{
    m_shellName = shellName;
}

QString TerminalWidget::shellName() const
{
    return m_shellName;
}

FilePath TerminalWidget::cwd() const
{
    return m_cwd;
}

CommandLine TerminalWidget::currentCommand() const
{
    return m_currentCommand;
}

std::optional<Id> TerminalWidget::identifier() const
{
    return m_openParameters.identifier;
}

QProcess::ProcessState TerminalWidget::processState() const
{
    if (m_process)
        return m_process->state();

    return QProcess::NotRunning;
}

void TerminalWidget::restart(const OpenTerminalParameters &openParameters)
{
    QTC_ASSERT(!m_process || !m_process->isRunning(), return);
    m_openParameters = openParameters;

    m_process.reset();
    setupSurface();
    setupPty();
}

QPoint TerminalWidget::viewportToGlobal(QPoint p) const
{
    int y = p.y() - topMargin();
    const double offset = verticalScrollBar()->value() * m_cellSize.height();
    y += offset;

    return {p.x(), y};
}

QPoint TerminalWidget::globalToViewport(QPoint p) const
{
    int y = p.y() + topMargin();
    const double offset = verticalScrollBar()->value() * m_cellSize.height();
    y -= offset;

    return {p.x(), y};
}

QPoint TerminalWidget::globalToGrid(QPointF p) const
{
    return QPoint(p.x() / m_cellSize.width(), p.y() / m_cellSize.height());
}

QPointF TerminalWidget::gridToGlobal(QPoint p, bool bottom, bool right) const
{
    QPointF result = QPointF(p.x() * m_cellSize.width(), p.y() * m_cellSize.height());
    if (bottom || right)
        result += {right ? m_cellSize.width() : 0, bottom ? m_cellSize.height() : 0};
    return result;
}

qreal TerminalWidget::topMargin() const
{
    return viewport()->size().height() - (m_surface->liveSize().height() * m_cellSize.height());
}

static QPixmap generateWavyPixmap(qreal maxRadius, const QPen &pen)
{
    const qreal radiusBase = qMax(qreal(1), maxRadius);
    const qreal pWidth = pen.widthF();

    QString key = QLatin1String("WaveUnderline-") % pen.color().name()
                  % QString::number(*(size_t *) &radiusBase, 16)
                  % QString::number(*(size_t *) &pWidth);

    QPixmap pixmap;
    if (QPixmapCache::find(key, &pixmap))
        return pixmap;

    const qreal halfPeriod = qMax(qreal(2), qreal(radiusBase * 1.61803399)); // the golden ratio
    const int width = qCeil(100 / (2 * halfPeriod)) * (2 * halfPeriod);
    const qreal radius = qFloor(radiusBase * 2) / 2.;

    QPainterPath path;

    qreal xs = 0;
    qreal ys = radius;

    while (xs < width) {
        xs += halfPeriod;
        ys = -ys;
        path.quadTo(xs - halfPeriod / 2, ys, xs, 0);
    }

    pixmap = QPixmap(width, radius * 2);
    pixmap.fill(Qt::transparent);
    {
        QPen wavePen = pen;
        wavePen.setCapStyle(Qt::SquareCap);

        // This is to protect against making the line too fat, as happens on macOS
        // due to it having a rather thick width for the regular underline.
        const qreal maxPenWidth = .8 * radius;
        if (wavePen.widthF() > maxPenWidth)
            wavePen.setWidthF(maxPenWidth);

        QPainter imgPainter(&pixmap);
        imgPainter.setPen(wavePen);
        imgPainter.setRenderHint(QPainter::Antialiasing);
        imgPainter.translate(0, radius);
        imgPainter.drawPath(path);
    }

    QPixmapCache::insert(key, pixmap);

    return pixmap;
}

// Copied from qpainter.cpp
static void drawTextItemDecoration(QPainter &painter,
                                   const QPointF &pos,
                                   QTextCharFormat::UnderlineStyle underlineStyle,
                                   QTextItem::RenderFlags flags,
                                   qreal width,
                                   const QColor &underlineColor,
                                   const QRawFont &font)
{
    if (underlineStyle == QTextCharFormat::NoUnderline
        && !(flags & (QTextItem::StrikeOut | QTextItem::Overline)))
        return;

    const QPen oldPen = painter.pen();
    const QBrush oldBrush = painter.brush();
    painter.setBrush(Qt::NoBrush);
    QPen pen = oldPen;
    pen.setStyle(Qt::SolidLine);
    pen.setWidthF(font.lineThickness());
    pen.setCapStyle(Qt::FlatCap);

    QLineF line(qFloor(pos.x()), pos.y(), qFloor(pos.x() + width), pos.y());

    const qreal underlineOffset = font.underlinePosition();

    /*if (underlineStyle == QTextCharFormat::SpellCheckUnderline) {
        QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
        if (theme)
            underlineStyle = QTextCharFormat::UnderlineStyle(
                theme->themeHint(QPlatformTheme::SpellCheckUnderlineStyle).toInt());
        if (underlineStyle == QTextCharFormat::SpellCheckUnderline) // still not resolved
            underlineStyle = QTextCharFormat::WaveUnderline;
    }*/

    if (underlineStyle == QTextCharFormat::WaveUnderline) {
        painter.save();
        painter.translate(0, pos.y() + 1);
        qreal maxHeight = font.descent() - qreal(1);

        QColor uc = underlineColor;
        if (uc.isValid())
            pen.setColor(uc);

        // Adapt wave to underlineOffset or pen width, whatever is larger, to make it work on all platforms
        const QPixmap wave = generateWavyPixmap(qMin(qMax(underlineOffset, pen.widthF()),
                                                     maxHeight / qreal(2.)),
                                                pen);
        const int descent = qFloor(maxHeight);

        painter.setBrushOrigin(painter.brushOrigin().x(), 0);
        painter.fillRect(pos.x(), 0, qCeil(width), qMin(wave.height(), descent), wave);
        painter.restore();
    } else if (underlineStyle != QTextCharFormat::NoUnderline) {
        // Deliberately ceil the offset to avoid the underline coming too close to
        // the text above it, but limit it to stay within descent.
        qreal adjustedUnderlineOffset = std::ceil(underlineOffset) + 0.5;
        if (underlineOffset <= font.descent())
            adjustedUnderlineOffset = qMin(adjustedUnderlineOffset, font.descent() - qreal(0.5));
        const qreal underlinePos = pos.y() + adjustedUnderlineOffset;
        QColor uc = underlineColor;
        if (uc.isValid())
            pen.setColor(uc);

        pen.setStyle((Qt::PenStyle)(underlineStyle));
        painter.setPen(pen);
        QLineF underline(line.x1(), underlinePos, line.x2(), underlinePos);
        painter.drawLine(underline);
    }

    pen.setStyle(Qt::SolidLine);
    pen.setColor(oldPen.color());

    if (flags & QTextItem::StrikeOut) {
        QLineF strikeOutLine = line;
        strikeOutLine.translate(0., -font.ascent() / 3.);
        QColor uc = underlineColor;
        if (uc.isValid())
            pen.setColor(uc);
        painter.setPen(pen);
        painter.drawLine(strikeOutLine);
    }

    if (flags & QTextItem::Overline) {
        QLineF overline = line;
        overline.translate(0., -font.ascent());
        QColor uc = underlineColor;
        if (uc.isValid())
            pen.setColor(uc);
        painter.setPen(pen);
        painter.drawLine(overline);
    }

    painter.setPen(oldPen);
    painter.setBrush(oldBrush);
}

bool TerminalWidget::paintFindMatches(QPainter &p,
                                      QList<SearchHit>::const_iterator &it,
                                      const QRectF &cellRect,
                                      const QPoint gridPos) const
{
    if (it == m_search->hits().constEnd())
        return false;

    const int pos = m_surface->gridToPos(gridPos);
    while (it != m_search->hits().constEnd()) {
        if (pos < it->start)
            return false;

        if (pos >= it->end) {
            ++it;
            continue;
        }
        break;
    }

    if (it == m_search->hits().constEnd())
        return false;

    p.fillRect(cellRect, m_currentColors[ColorIndex::FindMatch]);

    return true;
}

bool TerminalWidget::paintSelection(QPainter &p, const QRectF &cellRect, const QPoint gridPos) const
{
    bool isInSelection = false;
    const int pos = m_surface->gridToPos(gridPos);

    if (m_selection)
        isInSelection = pos >= m_selection->start && pos < m_selection->end;

    if (isInSelection)
        p.fillRect(cellRect, m_currentColors[ColorIndex::Selection]);

    return isInSelection;
}

int TerminalWidget::paintCell(QPainter &p,
                              const QRectF &cellRect,
                              QPoint gridPos,
                              const Internal::TerminalCell &cell,
                              QFont &f,
                              QList<SearchHit>::const_iterator &searchIt) const
{
    bool paintBackground = !paintSelection(p, cellRect, gridPos)
                           && !paintFindMatches(p, searchIt, cellRect, gridPos);

    bool isDefaultBg = std::holds_alternative<int>(cell.backgroundColor)
                       && std::get<int>(cell.backgroundColor) == 17;

    if (paintBackground && !isDefaultBg)
        p.fillRect(cellRect, toQColor(cell.backgroundColor));

    p.setPen(toQColor(cell.foregroundColor));

    f.setBold(cell.bold);
    f.setItalic(cell.italic);

    if (!cell.text.isEmpty()) {
        const auto r = Internal::GlyphCache::instance().get(f, cell.text);

        if (r) {
            const auto brSize = r->boundingRect().size();
            QPointF brOffset;
            if (brSize.width() > cellRect.size().width())
                brOffset.setX(-(brSize.width() - cellRect.size().width()) / 2.0);
            if (brSize.height() > cellRect.size().height())
                brOffset.setY(-(brSize.height() - cellRect.size().height()) / 2.0);

            QPointF finalPos = cellRect.topLeft() + brOffset;

            p.drawGlyphRun(finalPos, *r);

            bool tempLink = false;
            if (m_linkSelection) {
                int chPos = m_surface->gridToPos(gridPos);
                tempLink = chPos >= m_linkSelection->start && chPos < m_linkSelection->end;
            }
            if (cell.underlineStyle != QTextCharFormat::NoUnderline || cell.strikeOut || tempLink) {
                QTextItem::RenderFlags flags;
                //flags.setFlag(QTextItem::RenderFlag::Underline, cell.format.fontUnderline());
                flags.setFlag(QTextItem::StrikeOut, cell.strikeOut);
                finalPos.setY(finalPos.y() + r->rawFont().ascent());
                drawTextItemDecoration(p,
                                       finalPos,
                                       tempLink ? QTextCharFormat::DashUnderline
                                                : cell.underlineStyle,
                                       flags,
                                       cellRect.size().width(),
                                       {},
                                       r->rawFont());
            }
        }
    }

    return cell.width;
}

void TerminalWidget::paintCursor(QPainter &p) const
{
    if (!m_process || !m_process->isRunning())
        return;

    auto cursor = m_surface->cursor();

    const bool blinkState = !cursor.blink || m_cursorBlinkState
                            || !TerminalSettings::instance().allowBlinkingCursor.value();

    if (cursor.visible && blinkState) {
        const int cursorCellWidth = m_surface->cellWidthAt(cursor.position.x(), cursor.position.y());

        QRectF cursorRect = QRectF(gridToGlobal(cursor.position),
                                   gridToGlobal({cursor.position.x() + cursorCellWidth,
                                                 cursor.position.y()},
                                                true));

        cursorRect.adjust(0, 0, 0, -1);

        if (hasFocus()) {
            QPainter::CompositionMode oldMode = p.compositionMode();
            p.setCompositionMode(QPainter::RasterOp_NotDestination);
            switch (cursor.shape) {
            case Internal::Cursor::Shape::Block:
                p.fillRect(cursorRect, p.pen().brush());
                break;
            case Internal::Cursor::Shape::Underline:
                p.drawLine(cursorRect.bottomLeft(), cursorRect.bottomRight());
                break;
            case Internal::Cursor::Shape::LeftBar:
                p.drawLine(cursorRect.topLeft(), cursorRect.bottomLeft());
                break;
            }
            p.setCompositionMode(oldMode);
        } else {
            p.drawRect(cursorRect);
        }
    }
}

void TerminalWidget::paintPreedit(QPainter &p) const
{
    auto cursor = m_surface->cursor();
    if (!m_preEditString.isEmpty()) {
        QRectF rect = QRectF(gridToGlobal(cursor.position),
                             gridToGlobal({cursor.position.x(), cursor.position.y()}, true, true));

        p.fillRect(rect, QColor::fromRgb(0, 0, 0));
        p.setPen(Qt::white);
        p.drawText(rect, m_preEditString);
    }
}

void TerminalWidget::paintCells(QPainter &p, QPaintEvent *event) const
{
    QFont f = m_font;

    const int scrollOffset = verticalScrollBar()->value();

    const int maxRow = m_surface->fullSize().height();
    const int startRow = qFloor((qreal) event->rect().y() / m_cellSize.height()) + scrollOffset;
    const int endRow = qMin(maxRow,
                            qCeil((event->rect().y() + event->rect().height()) / m_cellSize.height())
                                + scrollOffset);

    QList<SearchHit>::const_iterator searchIt
        = std::lower_bound(m_search->hits().constBegin(),
                           m_search->hits().constEnd(),
                           startRow,
                           [this](const SearchHit &hit, int value) {
                               return m_surface->posToGrid(hit.start).y() < value;
                           });

    for (int cellY = startRow; cellY < endRow; ++cellY) {
        for (int cellX = 0; cellX < m_surface->liveSize().width();) {
            const auto cell = m_surface->fetchCell(cellX, cellY);

            QRectF cellRect(gridToGlobal({cellX, cellY}),
                            QSizeF{m_cellSize.width() * cell.width, m_cellSize.height()});

            int numCells = paintCell(p, cellRect, {cellX, cellY}, cell, f, searchIt);

            cellX += numCells;
        }
    }
}

void TerminalWidget::paintDebugSelection(QPainter &p, const Selection &selection) const
{
    auto s = globalToViewport(gridToGlobal(m_surface->posToGrid(selection.start)).toPoint());
    const auto e = globalToViewport(
        gridToGlobal(m_surface->posToGrid(selection.end), true).toPoint());

    p.setPen(QPen(Qt::green, 1, Qt::DashLine));
    p.drawLine(s.x(), 0, s.x(), height());
    p.drawLine(0, s.y(), width(), s.y());

    p.setPen(QPen(Qt::red, 1, Qt::DashLine));

    p.drawLine(e.x(), 0, e.x(), height());
    p.drawLine(0, e.y(), width(), e.y());
}

void TerminalWidget::paintEvent(QPaintEvent *event)
{
    QElapsedTimer t;
    t.start();
    event->accept();
    QPainter p(viewport());

    p.save();

    if (paintLog().isDebugEnabled())
        p.fillRect(event->rect(), QColor::fromRgb(rand() % 60, rand() % 60, rand() % 60));
    else
        p.fillRect(event->rect(), m_currentColors[ColorIndex::Background]);

    int scrollOffset = verticalScrollBar()->value();
    int offset = -(scrollOffset * m_cellSize.height());

    qreal margin = topMargin();

    p.translate(QPointF{0.0, offset + margin});

    paintCells(p, event);
    paintCursor(p);
    paintPreedit(p);

    p.restore();

    p.fillRect(QRectF{{0, 0}, QSizeF{(qreal) width(), topMargin()}},
               m_currentColors[ColorIndex::Background]);

    if (selectionLog().isDebugEnabled()) {
        if (m_selection)
            paintDebugSelection(p, *m_selection);
        if (m_linkSelection)
            paintDebugSelection(p, *m_linkSelection);
    }

    if (paintLog().isDebugEnabled()) {
        QToolTip::showText(this->mapToGlobal(QPoint(width() - 200, 0)),
                           QString("Paint: %1ms").arg(t.elapsed()));
    }
}

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    // Don't blink during typing
    if (m_cursorBlinkTimer.isActive()) {
        m_cursorBlinkTimer.start();
        m_cursorBlinkState = true;
    }

    if (event->key() == Qt::Key_Escape) {
        bool sendToTerminal = TerminalSettings::instance().sendEscapeToTerminal.value();
        bool send = false;
        if (sendToTerminal && event->modifiers() == Qt::NoModifier)
            send = true;
        else if (!sendToTerminal && event->modifiers() == Qt::ShiftModifier)
            send = true;

        if (send) {
            event->setModifiers(Qt::NoModifier);
            m_surface->sendKey(event);
            return;
        }

        if (m_selection)
            TerminalCommands::widgetActions().clearSelection.trigger();
        else {
            QTC_ASSERT(Core::ActionManager::command(Core::Constants::S_RETURNTOEDITOR), return);
            Core::ActionManager::command(Core::Constants::S_RETURNTOEDITOR)->action()->trigger();
        }
        return;
    }

    auto oldSelection = m_selection;
    if (TerminalCommands::triggerAction(event)) {
        if (oldSelection && oldSelection == m_selection)
            setSelection(std::nullopt);
        return;
    }

    event->accept();

    m_surface->sendKey(event);
}

void TerminalWidget::applySizeChange()
{
    QSize newLiveSize = {
        qFloor((qreal) (viewport()->size().width()) / (qreal) m_cellSize.width()),
        qFloor((qreal) (viewport()->size().height()) / m_cellSize.height()),
    };

    if (newLiveSize.height() <= 0)
        newLiveSize.setHeight(1);

    if (newLiveSize.width() <= 0)
        newLiveSize.setWidth(1);

    if (m_process && m_process->ptyData())
        m_process->ptyData()->resize(newLiveSize);

    m_surface->resize(newLiveSize);
    flushVTerm(true);
}

void TerminalWidget::updateScrollBars()
{
    int scrollSize = m_surface->fullSize().height() - m_surface->liveSize().height();
    verticalScrollBar()->setRange(0, scrollSize);
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    updateViewport();
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

QRect TerminalWidget::gridToViewport(QRect rect) const
{
    int offset = verticalScrollBar()->value();

    int startRow = rect.y() - offset;
    int numRows = rect.height();
    int numCols = rect.width();

    QRect r{qFloor(rect.x() * m_cellSize.width()),
            qFloor(startRow * m_cellSize.height()),
            qCeil(numCols * m_cellSize.width()),
            qCeil(numRows * m_cellSize.height())};

    r.translate(0, topMargin());

    return r;
}

void TerminalWidget::updateViewport()
{
    viewport()->update();
}

void TerminalWidget::updateViewportRect(const QRect &rect)
{
    viewport()->update(rect);
}

void TerminalWidget::wheelEvent(QWheelEvent *event)
{
    verticalScrollBar()->event(event);
}

void TerminalWidget::focusInEvent(QFocusEvent *)
{
    updateViewport();
    configBlinkTimer();
    updateCopyState();
}
void TerminalWidget::focusOutEvent(QFocusEvent *)
{
    updateViewport();
    configBlinkTimer();
}

void TerminalWidget::inputMethodEvent(QInputMethodEvent *event)
{
    m_preEditString = event->preeditString();

    if (event->commitString().isEmpty()) {
        updateViewport();
        return;
    }

    m_surface->sendKey(event->commitString());
}

void TerminalWidget::mousePressEvent(QMouseEvent *event)
{
    m_scrollDirection = 0;

    m_activeMouseSelect.start = viewportToGlobal(event->pos());

    if (event->button() == Qt::LeftButton && event->modifiers() == Qt::ControlModifier) {
        if (m_linkSelection) {
            if (m_linkSelection->link.targetFilePath.scheme().toString().startsWith("http")) {
                QDesktopServices::openUrl(m_linkSelection->link.targetFilePath.toUrl());
                return;
            }

            if (m_linkSelection->link.targetFilePath.isDir())
                Core::FileUtils::showInFileSystemView(m_linkSelection->link.targetFilePath);
            else
                Core::EditorManager::openEditorAt(m_linkSelection->link);
        }
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (std::chrono::system_clock::now() - m_lastDoubleClick < 500ms) {
            m_selectLineMode = true;
            const Selection newSelection{m_surface->gridToPos(
                                             {0, m_surface->posToGrid(m_selection->start).y()}),
                                         m_surface->gridToPos(
                                             {m_surface->liveSize().width(),
                                              m_surface->posToGrid(m_selection->end).y()}),
                                         false};
            setSelection(newSelection);
        } else {
            m_selectLineMode = false;
            int pos = m_surface->gridToPos(globalToGrid(viewportToGlobal(event->pos())));
            setSelection(Selection{pos, pos, false});
        }
        event->accept();
        updateViewport();
    } else if (event->button() == Qt::RightButton) {
        if (event->modifiers() == Qt::ShiftModifier) {
            QMenu *contextMenu = new QMenu(this);
            contextMenu->addAction(&TerminalCommands::widgetActions().copy);
            contextMenu->addAction(&TerminalCommands::widgetActions().paste);
            contextMenu->addSeparator();
            contextMenu->addAction(&TerminalCommands::widgetActions().clearTerminal);
            contextMenu->addSeparator();
            contextMenu->addAction(TerminalCommands::openSettingsAction());

            contextMenu->popup(event->globalPos());
        } else if (m_selection) {
            copyToClipboard();
            setSelection(std::nullopt);
        } else {
            pasteFromClipboard();
        }
    } else if (event->button() == Qt::MiddleButton) {
        QClipboard *clipboard = QApplication::clipboard();
        if (clipboard->supportsSelection()) {
            const QString selectionText = clipboard->text(QClipboard::Selection);
            if (!selectionText.isEmpty())
                m_surface->pasteFromClipboard(selectionText);
        } else {
            m_surface->pasteFromClipboard(textFromSelection());
        }
    }
}
void TerminalWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selection && event->buttons() & Qt::LeftButton) {
        Selection newSelection = *m_selection;
        int scrollVelocity = 0;
        if (event->pos().y() < 0) {
            scrollVelocity = (event->pos().y());
        } else if (event->pos().y() > viewport()->height()) {
            scrollVelocity = (event->pos().y() - viewport()->height());
        }

        if ((scrollVelocity != 0) != m_scrollTimer.isActive()) {
            if (scrollVelocity != 0)
                m_scrollTimer.start();
            else
                m_scrollTimer.stop();
        }

        m_scrollDirection = scrollVelocity;

        if (m_scrollTimer.isActive() && scrollVelocity != 0) {
            const std::chrono::milliseconds scrollInterval = 1000ms / qAbs(scrollVelocity);
            if (m_scrollTimer.intervalAsDuration() != scrollInterval)
                m_scrollTimer.setInterval(scrollInterval);
        }

        int start = m_surface->gridToPos(globalToGrid(m_activeMouseSelect.start));
        int newEnd = m_surface->gridToPos(globalToGrid(viewportToGlobal(event->pos())));

        if (start > newEnd) {
            std::swap(start, newEnd);
        }
        if (start < 0)
            start = 0;

        if (m_selectLineMode) {
            newSelection.start = m_surface->gridToPos({0, m_surface->posToGrid(start).y()});
            newSelection.end = m_surface->gridToPos(
                {m_surface->liveSize().width(), m_surface->posToGrid(newEnd).y()});
        } else {
            newSelection.start = start;
            newSelection.end = newEnd;
        }

        setSelection(newSelection);
    } else if (event->modifiers() == Qt::ControlModifier) {
        checkLinkAt(event->pos());
    } else if (m_linkSelection) {
        m_linkSelection.reset();
        updateViewport();
    }

    if (m_linkSelection) {
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::IBeamCursor);
    }
}

void TerminalWidget::checkLinkAt(const QPoint &pos)
{
    const TextAndOffsets hit = textAt(pos);

    if (hit.text.size() > 0) {
        QString t = chopIfEndsWith(QString::fromUcs4(hit.text.c_str(), hit.text.size()).trimmed(),
                                   ':');
        if (t.startsWith("~/")) {
            t = QDir::homePath() + t.mid(1);
        }

        Link link = Link::fromString(t, true);

        if (!link.targetFilePath.isAbsolutePath())
            link.targetFilePath = m_cwd.pathAppended(link.targetFilePath.path());

        if (link.hasValidTarget()
            && (link.targetFilePath.scheme().toString().startsWith("http")
                || link.targetFilePath.exists())) {
            const LinkSelection newSelection = LinkSelection{{hit.start, hit.end}, link};
            if (!m_linkSelection || *m_linkSelection != newSelection) {
                m_linkSelection = newSelection;
                updateViewport();
            }
            return;
        }
    }

    if (m_linkSelection) {
        m_linkSelection.reset();
        updateViewport();
    }
}

void TerminalWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_scrollTimer.stop();

    if (m_selection && event->button() == Qt::LeftButton) {
        if (m_selection->end - m_selection->start == 0)
            setSelection(std::nullopt);
        else
            setSelection(Selection{m_selection->start, m_selection->end, true});
    }
}

TerminalWidget::TextAndOffsets TerminalWidget::textAt(const QPoint &pos) const
{
    auto it = m_surface->iteratorAt(globalToGrid(viewportToGlobal(pos)));
    auto itRev = m_surface->rIteratorAt(globalToGrid(viewportToGlobal(pos)));

    std::u32string whiteSpaces = U" \t\x00a0";

    const bool inverted = whiteSpaces.find(*it) != std::u32string::npos || *it == 0;

    auto predicate = [inverted, whiteSpaces](const std::u32string::value_type &ch) {
        if (inverted)
            return ch != 0 && whiteSpaces.find(ch) == std::u32string::npos;
        else
            return ch == 0 || whiteSpaces.find(ch) != std::u32string::npos;
    };

    auto itRight = std::find_if(it, m_surface->end(), predicate);
    auto itLeft = std::find_if(itRev, m_surface->rend(), predicate);

    std::u32string text;
    std::copy(itLeft.base(), it, std::back_inserter(text));
    std::copy(it, itRight, std::back_inserter(text));

    return {(itLeft.base()).position(), itRight.position(), text};
}

void TerminalWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    const auto hit = textAt(event->pos());

    setSelection(Selection{hit.start, hit.end, true});

    m_lastDoubleClick = std::chrono::system_clock::now();

    event->accept();
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
        p.fillRect(QRect(QPoint(0, 0), size()), m_currentColors[ColorIndex::Background]);
        return true;
    }

    return QAbstractScrollArea::event(event);
}

} // namespace Terminal
