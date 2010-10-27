#include "maemodeploystepwidget.h"
#include "ui_maemodeploystepwidget.h"

#include "maemodeploystep.h"
#include "maemodeployablelistmodel.h"
#include "maemodeployables.h"
#include "maemodeviceconfiglistmodel.h"
#include "maemorunconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployStepWidget::MaemoDeployStepWidget(MaemoDeployStep *step) :
    ProjectExplorer::BuildStepConfigWidget(),
    ui(new Ui::MaemoDeployStepWidget),
    m_step(step)
{
    ui->setupUi(this);
    ui->tableView->setTextElideMode(Qt::ElideMiddle);
    ui->modelComboBox->setModel(m_step->deployables().data());
    connect(m_step->deployables().data(), SIGNAL(modelAboutToBeReset()),
        SLOT(handleModelListToBeReset()));

    // Queued connection because of race condition with combo box's reaction
    // to modelReset().
    connect(m_step->deployables().data(), SIGNAL(modelReset()),
        SLOT(handleModelListReset()), Qt::QueuedConnection);

    connect(ui->modelComboBox, SIGNAL(currentIndexChanged(int)),
        SLOT(setModel(int)));
    connect(ui->addDesktopFileButton, SIGNAL(clicked()),
        SLOT(addDesktopFile()));
    handleModelListReset();
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
    ui->deployToSysrootCheckBox->setChecked(m_step->isDeployToSysrootEnabled());
    connect(ui->deployToSysrootCheckBox, SIGNAL(toggled(bool)), this,
        SLOT(setDeployToSysroot(bool)));
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
    return tr("<b>Deploy to device</b>: %1").arg(m_step->deviceConfig().name);
}

QString MaemoDeployStepWidget::displayName() const
{
    return QString();
}

void MaemoDeployStepWidget::setCurrentDeviceConfig(int index)
{
    m_step->deviceConfigModel()->setCurrentIndex(index);
}

void MaemoDeployStepWidget::setDeployToSysroot(bool doDeploy)
{
    m_step->setDeployToSysrootEnabled(doDeploy);
}

void MaemoDeployStepWidget::handleModelListToBeReset()
{
    ui->tableView->reset(); // Otherwise we'll crash if the user is currently editing.
    ui->tableView->setModel(0);
    ui->addDesktopFileButton->setEnabled(false);
}

void MaemoDeployStepWidget::handleModelListReset()
{
    QTC_ASSERT(m_step->deployables()->modelCount() == ui->modelComboBox->count(), return);
    if (m_step->deployables()->modelCount() > 0) {
        if (ui->modelComboBox->currentIndex() == -1)
            ui->modelComboBox->setCurrentIndex(0);
        else
            setModel(ui->modelComboBox->currentIndex());
    }
}

void MaemoDeployStepWidget::setModel(int row)
{
    bool canAddDesktopFile = false;
    if (row != -1) {
        MaemoDeployableListModel *const model
            = m_step->deployables()->modelAt(row);
        ui->tableView->setModel(model);
        ui->tableView->resizeRowsToContents();
        canAddDesktopFile = model->canAddDesktopFile();
    }
    ui->addDesktopFileButton->setEnabled(canAddDesktopFile);
}

void MaemoDeployStepWidget::addDesktopFile()
{
    const int modelRow = ui->modelComboBox->currentIndex();
    if (modelRow == -1)
        return;
    MaemoDeployableListModel *const model
        = m_step->deployables()->modelAt(modelRow);
    QString error;
    if (!model->addDesktopFile(error)) {
        QMessageBox::warning(this, tr("Could not create desktop file"),
             tr("Error creating desktop file: %1").arg(error));
    }
    ui->addDesktopFileButton->setEnabled(model->canAddDesktopFile());
    ui->tableView->resizeRowsToContents();
}

} // namespace Internal
} // namespace Qt4ProjectManager
