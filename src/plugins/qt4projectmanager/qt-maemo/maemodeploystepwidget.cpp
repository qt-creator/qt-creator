#include "maemodeploystepwidget.h"
#include "ui_maemodeploystepwidget.h"

#include "maemodeploystep.h"
#include "maemodeployablelistmodel.h"
#include "maemodeployablelistwidget.h"
#include "maemodeployables.h"
#include "maemorunconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployStepWidget::MaemoDeployStepWidget(MaemoDeployStep *step) :
    ProjectExplorer::BuildStepConfigWidget(),
    ui(new Ui::MaemoDeployStepWidget),
    m_step(step)
{
    ui->setupUi(this);

    connect(m_step->deployables(), SIGNAL(modelsCreated()), this,
        SLOT(handleModelsCreated()));
    handleModelsCreated();
}

MaemoDeployStepWidget::~MaemoDeployStepWidget()
{
    delete ui;
}

void MaemoDeployStepWidget::init()
{
    const ProjectExplorer::RunConfiguration * const rc =
        m_step->buildConfiguration()->target()->activeRunConfiguration();
    if (rc) {
        connect(qobject_cast<const MaemoRunConfiguration *>(rc),
            SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target *)),
            this, SLOT(handleDeviceUpdate()));
    }
}

void MaemoDeployStepWidget::handleDeviceUpdate()
{
    emit updateSummary();
}

QString MaemoDeployStepWidget::summaryText() const
{
    return tr("<b>Deploy to device</b>: ") + m_step->deviceConfig().name;
}

QString MaemoDeployStepWidget::displayName() const
{
    return QString();
}


void MaemoDeployStepWidget::handleModelsCreated()
{
    ui->tabWidget->clear();
    for (int i = 0; i < m_step->deployables()->modelCount(); ++i) {
        MaemoDeployableListModel * const model
            = m_step->deployables()->modelAt(i);
        ui->tabWidget->addTab(new MaemoDeployableListWidget(this, model),
            model->projectName());
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
