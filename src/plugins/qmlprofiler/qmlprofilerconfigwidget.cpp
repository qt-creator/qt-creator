/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprofilerconfigwidget.h"
#include "ui_qmlprofilerconfigwidget.h"

namespace QmlProfiler {
namespace Internal {

QmlProfilerConfigWidget::QmlProfilerConfigWidget(QmlProfilerSettings *settings, QWidget *parent) :
    QWidget(parent), m_ui(new Ui::QmlProfilerConfigWidget), m_settings(settings)
{
    m_ui->setupUi(this);
    updateUi();

    connect(m_ui->flushEnabled, &QCheckBox::toggled,
            m_settings, &QmlProfilerSettings::setFlushEnabled);

    connect(m_ui->flushInterval, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            m_settings, &QmlProfilerSettings::setFlushInterval);

    connect(m_ui->aggregateTraces, &QCheckBox::toggled,
            m_settings, &QmlProfilerSettings::setAggregateTraces);

    connect(m_settings, &QmlProfilerSettings::changed, this, &QmlProfilerConfigWidget::updateUi);
}

QmlProfilerConfigWidget::~QmlProfilerConfigWidget()
{
    delete m_ui;
}

void QmlProfilerConfigWidget::updateUi()
{
    m_ui->flushEnabled->setChecked(m_settings->flushEnabled());
    m_ui->flushInterval->setEnabled(m_settings->flushEnabled());
    m_ui->flushInterval->setValue(m_settings->flushInterval());
    m_ui->aggregateTraces->setChecked(m_settings->aggregateTraces());
}

} // Internal
} // QmlProfiler
