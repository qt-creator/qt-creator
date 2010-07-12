#include "maemodeploystepwidget.h"
#include "ui_maemodeploystepwidget.h"

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployStepWidget::MaemoDeployStepWidget() :
    ProjectExplorer::BuildStepConfigWidget(),
    ui(new Ui::MaemoDeployStepWidget)
{
    ui->setupUi(this);
}

MaemoDeployStepWidget::~MaemoDeployStepWidget()
{
    delete ui;
}

void MaemoDeployStepWidget::init()
{
}

QString MaemoDeployStepWidget::summaryText() const
{
    return tr("<b>Deploy to device</b> ");
}

QString MaemoDeployStepWidget::displayName() const
{
    return QString();
}

} // namespace Internal
} // namespace Qt4ProjectManager
