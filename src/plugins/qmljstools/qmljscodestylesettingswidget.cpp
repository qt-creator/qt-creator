// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmljscodestylesettingswidget.h"

#include "qmljscodestylesettings.h"
#include "ui_qmljscodestylesettingswidget.h"

#include <QTextStream>

namespace QmlJSTools {

QmlJSCodeStyleSettingsWidget::QmlJSCodeStyleSettingsWidget(QWidget *parent) :
    QGroupBox(parent),
    ui(new Internal::Ui::QmlJSCodeStyleSettingsWidget)
{
    ui->setupUi(this);

    connect(ui->lineLengthSpinBox, &QSpinBox::valueChanged,
            this, &QmlJSCodeStyleSettingsWidget::slotSettingsChanged);
}

QmlJSCodeStyleSettingsWidget::~QmlJSCodeStyleSettingsWidget()
{
    delete ui;
}

void QmlJSCodeStyleSettingsWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings& s)
{
    QSignalBlocker blocker(this);
    ui->lineLengthSpinBox->setValue(s.lineLength);
}

QmlJSCodeStyleSettings QmlJSCodeStyleSettingsWidget::codeStyleSettings() const
{
    QmlJSCodeStyleSettings set;

    set.lineLength = ui->lineLengthSpinBox->value();

    return set;
}

void QmlJSCodeStyleSettingsWidget::slotSettingsChanged()
{
    emit settingsChanged(codeStyleSettings());
}

} // namespace TextEditor
