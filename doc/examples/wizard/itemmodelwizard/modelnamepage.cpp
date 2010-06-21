#include "modelnamepage.h"
#include "ui_modelnamepage.h"

ModelNamePage::ModelNamePage(QWidget *parent) :
QWizardPage(parent)
{
    setTitle("Enter model class information");
    setSubTitle("The header and source file names will be derived from the class name");
    ui.setupUi(this);
}

ModelNamePage::~ModelNamePage()
{

}

void ModelNamePage::setPath(const QString& path)
{
    this->path = path;
}

void ModelNamePage::on_txtModelClass_textEdited(const QString& txt)
{
    ui.txtHeaderFile->setText(txt + ".h");
    ui.txtImplFile->setText(txt + ".cpp");
}

ModelClassParameters ModelNamePage::parameters() const
{
    ModelClassParameters params;
    params.className = ui.txtModelClass->text();
    params.headerFile = ui.txtHeaderFile->text();

    params.sourceFile = ui.txtImplFile->text();
    params.baseClass = ui.cmbBaseClass->currentText();
    params.path = path;
    return params;
}
