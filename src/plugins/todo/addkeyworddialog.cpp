#include "addkeyworddialog.h"
#include "ui_addkeyworddialog.h"
#include <QColorDialog>


AddKeywordDialog::AddKeywordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddKeywordDialog)
{
    ui->setupUi(this);
    this->ui->listWidget->setViewMode(QListWidget::IconMode);
    this->ui->listWidget->addItem(new QListWidgetItem(QIcon(":/info"), "information"));
    this->ui->listWidget->addItem(new QListWidgetItem(QIcon(":/warning"), "warning"));
    this->ui->listWidget->addItem(new QListWidgetItem(QIcon(":/error"), "error"));
    connect(this->ui->pushButton, SIGNAL(clicked()), this, SLOT(chooseColor()));
}

AddKeywordDialog::~AddKeywordDialog()
{
    delete ui;
}

QString AddKeywordDialog::keywordName()
{
    return this->ui->lineEdit->text();
}

QColor AddKeywordDialog::keywordColor()
{
    return QColor(this->ui->colorEdit->text());
}

QIcon AddKeywordDialog::keywordIcon()
{
    return this->ui->listWidget->currentItem()->icon();
}

void AddKeywordDialog::chooseColor()
{
    QColorDialog *dialog = new QColorDialog(QColor(this->ui->colorEdit->text()),this);
    this->ui->colorEdit->setText(dialog->getColor().name());
}
