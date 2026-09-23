// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "algorithm.h"
#include "hostosinfo.h"
#include "icon.h"
#include "qtcassert.h"
#include "stylehelper.h"
#include "theme/theme.h"
#include "utilsicons.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QIconEngine>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPixmapCache>
#include <QWidget>

#include <functional>

namespace Utils {

static const qreal PunchEdgeWidth = 0.5;
static const qreal PunchEdgeIntensity = 0.6;

static QPixmap maskToColorAndAlpha(const QPixmap &mask, const QColor &color)
{
    QImage result(mask.toImage().convertToFormat(QImage::Format_ARGB32));
    result.setDevicePixelRatio(mask.devicePixelRatio());
    auto bitsStart = reinterpret_cast<QRgb*>(result.bits());
    const QRgb *bitsEnd = bitsStart + result.width() * result.height();
    const QRgb tint = color.rgb() & 0x00ffffff;
    const auto alpha = QRgb(color.alpha());
    for (QRgb *pixel = bitsStart; pixel < bitsEnd; ++pixel) {
        QRgb pixelAlpha = (((~*pixel) & 0xff) * alpha) >> 8;
        *pixel = (pixelAlpha << 24) | tint;
    }
    return QPixmap::fromImage(result);
}

using MaskAndColor = QPair<QPixmap, QColor>;
using MasksAndColors = QList<MaskAndColor>;
static MasksAndColors masksAndColors(const QList<IconMaskAndColor> &icon, int dpr)
{
    MasksAndColors result;
    for (const IconMaskAndColor &i: icon) {
        const QString &fileName = i.first.toFSPathString();
        const QColor color = creatorColor(i.second);
        const QString dprFileName = StyleHelper::availableImageResolutions(i.first.toFSPathString())
                                            .contains(dpr)
                                        ? StyleHelper::imageFileWithResolution(fileName, dpr)
                                        : fileName;
        QPixmap pixmap;
        if (!pixmap.load(dprFileName)) {
            pixmap = QPixmap(1, 1);
            qWarning() << "Could not load image: " << dprFileName;
        }
        result.append({pixmap, color});
    }
    return result;
}

static void smearPixmap(QPainter *painter, const QPixmap &pixmap, qreal radius)
{
    const qreal nagative = -radius - 0.01; // Workaround for QPainter rounding behavior
    const qreal positive = radius;
    painter->drawPixmap(QPointF(nagative, nagative), pixmap);
    painter->drawPixmap(QPointF(0, nagative), pixmap);
    painter->drawPixmap(QPointF(positive, nagative), pixmap);
    painter->drawPixmap(QPointF(positive, 0), pixmap);
    painter->drawPixmap(QPointF(positive, positive), pixmap);
    painter->drawPixmap(QPointF(0, positive), pixmap);
    painter->drawPixmap(QPointF(nagative, positive), pixmap);
    painter->drawPixmap(QPointF(nagative, 0), pixmap);
}

static QPixmap combinedMask(const MasksAndColors &masks, Icon::IconStyleOptions style)
{
    if (masks.count() == 1)
        return masks.first().first;

    QPixmap result(masks.first().first);
    QPainter p(&result);
    p.setCompositionMode(QPainter::CompositionMode_Darken);
    auto maskImage = masks.constBegin();
    maskImage++;
    for (;maskImage != masks.constEnd(); ++maskImage) {
        if (style & Icon::PunchEdges) {
            p.save();
            p.setOpacity(PunchEdgeIntensity);
            p.setCompositionMode(QPainter::CompositionMode_Lighten);
            smearPixmap(&p, maskToColorAndAlpha((*maskImage).first, Qt::white), PunchEdgeWidth);
            p.restore();
        }
        p.drawPixmap(0, 0, (*maskImage).first);
    }
    p.end();
    return result;
}

static QPixmap masksToIcon(const MasksAndColors &masks, const QPixmap &combinedMask, Icon::IconStyleOptions style)
{
    QPixmap result(combinedMask.size());
    result.setDevicePixelRatio(combinedMask.devicePixelRatio());
    result.fill(Qt::transparent);
    QPainter p(&result);

    for (MasksAndColors::const_iterator maskImage = masks.constBegin();
         maskImage != masks.constEnd(); ++maskImage) {
        if (style & Icon::PunchEdges && maskImage != masks.constBegin()) {
            // Punch a transparent outline around an overlay.
            p.save();
            p.setOpacity(PunchEdgeIntensity);
            p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            smearPixmap(&p, maskToColorAndAlpha((*maskImage).first, Qt::white), PunchEdgeWidth);
            p.restore();
        }
        p.drawPixmap(0, 0, maskToColorAndAlpha((*maskImage).first, (*maskImage).second));
    }

    if (style & Icon::DropShadow && creatorTheme()->flag(Theme::ToolBarIconShadow)) {
        const QPixmap shadowMask = maskToColorAndAlpha(combinedMask, Qt::black);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        p.setOpacity(0.08);
        p.drawPixmap(QPointF(0, -0.501), shadowMask);
        p.drawPixmap(QPointF(-0.501, 0), shadowMask);
        p.drawPixmap(QPointF(0.5, 0), shadowMask);
        p.drawPixmap(QPointF(0.5, 0.5), shadowMask);
        p.drawPixmap(QPointF(-0.501, 0.5), shadowMask);
        p.setOpacity(0.3);
        p.drawPixmap(0, 1, shadowMask);
    }

    p.end();

    return result;
}

Icon::Icon() = default;

Icon::Icon(const QList<IconMaskAndColor> &args, Icon::IconStyleOptions style)
    : m_iconSourceList(args)
    , m_style(style)
{
}

Icon::Icon(const FilePath &imageFileName)
    : m_iconSourceList({{imageFileName, Theme::Color(-1)}})
{
}

using OptMasksAndColors = std::optional<MasksAndColors>;
static OptMasksAndColors highlightMasksAndColors(const MasksAndColors &defaultState,
                                                 const QList<IconMaskAndColor> &masks)
{
    MasksAndColors highlighted = defaultState;
    bool colorsReplaced = false;
    int index = 0;
    for (const IconMaskAndColor &mask : masks) {
        const Theme::Color highlight = Theme::highlightFor(mask.second);
        if (highlight != mask.second) {
            highlighted[index].second = creatorColor(highlight);
            colorsReplaced = true;
            continue;
        }
        ++index;
    }
    return colorsReplaced ? std::make_optional(highlighted) : std::nullopt;
}

static QIcon renderIcon(const QList<IconMaskAndColor> &iconSourceList,
                        Icon::IconStyleOptions style,
                        int maxDpr)
{
    QIcon result;
    for (int dpr = 1; dpr <= maxDpr; dpr++) {
        const MasksAndColors masks = masksAndColors(iconSourceList, dpr);
        const QPixmap combinedMask = Utils::combinedMask(masks, style);
        result.addPixmap(masksToIcon(masks, combinedMask, style), QIcon::Normal, QIcon::Off);
        const QColor disabledColor = creatorColor(Theme::IconsDisabledColor);
        const QPixmap disabledIcon = maskToColorAndAlpha(combinedMask, disabledColor);
        if (const OptMasksAndColors activeMasks =
            highlightMasksAndColors(masks, iconSourceList);
            activeMasks.has_value()) {
            const QPixmap activePixmap = masksToIcon(*activeMasks, combinedMask, style);
            result.addPixmap(activePixmap, QIcon::Active, QIcon::On);
            result.addPixmap(disabledIcon, QIcon::Disabled, QIcon::On);
            result.addPixmap(disabledIcon, QIcon::Disabled, QIcon::Off);
        } else {
            result.addPixmap(disabledIcon, QIcon::Disabled);
        }
    }
    return result;
}

// Resolves the theme colors when the icon is drawn instead of when it is
// created, so that icons which were handed to a QAction or a QWidget once
// follow a theme change.
class ThemedIconEngine final : public QIconEngine
{
public:
    explicit ThemedIconEngine(const std::function<QIcon()> &render)
        : m_render(render)
    {}

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        // Fills the rect the way QPixmapIconEngine does, which also scales up.
        // QIcon::paint() would clamp to actualSize(), which never does.
        const QPaintDevice *device = painter->device();
        const qreal dpr = device ? device->devicePixelRatio() : qApp->devicePixelRatio();
        painter->drawPixmap(rect, icon().pixmap(rect.size(), dpr, mode, state));
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        return icon().pixmap(size, mode, state);
    }

    QPixmap scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state,
                         qreal scale) override
    {
        return icon().pixmap(size, scale, mode, state);
    }

    QSize actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        return icon().actualSize(size, mode, state);
    }

    QList<QSize> availableSizes(QIcon::Mode mode, QIcon::State state) override
    {
        return icon().availableSizes(mode, state);
    }

    QIconEngine *clone() const override { return new ThemedIconEngine(m_render); }

    QString key() const override { return "Utils::ThemedIconEngine"; }

    bool isNull() override { return false; }

private:
    // Rendering a recolored icon is expensive enough to be worth keeping, but
    // the result only stays valid for one theme and one device pixel ratio.
    QIcon icon() const
    {
        const int maxDpr = qRound(qApp->devicePixelRatio());
        if (m_generation != ThemeManager::generation() || m_devicePixelRatio != maxDpr) {
            m_generation = ThemeManager::generation();
            m_devicePixelRatio = maxDpr;
            m_icon = m_render();
        }
        return m_icon;
    }

    const std::function<QIcon()> m_render;
    mutable QIcon m_icon;
    mutable int m_generation = -1;
    mutable int m_devicePixelRatio = -1;
};

static QString sourceListKey(const QList<IconMaskAndColor> &iconSourceList,
                             Icon::IconStyleOptions style)
{
    QString result = QString::number(int(style));
    for (const IconMaskAndColor &source : iconSourceList)
        result += '|' + source.first.toFSPathString() + '|' + QString::number(int(source.second));
    return result;
}

QIcon Icon::icon() const
{
    if (m_iconSourceList.isEmpty())
        return QIcon();

    if (m_style == None)
        return QIcon(m_iconSourceList.constFirst().first.toFSPathString());

    // Keep handing out the same QIcon: it is what holds the rendered icon, and
    // its cacheKey() is a key for QPixmapCache entries in
    // StyleHelper::drawIconWithShadow.
    if (m_icon.isNull()) {
        const QList<IconMaskAndColor> sources = m_iconSourceList;
        const IconStyleOptions style = m_style;
        m_icon = QIcon(new ThemedIconEngine([sources, style] {
            return renderIcon(sources, style, qRound(qApp->devicePixelRatio()));
        }));
    }
    return m_icon;
}

QPixmap Icon::pixmap(QIcon::Mode iconMode) const
{
    if (m_iconSourceList.isEmpty())
        return QPixmap();

    if (m_style == None) {
        return QPixmap(
            StyleHelper::dpiSpecificImageFile(m_iconSourceList.constFirst().first.toFSPathString()));
    }

    const int dpr = qRound(qApp->devicePixelRatio());
    // QPixmapCache is cleared on a theme change, so the theme needs no key of its own.
    const QString key = "Utils::Icon::pixmap|" + QString::number(int(iconMode)) + '|'
                        + QString::number(dpr) + '|' + sourceListKey(m_iconSourceList, m_style);
    QPixmap result;
    if (QPixmapCache::find(key, &result))
        return result;

    const MasksAndColors masks = masksAndColors(m_iconSourceList, dpr);
    const QPixmap combinedMask = Utils::combinedMask(masks, m_style);
    result = iconMode == QIcon::Disabled
                 ? maskToColorAndAlpha(combinedMask, creatorColor(Theme::IconsDisabledColor))
                 : masksToIcon(masks, combinedMask, m_style);
    QPixmapCache::insert(key, result);
    return result;
}

FilePath Icon::imageFilePath() const
{
    QTC_ASSERT(m_iconSourceList.length() == 1, return {});
    return m_iconSourceList.first().first;
}

static QIcon renderSideBarIcon(const Icon &classic, const Icon &flat)
{
    QIcon result;
    if (creatorTheme()->flag(Theme::FlatSideBarIcons)) {
        result = flat.icon();
    } else {
        const QPixmap pixmap = classic.pixmap();
        result.addPixmap(pixmap);
        // Ensure that the icon contains a disabled state of that size, since
        // Since we have icons with mixed sizes (e.g. DEBUG_START), and want to
        // avoid that QIcon creates scaled versions of missing QIcon::Disabled
        // sizes.
        result.addPixmap(StyleHelper::disabledSideBarIcon(pixmap), QIcon::Disabled);
    }
    return result;
}

QIcon Icon::sideBarIcon(const Icon &classic, const Icon &flat)
{
    return QIcon(new ThemedIconEngine([classic, flat] {
        return renderSideBarIcon(classic, flat);
    }));
}

static QIcon renderCombinedIcon(const QList<QIcon> &icons)
{
    QIcon result;
    const qreal devicePixelRatio = qApp->devicePixelRatio();
    for (const QIcon &icon: icons)
        for (const QIcon::Mode mode: {QIcon::Disabled, QIcon::Normal})
            for (const QSize &size: icon.availableSizes(mode))
                result.addPixmap(icon.pixmap(size, devicePixelRatio, mode), mode);
    return result;
}

QIcon Icon::combinedIcon(const QList<QIcon> &icons)
{
    return QIcon(new ThemedIconEngine([icons] { return renderCombinedIcon(icons); }));
}

QIcon Icon::combinedIcon(const QList<Icon> &icons)
{
    const QList<QIcon> qIcons = transform(icons, &Icon::icon);
    return combinedIcon(qIcons);
}

void setThemedPixmap(QLabel *label, const Icon &icon)
{
    const auto setPixmap = [label, icon] { label->setPixmap(icon.pixmap()); };
    setPixmap();
    ThemeManager::onChanged(label, themedPixmapKey, setPixmap);
}

QIcon Icon::fromTheme(const QString &name)
{
    static QHash<QString, QIcon> cache;
    static int generation = -1;
    if (generation != ThemeManager::generation()) {
        generation = ThemeManager::generation();
        cache.clear();
    }

    auto found = cache.find(name);
    if (found != cache.end())
        return *found;

    QIcon icon;
    const bool avoidIconFromTheme =
        HostOsInfo::isWindowsHost() // Temporary workaround for QTBUG-140898
        || (HostOsInfo::isLinuxHost()
            && creatorTheme()->colorScheme() != Theme::systemColorScheme());
    if (!avoidIconFromTheme)
        icon = QIcon::fromTheme(name);
    const bool useIconFromTheme = !avoidIconFromTheme && !icon.isNull();
    if (name == "go-next") {
        cache.insert(name, useIconFromTheme ? icon : QIcon(":/utils/images/arrow.png"));
    } else if (name == "document-open") {
        cache.insert(name, useIconFromTheme ? icon : Icons::OPENFILE.icon());
    } else if (name == "edit-copy") {
        cache.insert(name, useIconFromTheme ? icon : Icons::COPY.icon());
    } else if (name == "document-new") {
        cache.insert(name, useIconFromTheme ? icon : Icons::NEWFILE.icon());
    } else if (name == "document-save") {
        cache.insert(name, useIconFromTheme ? icon : Icons::SAVEFILE.icon());
    } else if (name == "document-revert") {
        cache.insert(name, useIconFromTheme ? icon : Icons::UNDO.icon());
    } else if (name == "edit-undo") {
        cache.insert(name, useIconFromTheme ? icon : Icons::UNDO.icon());
    } else if (name == "edit-redo") {
        cache.insert(name, useIconFromTheme ? icon : Icons::REDO.icon());
    } else if (name == "edit-cut") {
        cache.insert(name, useIconFromTheme ? icon : Icons::CUT.icon());
    } else if (name == "edit-paste") {
        cache.insert(name, useIconFromTheme ? icon : Icons::PASTE.icon());
    } else if (name == "zoom-in") {
        cache.insert(name, useIconFromTheme ? icon : Icons::ZOOMIN_TOOLBAR.icon());
    } else if (name == "zoom-out") {
        cache.insert(name, useIconFromTheme ? icon : Icons::ZOOMOUT_TOOLBAR.icon());
    } else if (name == "zoom-original") {
        cache.insert(name, useIconFromTheme ? icon : Icons::EYE_OPEN_TOOLBAR.icon());
    } else if (name == "help-about") {
        cache.insert(name, useIconFromTheme ? icon : Icons::INFO.icon());
    } else if (name == "application-exit") {
        cache.insert(name, useIconFromTheme ? icon : Icons::CLOSE.icon());
    } else if (name == "edit-clear") {
        cache.insert(name, useIconFromTheme ? icon : Icons::EDIT_CLEAR.icon());
    } else if (name == "edit-clear-locationbar-rtl") {
        // KDE has custom icons for this. If these icons are not available we use the freedesktop
        // standard name "edit-clear" before falling back to a bundled resource.
        cache.insert(name, useIconFromTheme ? icon : fromTheme("edit-clear"));
    } else if (name == "edit-clear-locationbar-ltr") {
        cache.insert(name, useIconFromTheme ? icon : fromTheme("edit-clear"));
    } else {
        cache.insert(name, icon);
    }

    return cache[name];
}

} // namespace Utils
