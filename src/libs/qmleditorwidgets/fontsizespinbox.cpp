/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "fontsizespinbox.h"

#include <QLineEdit>
#include <QRegExpValidator>

namespace QmlEditorWidgets {

FontSizeSpinBox::FontSizeSpinBox(QWidget *parent) :
    QAbstractSpinBox(parent), m_isPointSize(true), m_value(0)
{
    connect(this, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
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
    QRegExp rx(QLatin1String("\\d+\\s*(px|pt)"));
    QRegExpValidator v(rx, 0);
    return v.validate(input, p);
}

} //QmlDesigner
