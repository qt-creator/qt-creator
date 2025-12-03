// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "previewtooltip.h"
#include "navigatortracing.h"
#include "ui_previewtooltip.h"

#include <utils/theme/theme.h>

#include <QPainter>
#include <QPixmap>

namespace QmlDesigner {

using NavigatorTracing::category;

PreviewToolTip::PreviewToolTip(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::PreviewToolTip)
{
    NanotraceHR::Tracer tracer{"preview tooltip constructor", category()};

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowTransparentForInput
                   | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    m_ui->setupUi(this);
    m_ui->idLabel->setElideMode(Qt::ElideLeft);
    m_ui->typeLabel->setElideMode(Qt::ElideLeft);
    m_ui->infoLabel->setElideMode(Qt::ElideLeft);
    setStyleSheet(QString("QWidget { background-color: %1 }")
                  .arg(Utils::creatorColor(Utils::Theme::BackgroundColorNormal).name()));
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
    NanotraceHR::Tracer tracer{"preview tooltip destructor", category()};

    delete m_ui;
}

void PreviewToolTip::setId(const QString &id)
{
    NanotraceHR::Tracer tracer{"preview tooltip set id", category()};

    m_ui->idLabel->setText(id);
}

void PreviewToolTip::setType(const QString &type)
{
    NanotraceHR::Tracer tracer{"preview tooltip set type", category()};

    m_ui->typeLabel->setText(type);
}

void PreviewToolTip::setInfo(const QString &info)
{
    NanotraceHR::Tracer tracer{"preview tooltip set info", category()};

    m_ui->infoLabel->setText(info);
}

void PreviewToolTip::setPixmap(const QPixmap &pixmap)
{
    NanotraceHR::Tracer tracer{"preview tooltip set pixmap", category()};

    QPixmap scaled = pixmap.scaled(m_ui->labelBackground->size(), Qt::KeepAspectRatio);
    scaled.setDevicePixelRatio(1.);
    m_ui->imageLabel->setPixmap(scaled);
}

QString PreviewToolTip::id() const
{
    NanotraceHR::Tracer tracer{"preview tooltip get id", category()};

    return m_ui->idLabel->text();
}

}
