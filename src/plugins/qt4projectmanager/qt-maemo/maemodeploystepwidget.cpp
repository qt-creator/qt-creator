#include "maemodeploystepwidget.h"
#include "ui_maemodeploystepwidget.h"

#include "maemodeploystep.h"
#include "maemodeployablelistmodel.h"
#include "maemodeployablelistwidget.h"
#include "maemodeployables.h"
#include "maemodeviceconfiglistmodel.h"
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
    handleDeviceConfigModelChanged();
    connect(m_step->buildConfiguration()->target(),
        SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
        this, SLOT(handleDeviceConfigModelChanged()));
    connect(ui->deviceConfigComboBox, SIGNAL(activated(int)), this,
        SLOT(setCurrentDeviceConfig(int)));
#ifdef DEPLOY_VIA_MOUNT
    ui->deployToSysrootCheckBox->setChecked(m_step->deployToSysroot());
    connect(ui->deployToSysrootCheckBox, SIGNAL(toggled(bool)), this,
        SLOT(setDeployToSysroot(bool)));
#else
    ui->deployToSysrootCheckBox->hide();
#endif
    handleDeviceConfigModelChanged();
}

void MaemoDeployStepWidget::handleDeviceConfigModelChanged()
{
    const MaemoDeviceConfigListModel * const oldModel
        = qobject_cast<MaemoDeviceConfigListModel *>(ui->deviceConfigComboBox->model());
    if (oldModel)
        disconnect(oldModel, 0, this, 0);
    MaemoDeviceConfigListModel * const devModel = m_step->deviceConfigModel();
    ui->deviceConfigComboBox->setModel(devModel);
    connect(devModel, SIGNAL(currentChanged()), this,
        SLOT(handleDeviceUpdate()));
    connect(devModel, SIGNAL(modelReset()), this,
        SLOT(handleDeviceUpdate()));
    handleDeviceUpdate();
}

void MaemoDeployStepWidget::handleDeviceUpdate()
{
    ui->deviceConfigComboBox->setCurrentIndex(m_step->deviceConfigModel()
        ->currentIndex());
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

void MaemoDeployStepWidget::setCurrentDeviceConfig(int index)
{
    m_step->deviceConfigModel()->setCurrentIndex(index);
}

void MaemoDeployStepWidget::setDeployToSysroot(bool doDeploy)
{
#ifdef DEPLOY_VIA_MOUNT
    m_step->setDeployToSysroot(doDeploy);
#else
    Q_UNUSED(doDeploy)
#endif
}

} // namespace Internal
} // namespace Qt4ProjectManager
