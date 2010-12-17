/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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
        text.append(" pt");
    else
        text.append(" px");
    lineEdit()->setText(text);

}

void FontSizeSpinBox::onEditingFinished()
{
    QString str = lineEdit()->text();
    if (str.contains("px")) {
        setIsPixelSize(true);
        str.remove("px");
        setValue(str.toInt());
    } else {
        setIsPointSize(true);
        str.remove("pt");
        setValue(str.toInt());
    }
}

QValidator::State FontSizeSpinBox::validate (QString &input, int &p) const
{
    QRegExp rx("\\d+\\s*(px|pt)");
    QRegExpValidator v(rx, 0);
    return v.validate(input, p);
}

} //QmlDesigner
