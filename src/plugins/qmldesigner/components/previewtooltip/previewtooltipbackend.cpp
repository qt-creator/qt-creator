// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "previewtooltipbackend.h"

#include "previewimagetooltip.h"

#include <coreplugin/icore.h>
#include <asynchronousimagecache.h>

#include <QApplication>
#include <QMetaObject>
#include <QScreen>

namespace QmlDesigner {

PreviewTooltipBackend::PreviewTooltipBackend(AsynchronousImageCache &cache)
    : m_cache{cache}
{}

PreviewTooltipBackend::~PreviewTooltipBackend()
{
    hideTooltip();
}

void PreviewTooltipBackend::showTooltip()
{
    m_tooltip = std::make_unique<PreviewImageTooltip>();

    m_tooltip->setName(m_name);
    m_tooltip->setPath(m_path);
    m_tooltip->setInfo(m_info);

    m_cache.requestImage(
        Utils::PathString{m_path},
        [tooltip = QPointer<PreviewImageTooltip>(m_tooltip.get()),
         scaleImage = m_scaleImage](const QImage &image) {
            QMetaObject::invokeMethod(tooltip, [tooltip, image, scaleImage] {
                if (tooltip) {
                    tooltip->setImage(image, scaleImage);
                    tooltip->show();
                }
            });
        },
        [](auto) {},
        Utils::PathString{m_extraId},
        m_auxiliaryData);

    reposition();
}

void PreviewTooltipBackend::hideTooltip()
{
    if (m_tooltip)
        m_tooltip->hide();

    m_tooltip.reset();
}

bool PreviewTooltipBackend::isVisible() const
{
    if (m_tooltip)
        return m_tooltip->isVisible();
    return false;
}

void PreviewTooltipBackend::reposition()
{
    if (m_tooltip) {
        // By default tooltip is placed right and below the cursor, but if the screen
        // doesn't have sufficient space in that direction, position is adjusted.
        // It is assumed that some diagonal direction will have enough space.
        const QPoint mousePos = QCursor::pos();
        QScreen *screen = qApp->screenAt(mousePos);
        QRect tipRect = m_tooltip->geometry();
        QPoint offset(10, 5);
        QPoint pos = mousePos + offset;
        if (screen) {
            QRect rect = screen->geometry();
            tipRect.moveTo(pos);
            if (!rect.contains(tipRect)) {
                pos = mousePos + QPoint(-offset.x() - m_tooltip->size().width(),
                                        offset.y());
                tipRect.moveTo(pos);
                if (!rect.contains(tipRect)) {
                    pos = mousePos + QPoint(offset.x(), -offset.y() - m_tooltip->size().height());
                    tipRect.moveTo(pos);
                    if (!rect.contains(tipRect)) {
                        pos = mousePos + QPoint(-offset.x() - m_tooltip->size().width(),
                                                -offset.y() - m_tooltip->size().height());
                        tipRect.moveTo(pos);
                    }
                }
            }
        }

        m_tooltip->move(pos);
    }
}

QString PreviewTooltipBackend::name() const
{
    return m_name;
}

void PreviewTooltipBackend::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }
}

QString PreviewTooltipBackend::path() const
{
    return m_path;
}

void PreviewTooltipBackend::setPath(const QString &path)
{
    if (m_path != path) {
        m_path = path;
        emit pathChanged();
    }
}

QString PreviewTooltipBackend::info() const
{
    return m_info;
}

void PreviewTooltipBackend::setInfo(const QString &info)
{
    if (m_info != info) {
        m_info = info;
        emit infoChanged();
    }
}

QString PreviewTooltipBackend::extraId() const
{
    return m_extraId;
}

// Sets the imageCache extraId hint. Valid content depends on image cache collector used.
void PreviewTooltipBackend::setExtraId(const QString &extraId)
{
    if (m_extraId != extraId) {
        m_extraId = extraId;
        emit extraIdChanged();
    }
}

bool PreviewTooltipBackend::scaleImage() const
{
    return m_scaleImage;
}

void PreviewTooltipBackend::setScaleImage(bool scale)
{
    if (m_scaleImage != scale) {
        m_scaleImage = scale;
        emit scaleImageChanged();
    }
}

} // namespace QmlDesigner
