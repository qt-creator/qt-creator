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

#include "previewtooltip.h"
#include "ui_previewtooltip.h"

#include <utils/theme/theme.h>

#include <QtGui/qpixmap.h>

namespace QmlDesigner {

PreviewToolTip::PreviewToolTip(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::PreviewToolTip)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowTransparentForInput
                   | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    m_ui->setupUi(this);
    m_ui->idLabel->setElideMode(Qt::ElideLeft);
    m_ui->typeLabel->setElideMode(Qt::ElideLeft);
    m_ui->infoLabel->setElideMode(Qt::ElideLeft);
    setStyleSheet(QString("QWidget { background-color: %1 }").arg(Utils::creatorTheme()->color(Utils::Theme::BackgroundColorNormal).name()));
}

PreviewToolTip::~PreviewToolTip()
{
    delete m_ui;
}

void PreviewToolTip::setId(const QString &id)
{
    m_ui->idLabel->setText(id);
}

void PreviewToolTip::setType(const QString &type)
{
    m_ui->typeLabel->setText(type);
}

void PreviewToolTip::setInfo(const QString &info)
{
    m_ui->infoLabel->setText(info);
}

void PreviewToolTip::setPixmap(const QPixmap &pixmap)
{
    m_ui->imageLabel->setPixmap(pixmap);
}

QString PreviewToolTip::id() const
{
    return m_ui->idLabel->text();
}

}
