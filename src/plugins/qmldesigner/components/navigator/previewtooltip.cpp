// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "previewtooltip.h"
#include "ui_previewtooltip.h"

#include <utils/theme/theme.h>

#include <QPainter>
#include <QPixmap>

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
    m_ui->imageLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0)");

    static QPixmap checkers;
    if (checkers.isNull()) {
        checkers = {150, 150};
        QPainter painter(&checkers);
        painter.setBrush(QPixmap(":/navigator/icon/checkers.png"));
        painter.drawRect(0, 0, 150, 150);
    }
    m_ui->labelBackground->setPixmap(checkers);

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
