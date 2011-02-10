#include "lineeditaction.h"

#include <QLineEdit>
#include <QIntValidator>

namespace QmlDesigner {

LineEditAction::LineEditAction(const QString &placeHolderText, QObject *parent) :
    QWidgetAction(parent),
    m_placeHolderText(placeHolderText)
{
}

QWidget *LineEditAction::createWidget(QWidget *parent)
{
    QLineEdit *lineEdit = new QLineEdit(parent);

    lineEdit->setPlaceholderText(m_placeHolderText);
    lineEdit->setFixedWidth(80);
    lineEdit->setValidator(new QIntValidator(0, 4096, this));

    connect(lineEdit, SIGNAL(textEdited(QString)), this, SIGNAL(textChanged(QString)));

    return lineEdit;
}


} // namespace QmlDesigner
