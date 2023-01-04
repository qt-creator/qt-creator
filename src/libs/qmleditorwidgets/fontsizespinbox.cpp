// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fontsizespinbox.h"

#include <QLineEdit>
#include <QRegularExpressionValidator>

namespace QmlEditorWidgets {

FontSizeSpinBox::FontSizeSpinBox(QWidget *parent) :
    QAbstractSpinBox(parent), m_isPointSize(true), m_value(0)
{
    connect(this, &FontSizeSpinBox::editingFinished,
            this, &FontSizeSpinBox::onEditingFinished);
}

void FontSizeSpinBox::stepBy(int steps)
{
    setValue(value() + steps);
}

void FontSizeSpinBox::clear ()
{
    setValue(1);
}

void FontSizeSpinBox::setValue (int val)
{
    if (m_value == val)
        return;

    m_value = val;
    setText();
    emit valueChanged(val);
}

QAbstractSpinBox::StepEnabled FontSizeSpinBox::stepEnabled() const
{
    if (value() > 1)
        return (StepUpEnabled | StepDownEnabled);
    else
        return StepUpEnabled;
}
void FontSizeSpinBox::setText()
{
    QString text = QString::number(m_value);
    if (isPointSize())
        text.append(QLatin1String(" pt"));
    else
        text.append(QLatin1String(" px"));
    lineEdit()->setText(text);

}

void FontSizeSpinBox::onEditingFinished()
{
    QString str = lineEdit()->text();
    if (str.contains(QLatin1String("px"))) {
        setIsPixelSize(true);
        str.remove(QLatin1String("px"));
        setValue(str.toInt());
    } else {
        setIsPointSize(true);
        str.remove(QLatin1String("pt"));
        setValue(str.toInt());
    }
}

QValidator::State FontSizeSpinBox::validate (QString &input, int &p) const
{
    QRegularExpressionValidator v(QRegularExpression(QLatin1String("\\d+\\s*(px|pt)")), nullptr);
    return v.validate(input, p);
}

} //QmlDesigner
