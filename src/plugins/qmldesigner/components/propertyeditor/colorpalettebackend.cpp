// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colorpalettebackend.h"

#include "propertyeditortracing.h"

#include <coreplugin/icore.h>

#include <QApplication>
#include <QColorDialog>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QTimer>

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>

namespace QmlDesigner {

QPointer<ColorPaletteBackend> ColorPaletteBackend::m_instance = nullptr;

using PropertyEditorTracing::category;

ColorPaletteBackend::ColorPaletteBackend()
    : m_currentPalette()
    , m_data()
    , m_eyeDropperEventFilter(nullptr)
#ifdef Q_OS_WIN32
    , updateTimer(0)
#endif
{
    NanotraceHR::Tracer tracer{"color palette backend constructor", category()};

    m_data.insert(g_recent, Palette(PaletteType::Recent));
    m_data.insert(g_favorite, Palette(PaletteType::Favorite));

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
    NanotraceHR::Tracer tracer{"color palette backend destructor", category()};

    //writePalettes(); // TODO crash on QtDS close
    if (m_eyeDropperActive)
        eyeDropperLeave(QCursor::pos(), EyeDropperEventFilter::LeaveReason::Default);
}

void ColorPaletteBackend::readPalettes()
{
    NanotraceHR::Tracer tracer{"color palette backend read palettes", category()};

    for (auto &palette : m_data)
        palette.read();
}

void ColorPaletteBackend::writePalettes()
{
    NanotraceHR::Tracer tracer{"color palette backend write palettes", category()};

    for (auto &palette : m_data)
        palette.write();
}

void ColorPaletteBackend::addColor(const QString &color, const QString &paletteName)
{
    NanotraceHR::Tracer tracer{"color palette backend add color", category()};

    auto found = m_data.find(paletteName);

    if (found == m_data.end()) {
        qWarning() << Q_FUNC_INFO << "Unknown palette: " << paletteName;
        return;
    }

    auto palette = *found;

    // If palette is currently active palette also add it to the local color list
    if (paletteName == m_currentPalette) {
        if (m_currentPaletteColors.size() + 1 > g_maxPaletteSize)
            m_currentPaletteColors.removeLast();

        m_currentPaletteColors.prepend(color);
        emit currentPaletteColorsChanged();
    }

    if (palette.m_colors.size() + 1 > g_maxPaletteSize)
        palette.m_colors.removeLast();

    palette.m_colors.prepend(color);
    palette.write();
}

void ColorPaletteBackend::removeColor(int id, const QString &paletteName)
{
    NanotraceHR::Tracer tracer{"color palette backend remove color", category()};

    auto found = m_data.find(paletteName);

    if (found == m_data.end()) {
        qWarning() << Q_FUNC_INFO << "Unknown palette: " << paletteName;
        return;
    }
    auto palette = *found;

    if (id >= palette.m_colors.size()) {
        qWarning() << Q_FUNC_INFO << "Id(" << id << ") is out of bounds for palette " << paletteName;
        return;
    }

    // If palette is currently active palette also add it to the local color list
    if (paletteName == m_currentPalette) {
        m_currentPaletteColors.removeAt(id);

        // Fill up with empty strings
        while (m_currentPaletteColors.size() < g_maxPaletteSize)
            m_currentPaletteColors.append("");

        emit currentPaletteColorsChanged();
    }

    palette.m_colors.removeAt(id);
    palette.write();
}

void ColorPaletteBackend::addRecentColor(const QString &item)
{
    NanotraceHR::Tracer tracer{"color palette backend add recent color", category()};

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
    NanotraceHR::Tracer tracer{"color palette backend add favorite color", category()};

    addColor(item, g_favorite);
}

void ColorPaletteBackend::removeFavoriteColor(int id)
{
    NanotraceHR::Tracer tracer{"color palette backend  remove favorite color", category()};

    removeColor(id, g_favorite);
}

QStringList ColorPaletteBackend::palettes() const
{
    NanotraceHR::Tracer tracer{"color palette backend get palettes", category()};

    return m_data.keys();
}

const QString &ColorPaletteBackend::currentPalette() const
{
    NanotraceHR::Tracer tracer{"color palette backend get current palette", category()};

    return m_currentPalette;
}

void ColorPaletteBackend::setCurrentPalette(const QString &palette)
{
    NanotraceHR::Tracer tracer{"color palette backend set current palette", category()};

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
    NanotraceHR::Tracer tracer{"color palette backend get current palette colors", category()};

    return m_currentPaletteColors;
}

void ColorPaletteBackend::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"color palette backend register declarative type", category()};

    [[maybe_unused]] static const int typeIndex = qmlRegisterSingletonType<ColorPaletteBackend>(
        "QtQuickDesignerColorPalette", 1, 0, "ColorPaletteBackend", [](QQmlEngine *, QJSEngine *) {
            return new ColorPaletteBackend();
        });
}

void ColorPaletteBackend::showDialog(QColor color)
{
    NanotraceHR::Tracer tracer{"color palette backend show dialog", category()};

    auto colorDialog = new QColorDialog(Core::ICore::dialogParent());
    colorDialog->setCurrentColor(color);
    colorDialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(colorDialog, &QDialog::rejected,
            this, &ColorPaletteBackend::colorDialogRejected);
    connect(colorDialog, &QColorDialog::currentColorChanged,
            this, &ColorPaletteBackend::currentColorChanged);

    QTimer::singleShot(0, [colorDialog](){ colorDialog->exec(); });
}

void ColorPaletteBackend::invokeEyeDropper()
{
    NanotraceHR::Tracer tracer{"color palette backend invoke eye dropper", category()};

    eyeDropperEnter();
}

void ColorPaletteBackend::eyeDropperEnter()
{
    NanotraceHR::Tracer tracer{"color palette backend eye dropper enter", category()};

    if (m_eyeDropperActive)
        return;

    QPointer<QWindow> window = Core::ICore::mainWindow()->windowHandle();

    if (m_eyeDropperWindow.isNull()) {
        if (window.isNull()) {
            qWarning() << "No window found, cannot enter eyeDropperMode.";
            return;
        }

        m_eyeDropperWindow = window;
    }

    if (auto *platformServices = QGuiApplicationPrivate::platformIntegration()->services();
        platformServices
        && platformServices->hasCapability(QPlatformServices::Capability::ColorPicking)) {
        if (auto *colorPickerService = platformServices->colorPicker(m_eyeDropperWindow)) {
            connect(colorPickerService,
                    &QPlatformServiceColorPicker::colorPicked,
                    this,
                    [this, colorPickerService](const QColor &color) {
                        colorPickerService->deleteLater();
                        // When the service was canceled by pressing escape the color returned will
                        // be QColor(ARGB, 0, 0, 0, 0), so we test for alpha to avoid setting the color.
                        // https://codereview.qt-project.org/c/qt/qtbase/+/589113
                        if (color.alpha() != 0 || !color.isValid())
                            emit currentColorChanged(color);
                        m_eyeDropperActive = false;
                        emit eyeDropperActiveChanged();
                    });
            m_eyeDropperActive = true;
            emit eyeDropperActiveChanged();
            colorPickerService->pickColor();
            return;
        }
    }

    m_eyeDropperPreviousColor = m_eyeDropperCurrentColor;

    if (!bool(m_eyeDropperEventFilter))
        m_eyeDropperEventFilter.reset(new EyeDropperEventFilter(
            [this](QPoint pos, EyeDropperEventFilter::LeaveReason c) { eyeDropperLeave(pos, c); },
            [this](QPoint pos) { eyeDropperPointerMoved(pos); }));

    if (m_eyeDropperWindow->setMouseGrabEnabled(true)
        && m_eyeDropperWindow->setKeyboardGrabEnabled(true)) {
#if QT_CONFIG(cursor)
        QGuiApplication::setOverrideCursor(QCursor());
        updateEyeDropperPosition(QCursor::pos());
#endif
        m_eyeDropperWindow->installEventFilter(m_eyeDropperEventFilter.get());
        m_eyeDropperActive = true;
        emit eyeDropperActiveChanged();
    }
}

void ColorPaletteBackend::eyeDropperLeave(const QPoint &pos,
                                          EyeDropperEventFilter::LeaveReason actionOnLeave)
{
    NanotraceHR::Tracer tracer{"color palette backend eye dropper leave", category()};

    if (!m_eyeDropperActive)
        return;

    if (!m_eyeDropperWindow) {
        qWarning() << "Window not set, cannot leave eyeDropperMode.";
        return;
    }

    if (actionOnLeave != EyeDropperEventFilter::LeaveReason::Cancel) {
        m_eyeDropperCurrentColor = grabScreenColor(pos);

        emit currentColorChanged(m_eyeDropperCurrentColor);
    }

    m_eyeDropperWindow->removeEventFilter(m_eyeDropperEventFilter.get());
    m_eyeDropperWindow->setMouseGrabEnabled(false);
    m_eyeDropperWindow->setKeyboardGrabEnabled(false);
#if QT_CONFIG(cursor)
    QGuiApplication::restoreOverrideCursor();
#endif

    m_eyeDropperActive = false;
    emit eyeDropperActiveChanged();
    m_eyeDropperWindow.clear();
}

void ColorPaletteBackend::eyeDropperPointerMoved(const QPoint &pos)
{
    NanotraceHR::Tracer tracer{"color palette backend eye dropper pointer moved", category()};

    m_eyeDropperCurrentColor = grabScreenColor(pos);
    updateEyeDropperPosition(pos);
}

const QSize g_cursorSize(64, 64);
const QSize g_halfCursorSize = g_cursorSize / 2;
const QSize g_screenGrabSize(11, 11);
const QSize g_previewSize(12, 12);
const QSize g_halfPreviewSize = g_previewSize / 2;

QColor ColorPaletteBackend::grabScreenColor(const QPoint &p)
{
    NanotraceHR::Tracer tracer{"color palette backend grab screen color", category()};

    return grabScreenRect(QRect(p, QSize(1, 1))).pixel(0, 0);
}

QImage ColorPaletteBackend::grabScreenRect(const QRect &r)
{
    NanotraceHR::Tracer tracer{"color palette backend grab screen rect", category()};

    QScreen *screen = QGuiApplication::screenAt(r.topLeft());
    if (!screen)
        screen = QGuiApplication::primaryScreen();

    const QRect screenRect = screen->geometry();
    const QPixmap pixmap = screen->grabWindow(0,
                                              r.x() - screenRect.x(),
                                              r.y() - screenRect.y(),
                                              r.width(),
                                              r.height());

    return pixmap.toImage();
}

void ColorPaletteBackend::updateEyeDropper()
{
    NanotraceHR::Tracer tracer{"color palette backend update eye dropper", category()};

#ifndef QT_NO_CURSOR
    static QPoint lastGlobalPos;
    const QPoint newGlobalPos = QCursor::pos();
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
    NanotraceHR::Tracer tracer{"color palette backend update eye dropper position", category()};

#if QT_CONFIG(cursor)
    QPoint topLeft = globalPos - QPoint(g_halfCursorSize.width(), g_halfCursorSize.height());
    updateCursor(grabScreenRect(QRect(topLeft, g_cursorSize)));
#endif
}

void ColorPaletteBackend::updateCursor(const QImage &image)
{
    NanotraceHR::Tracer tracer{"color palette backend update cursor", category()};

    QWindow *window = Core::ICore::mainWindow()->windowHandle();
    if (!window)
        return;

    QPixmap pixmap(g_cursorSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath clipPath;
    clipPath.addEllipse(QRect(QPoint(0, 0), g_cursorSize).adjusted(1, 1, -1, -1));

    painter.setClipPath(clipPath, Qt::IntersectClip);

    const QPoint topLeft = QPoint(g_halfCursorSize.width(), g_halfCursorSize.height())
                           - QPoint(qFloor(g_screenGrabSize.width() / 2),
                                    qFloor(g_screenGrabSize.height() / 2));

    const QColor topCenter = image.pixelColor(g_halfCursorSize.width(), 0);
    const QColor bottomCenter = image.pixelColor(g_halfCursorSize.width(), g_cursorSize.height() - 1);
    const QColor leftCenter = image.pixelColor(0, g_halfCursorSize.height());
    const QColor rightCenter = image.pixelColor(g_cursorSize.width() - 1, g_halfCursorSize.height());

    // Find the mean gray value of top, bottom, left and right center
    int borderGray = (qGray(topCenter.rgb()) + qGray(bottomCenter.rgb()) + qGray(leftCenter.rgb())
                      + qGray(rightCenter.rgb()))
                     / 4;

    // Draw the magnified image/screen grab
    const QRect r(QPoint(), pixmap.size());
    painter.drawImage(r, image, QRect(topLeft, g_screenGrabSize));

    painter.setClipping(false);

    QPen pen(borderGray < 128 ? Qt::white : Qt::black, 2.0, Qt::SolidLine);

    // Draw the surrounding border
    painter.setPen(pen);
    painter.drawPath(clipPath);

    pen.setWidth(1);
    painter.setPen(pen);

    const QRect previewRect(QPoint(g_halfCursorSize.width() - g_halfPreviewSize.width(),
                                   g_halfCursorSize.height() - g_halfPreviewSize.height()),
                            g_previewSize);

    painter.fillRect(previewRect, m_eyeDropperCurrentColor);
    painter.drawRect(previewRect);

    painter.end();

    QGuiApplication::changeOverrideCursor(QCursor(pixmap));
}

bool ColorPaletteBackend::eyeDropperActive() const
{
    NanotraceHR::Tracer tracer{"color palette backend eye dropper active", category()};
    return m_eyeDropperActive;
}

} // namespace QmlDesigner
