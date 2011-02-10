#include "lineeditaction.h"

#include <QLineEdit>
#include <QIntValidator>

namespace QmlDesigner {

LineEditAction::LineEditAction(const QString &placeHolderText, QObject *parent) :
    QWidgetAction(parent),
    m_placeHolderText(placeHolderText)
{
}

void LineEditAction::setLineEditText(const QString &text)
{
    emit lineEditTextChange(text);
}

void LineEditAction::clearLineEditText()
{
    emit lineEditTextClear();
}

QWidget *LineEditAction::createWidget(QWidget *parent)
{
    QLineEdit *lineEdit = new QLineEdit(parent);

    lineEdit->setPlaceholderText(m_placeHolderText);
    lineEdit->setFixedWidth(80);
    lineEdit->setValidator(new QIntValidator(0, 4096, this));

    connect(lineEdit, SIGNAL(textEdited(QString)), this, SIGNAL(textChanged(QString)));
    connect(this, SIGNAL(lineEditTextClear()), lineEdit, SLOT(clear()));
    connect(this, SIGNAL(lineEditTextChange(QString)), lineEdit, SLOT(setText(QString)));

    return lineEdit;
}


} // namespace QmlDesigner
