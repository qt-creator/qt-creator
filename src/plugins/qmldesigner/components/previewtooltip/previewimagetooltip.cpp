// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "previewimagetooltip.h"
#include "ui_previewimagetooltip.h"

#include <utils/theme/theme.h>

#include <QtGui/qpixmap.h>

namespace QmlDesigner {

PreviewImageTooltip::PreviewImageTooltip(QWidget *parent)
    : QWidget(parent)
    , m_ui(std::make_unique<Ui::PreviewImageTooltip>())
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowTransparentForInput
                   | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    m_ui->setupUi(this);
    m_ui->nameLabel->setElideMode(Qt::ElideLeft);
    m_ui->pathLabel->setElideMode(Qt::ElideLeft);
    m_ui->infoLabel->setElideMode(Qt::ElideLeft);
    setStyleSheet(QString("QWidget { background-color: %1 }").arg(Utils::creatorTheme()->color(Utils::Theme::BackgroundColorNormal).name()));
}

PreviewImageTooltip::~PreviewImageTooltip() = default;

void PreviewImageTooltip::setName(const QString &name)
{
    m_ui->nameLabel->setText(name);
}

void PreviewImageTooltip::setPath(const QString &path)
{
    m_ui->pathLabel->setText(path);
}

void PreviewImageTooltip::setInfo(const QString &info)
{
    m_ui->infoLabel->setText(info);
}

void PreviewImageTooltip::setImage(const QImage &image, bool scale)
{
    QPixmap pm = QPixmap::fromImage({image});
    if (scale) {
        m_ui->imageLabel->setPixmap(pm.scaled(m_ui->imageLabel->width(),
                                              m_ui->imageLabel->height(),
                                              Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation));
    } else {
        m_ui->imageLabel->setPixmap(pm);
    }
}

}
