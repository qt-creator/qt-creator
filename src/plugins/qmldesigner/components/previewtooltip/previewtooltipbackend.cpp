/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "previewtooltipbackend.h"

#include "previewimagetooltip.h"

#include <coreplugin/icore.h>
#include <imagecache.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QMetaObject>

namespace QmlDesigner {

PreviewTooltipBackend::PreviewTooltipBackend(ImageCache &cache)
    : m_cache{cache}
{}

PreviewTooltipBackend::~PreviewTooltipBackend()
{
    hideTooltip();
}

void PreviewTooltipBackend::showTooltip()
{
    if (m_componentPath.isEmpty())
        return;

    m_tooltip = std::make_unique<PreviewImageTooltip>();

    m_tooltip->setComponentName(m_componentName);
    m_tooltip->setComponentPath(m_componentPath);

    m_cache.requestImage(
        m_componentPath,
        [tooltip = QPointer<PreviewImageTooltip>(m_tooltip.get())](const QImage &image) {
            QMetaObject::invokeMethod(tooltip, [tooltip, image] {
                if (tooltip)
                    tooltip->setImage(image);
            });
        },
        [] {});

    auto desktopWidget = QApplication::desktop();
    auto mousePosition = desktopWidget->cursor().pos();

    mousePosition += {20, 20};
    m_tooltip->move(mousePosition);
    m_tooltip->show();
}

void PreviewTooltipBackend::hideTooltip()
{
    if (m_tooltip)
        m_tooltip->hide();

    m_tooltip.reset();
}

QString QmlDesigner::PreviewTooltipBackend::componentPath() const
{
    return m_componentPath;
}

void QmlDesigner::PreviewTooltipBackend::setComponentPath(const QString &path)
{
    m_componentPath = path;

    if (m_componentPath != path)
        emit componentPathChanged();
}

QString QmlDesigner::PreviewTooltipBackend::componentName() const
{
    return m_componentName;
}

void QmlDesigner::PreviewTooltipBackend::setComponentName(const QString &name)
{
    m_componentName = name;

    if (m_componentName != name)
        emit componentNameChanged();
}

} // namespace QmlDesigner
