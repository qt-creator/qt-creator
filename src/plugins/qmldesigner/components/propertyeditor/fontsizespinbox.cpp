#include "fontsizespinbox.h"

#include <QLineEdit>
#include <QRegExpValidator>

namespace QmlDesigner {

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
