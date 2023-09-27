// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colorpalettebackend.h"

#include <QDebug>
#include <QColorDialog>
#include <QTimer>

#include <QApplication>
#include <QScreen>
#include <QPainter>

namespace QmlDesigner {

QPointer<ColorPaletteBackend> ColorPaletteBackend::m_instance = nullptr;

ColorPaletteBackend::ColorPaletteBackend()
    : m_currentPalette()
    , m_data()
    , m_colorPickingEventFilter(nullptr)
#ifdef Q_OS_WIN32
    , updateTimer(0)
#endif
{
    m_data.insert(g_recent, Palette(QmlDesigner::DesignerSettingsKey::COLOR_PALETTE_RECENT));
    m_data.insert(g_favorite, Palette(QmlDesigner::DesignerSettingsKey::COLOR_PALETTE_FAVORITE));

    readPalettes();

    // Set recent color palette by default
    setCurrentPalette(g_recent);

#ifdef Q_OS_WIN32
    dummyTransparentWindow.resize(1, 1);
    dummyTransparentWindow.setFlags(Qt::Tool | Qt::FramelessWindowHint);
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &ColorPaletteBackend::updateEyeDropper);
#endif
}

ColorPaletteBackend::~ColorPaletteBackend()
{
    //writePalettes(); // TODO crash on QtDS close
}

void ColorPaletteBackend::readPalettes()
{
    QHash<QString, Palette>::iterator i = m_data.begin();
    while (i != m_data.end()) {
        i.value().read();
        ++i;
    }
}

void ColorPaletteBackend::writePalettes()
{
    QHash<QString, Palette>::iterator i = m_data.begin();
    while (i != m_data.end()) {
        i.value().write();
        ++i;
    }
}

void ColorPaletteBackend::addColor(const QString &color, const QString &palette)
{
    if (!m_data.contains(palette)) {
        qWarning() << Q_FUNC_INFO << "Unknown palette: " << palette;
        return;
    }

    // If palette is currently active palette also add it to the local color list
    if (palette == m_currentPalette) {
        if (m_currentPaletteColors.size() + 1 > g_maxPaletteSize)
            m_currentPaletteColors.removeLast();

        m_currentPaletteColors.prepend(color);
        emit currentPaletteColorsChanged();
    }

    if (m_data[palette].m_colors.size() + 1 > g_maxPaletteSize)
        m_data[palette].m_colors.removeLast();

    m_data[palette].m_colors.prepend(color);
    m_data[palette].write();
}

void ColorPaletteBackend::removeColor(int id, const QString &palette)
{
    if (!m_data.contains(palette)) {
        qWarning() << Q_FUNC_INFO << "Unknown palette: " << palette;
        return;
    }

    if (id >= m_data[palette].m_colors.size()) {
        qWarning() << Q_FUNC_INFO << "Id(" << id << ") is out of bounds for palette " << palette;
        return;
    }

    // If palette is currently active palette also add it to the local color list
    if (palette == m_currentPalette) {
        m_currentPaletteColors.removeAt(id);

        // Fill up with empty strings
        while (m_currentPaletteColors.size() < g_maxPaletteSize)
            m_currentPaletteColors.append("");

        emit currentPaletteColorsChanged();
    }

    m_data[palette].m_colors.removeAt(id);
    m_data[palette].write();
}

void ColorPaletteBackend::addRecentColor(const QString &item)
{
    if (m_data[g_recent].m_colors.isEmpty()) {
        addColor(item, g_recent);
        return;
    }

    // Don't add recent color if the first one is the same
    if (m_data[g_recent].m_colors.constFirst() != item)
        addColor(item, g_recent);
}

void ColorPaletteBackend::addFavoriteColor(const QString &item)
{
    addColor(item, g_favorite);
}

void ColorPaletteBackend::removeFavoriteColor(int id)
{
    removeColor(id, g_favorite);
}

QStringList ColorPaletteBackend::palettes() const
{
    return m_data.keys();
}

const QString &ColorPaletteBackend::currentPalette() const
{
    return m_currentPalette;
}

void ColorPaletteBackend::setCurrentPalette(const QString &palette)
{
    if (!m_data.contains(palette)) {
        qWarning() << Q_FUNC_INFO << "Unknown palette: " << palette;
        return;
    }

    if (m_currentPalette == palette)
        return;

    // Store the current palette in settings
    if (!m_currentPalette.isEmpty() && m_currentPalette != palette)
        m_data[m_currentPalette].write();

    m_currentPalette = palette;
    m_currentPaletteColors.clear();

    for (const QString &color : m_data[m_currentPalette].m_colors)
        m_currentPaletteColors.append(color);

    // Prune to max palette size
    while (m_currentPaletteColors.size() > g_maxPaletteSize)
        m_currentPaletteColors.removeLast();

    // Fill up with empty strings
    while (m_currentPaletteColors.size() < g_maxPaletteSize)
        m_currentPaletteColors.append("");

    emit currentPaletteChanged(m_currentPalette);
    emit currentPaletteColorsChanged();
}

const QStringList &ColorPaletteBackend::currentPaletteColors() const
{
    return m_currentPaletteColors;
}

void ColorPaletteBackend::registerDeclarativeType()
{
    [[maybe_unused]] static const int typeIndex = qmlRegisterSingletonType<ColorPaletteBackend>(
        "QtQuickDesignerColorPalette", 1, 0, "ColorPaletteBackend", [](QQmlEngine *, QJSEngine *) {
            return new ColorPaletteBackend();
        });
}

void ColorPaletteBackend::showDialog(QColor color)
{
    auto colorDialog = new QColorDialog(Core::ICore::dialogParent());
    colorDialog->setCurrentColor(color);
    colorDialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(colorDialog, &QDialog::rejected,
            this, &ColorPaletteBackend::colorDialogRejected);
    connect(colorDialog, &QColorDialog::currentColorChanged,
            this, &ColorPaletteBackend::currentColorChanged);

    QTimer::singleShot(0, [colorDialog](){ colorDialog->exec(); });
}

void ColorPaletteBackend::eyeDropper()
{
    QWidget *widget = QApplication::activeWindow();
    if (!widget)
        return;

    if (!m_colorPickingEventFilter)
        m_colorPickingEventFilter = new QColorPickingEventFilter(this);

    widget->installEventFilter(m_colorPickingEventFilter);

#ifndef QT_NO_CURSOR
    widget->grabMouse(/*Qt::CrossCursor*/);
#else
    w->grabMouse();
#endif
#ifdef Q_OS_WIN32 // excludes WinRT
    // On Windows mouse tracking doesn't work over other processes's windows
    updateTimer->start(30);
    // HACK: Because mouse grabbing doesn't work across processes, we have to have a dummy,
    // invisible window to catch the mouse click, otherwise we will click whatever we clicked
    // and loose focus.
    dummyTransparentWindow.show();
#endif
    widget->grabKeyboard();
    /* With setMouseTracking(true) the desired color can be more precisely picked up,
     * and continuously pushing the mouse button is not necessary.
     */
    widget->setMouseTracking(true);
    updateEyeDropperPosition(QCursor::pos());
}

const int g_cursorWidth = 64;
const int g_cursorHeight = 64;
const int g_screenGrabWidth = 7;
const int g_screenGrabHeight = 7;
const int g_pixelX = 3;
const int g_pixelY = 3;

QColor ColorPaletteBackend::grabScreenColor(const QPoint &p)
{
    return grabScreenRect(p).pixel(g_pixelX, g_pixelY);
}

QImage ColorPaletteBackend::grabScreenRect(const QPoint &p)
{
    QScreen *screen = QGuiApplication::screenAt(p);
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    const QPixmap pixmap = screen->grabWindow(0, p.x(), p.y(), g_screenGrabWidth, g_screenGrabHeight);
    return pixmap.toImage();
}

void ColorPaletteBackend::updateEyeDropper()
{
#ifndef QT_NO_CURSOR
    static QPoint lastGlobalPos;
    QPoint newGlobalPos = QCursor::pos();
    if (lastGlobalPos == newGlobalPos)
        return;

    lastGlobalPos = newGlobalPos;

    updateEyeDropperPosition(newGlobalPos);
#ifdef Q_OS_WIN32
    dummyTransparentWindow.setPosition(newGlobalPos);
#endif
#endif // ! QT_NO_CURSOR
}

void ColorPaletteBackend::updateEyeDropperPosition(const QPoint &globalPos)
{
    updateCursor(grabScreenRect(globalPos));
}

void ColorPaletteBackend::updateCursor(const QImage &image)
{
    QWidget *widget = QApplication::activeWindow();
    if (!widget)
        return;

    QPixmap pixmap(QSize(g_cursorWidth, g_cursorHeight));
    QPainter painter(&pixmap);
    // Draw the magnified image/screen grab
    QRect r(QPoint(), pixmap.size());
    painter.drawImage(r, image, QRect(0, 0, g_screenGrabWidth, g_screenGrabHeight));

    const int pixelWidth = (g_cursorWidth - 1) / g_screenGrabWidth;
    const int pixelHeight = (g_cursorHeight - 1) / g_screenGrabHeight;
    // Draw the pixel lines
    painter.setPen(QPen(QColor(192, 192, 192, 150), 1.0, Qt::SolidLine));
    for (int i = 1; i != g_screenGrabWidth; ++i) {
        int x = pixelWidth * i;
        painter.drawLine(x, 0, x, g_cursorHeight);
    }

    for (int i = 1; i != g_screenGrabHeight; ++i) {
        int y = pixelHeight * i;
        painter.drawLine(0, y, g_cursorWidth, y);
    }
    // Draw the sorounding border
    painter.setPen(QPen(Qt::black, 1.0, Qt::SolidLine));
    painter.drawRect(r.adjusted(0, 0, -1, -1));

    const QColor color = image.pixel(g_pixelX, g_pixelY);
    QRect centerRect = QRect(2 * pixelWidth, 2 * pixelHeight, 3 * pixelWidth, 3 * pixelHeight);
    // Draw the center rectangle with the current eye dropper color
    painter.setBrush(QBrush(color, Qt::SolidPattern));
    painter.drawRect(centerRect);

    painter.end();
    QCursor cursor(pixmap);
    widget->setCursor(cursor);
}

void ColorPaletteBackend::releaseEyeDropper()
{
    QWidget *widget = QApplication::activeWindow();
    if (!widget)
        return;

    widget->removeEventFilter(m_colorPickingEventFilter);
    widget->releaseMouse();
#ifdef Q_OS_WIN32
    updateTimer->stop();
    dummyTransparentWindow.setVisible(false);
#endif
    widget->releaseKeyboard();
    widget->setMouseTracking(false);

    widget->unsetCursor();
}

bool ColorPaletteBackend::handleEyeDropperMouseMove(QMouseEvent *e)
{
    updateEyeDropperPosition(e->globalPosition().toPoint());
    return true;
}

bool ColorPaletteBackend::handleEyeDropperMouseButtonRelease(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        emit currentColorChanged(grabScreenColor(e->globalPosition().toPoint()));
    else
        emit eyeDropperRejected();

    releaseEyeDropper();
    return true;
}

bool ColorPaletteBackend::handleEyeDropperKeyPress(QKeyEvent *e)
{
#if QT_CONFIG(shortcut)
    if (e->matches(QKeySequence::Cancel)) {
        emit eyeDropperRejected();
        releaseEyeDropper();
    } //else
#endif
    //if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
    //    emit currentColorChanged(grabScreenColor(e->globalPosition().toPoint()));
    //    releaseEyeDropper();
    //}
    e->accept();
    return true;
}

/// EYE DROPPER


} // namespace QmlDesigner
