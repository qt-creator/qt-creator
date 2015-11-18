/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
