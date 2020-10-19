/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "previewimagetooltip.h"
#include "ui_previewimagetooltip.h"

#include <utils/theme/theme.h>

#include <QtGui/qpixmap.h>

namespace QmlDesigner {

PreviewImageTooltip::PreviewImageTooltip(QWidget *parent)
    : QWidget(parent)
    , m_ui(std::make_unique<Ui::PreviewImageTooltip>())
{
    //  setAttribute(Qt::WA_TransparentForMouseEvents);
    setWindowFlags(Qt::ToolTip);
    m_ui->setupUi(this);
    setStyleSheet(QString("QWidget { background-color: %1 }").arg(Utils::creatorTheme()->color(Utils::Theme::BackgroundColorNormal).name()));
}

PreviewImageTooltip::~PreviewImageTooltip() = default;

void PreviewImageTooltip::setComponentPath(const QString &path)
{
    m_ui->componentPathLabel->setText(path);
}

void PreviewImageTooltip::setComponentName(const QString &name)
{
    m_ui->componentNameLabel->setText(name);
}

void PreviewImageTooltip::setImage(const QImage &image)
{
    resize(image.width() + 20 + m_ui->componentNameLabel->width(),
           std::max(image.height() + 20, height()));
    m_ui->imageLabel->setPixmap(QPixmap::fromImage({image}));
}
}
