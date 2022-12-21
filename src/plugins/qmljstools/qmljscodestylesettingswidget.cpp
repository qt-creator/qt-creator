// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylesettingswidget.h"
#include "qmljscodestylesettings.h"
#include "qmljstoolstr.h"

#include <utils/layoutbuilder.h>

#include <QSpinBox>
#include <QTextStream>

namespace QmlJSTools {

QmlJSCodeStyleSettingsWidget::QmlJSCodeStyleSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_lineLengthSpinBox = new QSpinBox;
    m_lineLengthSpinBox->setMinimum(0);
    m_lineLengthSpinBox->setMaximum(999);

    using namespace Utils::Layouting;
    Column {
        Group {
            title(Tr::tr("Qml JS Code Style")),
            Form {
                Tr::tr("&Line length:"), m_lineLengthSpinBox, br,
            }
        }
    }.attachTo(this, WithoutMargins);

    connect(m_lineLengthSpinBox, &QSpinBox::valueChanged,
            this, &QmlJSCodeStyleSettingsWidget::slotSettingsChanged);
}

void QmlJSCodeStyleSettingsWidget::setCodeStyleSettings(const QmlJSCodeStyleSettings& s)
{
    QSignalBlocker blocker(this);
    m_lineLengthSpinBox->setValue(s.lineLength);
}

QmlJSCodeStyleSettings QmlJSCodeStyleSettingsWidget::codeStyleSettings() const
{
    QmlJSCodeStyleSettings set;

    set.lineLength = m_lineLengthSpinBox->value();

    return set;
}

void QmlJSCodeStyleSettingsWidget::slotSettingsChanged()
{
    emit settingsChanged(codeStyleSettings());
}

} // namespace TextEditor
