/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "setframevaluedialog.h"
#include "timelinecontrols.h"

#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaType>
#include <QDialogButtonBox>
#include <qnamespace.h>

#include <limits>

namespace QmlDesigner {

SetFrameValueDialog::SetFrameValueDialog(qreal frame, const QVariant &value,
                                         const QString &propertyName, QWidget *parent)
    : QDialog(parent, Qt::Tool)
    , m_valueGetter()
    , m_valueType(value.metaType())
    , m_frameControl(new QSpinBox)
{
    setWindowTitle(tr("Edit Keyframe"));

    auto frameLabelString = QString(tr("Frame"));
    auto labelWidth = fontMetrics().boundingRect(frameLabelString).width();
    if (auto tmp = fontMetrics().boundingRect(propertyName).width(); tmp > labelWidth)
        labelWidth = tmp;

    auto *frameLabel = new QLabel(frameLabelString);
    frameLabel->setAlignment(Qt::AlignRight);
    frameLabel->setFixedWidth(labelWidth);

    auto *valueLabel = new QLabel(propertyName);
    valueLabel->setAlignment(Qt::AlignRight);
    valueLabel->setFixedWidth(labelWidth);

    m_frameControl->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    m_frameControl->setValue(static_cast<int>(frame));
    m_frameControl->setAlignment(Qt::AlignRight);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* frameRow = new QHBoxLayout;
    frameRow->addWidget(frameLabel);
    frameRow->addWidget(m_frameControl);

    auto* valueRow = new QHBoxLayout;
    valueRow->addWidget(valueLabel);
    valueRow->addWidget(createValueControl(value));

    auto* hbox = new QVBoxLayout;
    hbox->addLayout(frameRow);
    hbox->addLayout(valueRow);
    hbox->addStretch();
    hbox->addWidget(buttons);

    setLayout(hbox);
}

SetFrameValueDialog::~SetFrameValueDialog()
{ }

qreal SetFrameValueDialog::frame() const
{
    return static_cast<qreal>(m_frameControl->value());
}

QVariant SetFrameValueDialog::value() const
{
    if (m_valueGetter)
        return m_valueGetter();
    return QVariant(m_valueType);
}

QWidget* SetFrameValueDialog::createValueControl(const QVariant& value)
{
    m_valueType = value.metaType();

    switch (value.metaType().id())
    {

    case QMetaType::QColor: {
        auto* widget = new ColorControl(value.value<QColor>());
        m_valueGetter = [widget]() { return widget->value(); };
        return widget;
    }

    case QMetaType::Bool: {
        auto* widget = new QCheckBox;
        widget->setChecked(value.toBool());
        m_valueGetter = [widget]() { return widget->isChecked(); };
        return widget;
    }

    case QMetaType::Int: {
        auto* widget = new QSpinBox;
        widget->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        widget->setAlignment(Qt::AlignRight);
        widget->setValue(value.toInt());
        m_valueGetter = [widget]() { return widget->value(); };
        return widget;
    }

    case QMetaType::UInt: {
        auto* widget = new QSpinBox;
        widget->setRange(0, std::numeric_limits<int>::max());
        widget->setAlignment(Qt::AlignRight);
        widget->setValue(value.toUInt());
        m_valueGetter = [widget]() { return static_cast<unsigned int>(widget->value()); };
        return widget;
    }

    case QMetaType::Float: {
        auto* widget = new QDoubleSpinBox;
        widget->setRange(std::numeric_limits<float>::min(), std::numeric_limits<float>::max());
        widget->setAlignment(Qt::AlignRight);
        widget->setValue(value.toFloat());
        m_valueGetter = [widget]() { return static_cast<float>(widget->value()); };
        return widget;
    }

    case QMetaType::Double:
        [[fallthrough]];

    default: {
        auto* widget = new QDoubleSpinBox;
        widget->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
        widget->setAlignment(Qt::AlignRight);
        widget->setValue(value.toDouble());
        m_valueGetter = [widget]() { return widget->value(); };
        return widget;
    }

    }

    m_valueGetter = nullptr;
    return nullptr;
}

} // namespace QmlDesigner
