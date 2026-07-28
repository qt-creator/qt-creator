// Copyright header

#include "form.h"
#include "ui_form.h"

Form::Form(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::Form)
{
    m_ui->setupUi(this);
}

Form::~Form()
{
    delete m_ui;
}
