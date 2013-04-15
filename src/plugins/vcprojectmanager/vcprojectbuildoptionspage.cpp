#include "vcprojectbuildoptionspage.h"

#include "vcprojectmanagerconstants.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QDialogButtonBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QVBoxLayout>

namespace VcProjectManager {
namespace Internal {

VcProjectEditMsBuildDialog::VcProjectEditMsBuildDialog(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *filePathLayout = new QHBoxLayout;
    m_pathChooser = new QLineEdit;
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_browseButton = new QPushButton(tr("Browse..."));
    filePathLayout->addWidget(m_pathChooser);
    filePathLayout->addWidget(m_browseButton);
    filePathLayout->setStretch(0, 1);
    mainLayout->addLayout(filePathLayout);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(m_browseButton, SIGNAL(clicked()), this, SLOT(showBrowseFileDialog()));
}

VcProjectEditMsBuildDialog::~VcProjectEditMsBuildDialog()
{
}

QString VcProjectEditMsBuildDialog::path() const
{
    return m_pathChooser->text();
}

void VcProjectEditMsBuildDialog::setPath(const QString &path)
{
    m_pathChooser->setText(path);
}

void VcProjectEditMsBuildDialog::showBrowseFileDialog()
{
    m_pathChooser->setText(QFileDialog::getOpenFileName(0, tr("Select Ms Build"), QString(), QLatin1String("*.exe")));
}

VcProjectBuildOptionsWidget::VcProjectBuildOptionsWidget(QWidget *parent) :
    QWidget(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    QVBoxLayout *buttonLayout = new QVBoxLayout;
    m_addMsBuildButton = new QPushButton(tr("Add..."));
    m_editBuildButton = new QPushButton(tr("Edit..."));
    m_editBuildButton->setEnabled(false);
    m_deleteBuildButton = new QPushButton(tr("Delete"));
    m_deleteBuildButton->setEnabled(false);
    buttonLayout->addWidget(m_addMsBuildButton);
    buttonLayout->addWidget(m_editBuildButton);
    buttonLayout->addWidget(m_deleteBuildButton);
    buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    QVBoxLayout *tableLayout = new QVBoxLayout;
    m_buildTableWidget = new QTableWidget();
    m_buildTableWidget->setColumnCount(2);
    QStringList horizontalHeaderLabels;
    horizontalHeaderLabels << tr("Executable") << tr("Version");
    m_buildTableWidget->setHorizontalHeaderLabels(horizontalHeaderLabels);
    m_buildTableWidget->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    m_buildTableWidget->horizontalHeader()->setClickable(false);
    m_buildTableWidget->horizontalHeader()->setMovable(false);
    m_buildTableWidget->horizontalHeader()->setSelectionMode(QAbstractItemView::NoSelection);
    m_buildTableWidget->verticalHeader()->hide();
    m_buildTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableLayout->addWidget(m_buildTableWidget);

    tableLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));

    mainLayout->addLayout(tableLayout);
    mainLayout->addLayout(buttonLayout);

    connect(m_addMsBuildButton, SIGNAL(clicked()), this, SIGNAL(addNewButtonClicked()));
    connect(m_editBuildButton, SIGNAL(clicked()), this, SIGNAL(editButtonClicked()));
    connect(m_deleteBuildButton, SIGNAL(clicked()), this, SIGNAL(deleteButtonClicked()));
    connect(m_buildTableWidget, SIGNAL(cellClicked(int, int)), this, SLOT(onTableRowIndexChange(int)));
}

VcProjectBuildOptionsWidget::~VcProjectBuildOptionsWidget()
{
}

MsBuildInformation VcProjectBuildOptionsWidget::build(int index)
{
    MsBuildInformation msBuild;
    QString exePath;
    QString version;

    if (0 <= index && index < m_buildTableWidget->rowCount()) {
        exePath = m_buildTableWidget->item(index, 0)->text();
        version = m_buildTableWidget->item(index, 1)->text();
    }

    msBuild.m_version = version;
    msBuild.m_executable = exePath;
    return msBuild;
}

int VcProjectBuildOptionsWidget::buildCount()
{
    return m_buildTableWidget->rowCount();
}

MsBuildInformation VcProjectBuildOptionsWidget::currentSelectedBuild() const
{
    QModelIndex currentIndex = m_buildTableWidget->selectionModel()->currentIndex();
    if (currentIndex.isValid()) {
        MsBuildInformation msBuild;
        msBuild.m_executable = m_buildTableWidget->item(currentIndex.row(), 0)->text();
        msBuild.m_version = m_buildTableWidget->item(currentIndex.row(), 1)->text();
        return msBuild;
    }

    return MsBuildInformation();
}

int VcProjectBuildOptionsWidget::currentSelectedRow() const
{
    if (m_buildTableWidget->selectionModel()->currentIndex().isValid())
        return m_buildTableWidget->selectionModel()->currentIndex().row();
    return -1;
}

bool VcProjectBuildOptionsWidget::exists(const QString &exePath)
{
    for (int i = 0; i < m_buildTableWidget->rowCount(); ++i) {
        QTableWidgetItem *item = m_buildTableWidget->item(i, 0);

        if (item->text() == exePath)
            return true;
    }

    return false;
}

void VcProjectBuildOptionsWidget::insertMSBuild(const MsBuildInformation &info)
{
    QTableWidgetItem *exeTableItem = new QTableWidgetItem();
    exeTableItem->setFlags(exeTableItem->flags() ^ Qt::ItemIsEditable);
    exeTableItem->setText(info.m_executable);

    QTableWidgetItem *versionTableItem = new QTableWidgetItem();
    versionTableItem->setFlags(versionTableItem->flags() ^ Qt::ItemIsEditable);
    versionTableItem->setText(info.m_version);

    m_buildTableWidget->insertRow(m_buildTableWidget->rowCount());
    m_buildTableWidget->setItem(m_buildTableWidget->rowCount() - 1, 0, exeTableItem);
    m_buildTableWidget->setItem(m_buildTableWidget->rowCount() - 1, 1, versionTableItem);

    if (!m_buildTableWidget->selectionModel()->currentIndex().isValid())
        m_buildTableWidget->selectRow(0);

    if (!m_editBuildButton->isEnabled())
        m_editBuildButton->setEnabled(true);

    if (!m_deleteBuildButton->isEnabled())
        m_deleteBuildButton->setEnabled(true);
}

void VcProjectBuildOptionsWidget::removeBuild(int index)
{
    if (0 <= index && index < m_buildTableWidget->rowCount())
        m_buildTableWidget->removeRow(index);

    if (m_buildTableWidget->rowCount() <= 0) {
        m_editBuildButton->setEnabled(false);
        m_deleteBuildButton->setEnabled(false);
    }
}

void VcProjectBuildOptionsWidget::updateMsBuild(const QString &exePath, const MsBuildInformation &newMsBuildInfo)
{
    for (int i = 0; i < m_buildTableWidget->rowCount(); ++i) {
        QTableWidgetItem *item = m_buildTableWidget->item(i, 0);

        if (item->text() == exePath) {
            item->setText(newMsBuildInfo.m_executable);

            // update version column item
            item = m_buildTableWidget->item(i, 1);
            item->setText(newMsBuildInfo.m_version);
            break;
        }
    }
}

void VcProjectBuildOptionsWidget::onTableRowIndexChange(int index)
{
    if (0 <= index && index < m_buildTableWidget->rowCount()) {
        m_editBuildButton->setEnabled(true);
        m_deleteBuildButton->setEnabled(true);
    }

    else {
        m_editBuildButton->setEnabled(false);
        m_deleteBuildButton->setEnabled(false);
    }

    emit currentBuildSelectionChanged(index);
}

VcProjectBuildOptionsPage::VcProjectBuildOptionsPage() :
    m_optionsWidget(0)
{
    setId(Core::Id("VcProject.MSBuild"));
    setDisplayName(tr("MS Build"));
    setCategory(Core::Id(VcProjectManager::Constants::VC_PROJECT_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
                                                   VcProjectManager::Constants::VC_PROJECT_SETTINGS_TR_CATEGORY));

    // TODO(Radovan): create and set proper icon
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
    loadSettings();
}

VcProjectBuildOptionsPage::~VcProjectBuildOptionsPage()
{
}

QWidget *VcProjectBuildOptionsPage::createPage(QWidget *parent)
{
    m_optionsWidget = new VcProjectBuildOptionsWidget(parent);

    foreach (const MsBuildInformation *msBuildInfo, m_msBuildInformations)
        m_optionsWidget->insertMSBuild(*msBuildInfo);

    connect(m_optionsWidget, SIGNAL(addNewButtonClicked()), this, SLOT(addNewMsBuild()));
    connect(m_optionsWidget, SIGNAL(editButtonClicked()), this, SLOT(editMsBuild()));
    connect(m_optionsWidget, SIGNAL(deleteButtonClicked()), this, SLOT(deleteMsBuild()));
    return m_optionsWidget;
}

void VcProjectBuildOptionsPage::apply()
{
    if (!m_optionsWidget || (m_optionsWidget && m_optionsWidget->buildCount() < 0))
        return;

    saveSettings();
}

void VcProjectBuildOptionsPage::finish()
{
}

QVector<MsBuildInformation *> VcProjectBuildOptionsPage::msBuilds() const
{
    return m_msBuildInformations;
}

void VcProjectBuildOptionsPage::loadSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(VcProjectManager::Constants::VC_PROJECT_SETTINGS_GROUP));
    QString msBuildInformationData = settings->value(QLatin1String(VcProjectManager::Constants::VC_PROJECT_MS_BUILD_INFORMATIONS)).toString();
    settings->endGroup();

    foreach (MsBuildInformation *msBuild, m_msBuildInformations)
        delete msBuild;

    m_msBuildInformations.clear();

    if (!msBuildInformationData.isEmpty()) {
        QStringList msBuildInformations = msBuildInformationData.split(QLatin1Char(';'));

        foreach (QString msBuildInfo, msBuildInformations) {
            QStringList msBuildData = msBuildInfo.split(QLatin1Char(','));
            MsBuildInformation *msBuild = new MsBuildInformation();
            msBuild->m_executable = msBuildData.first();
            msBuild->m_version = msBuildData.last();
            m_msBuildInformations.append(msBuild);
        }
    }
}

void VcProjectBuildOptionsPage::saveSettings()
{
    if (m_optionsWidget) {
        QString msBuildInformations;

        for (int i = 0; i < m_optionsWidget->buildCount(); ++i) {
            MsBuildInformation msBuildInfo = m_optionsWidget->build(i);
            msBuildInformations += msBuildInfo.m_executable + QLatin1Char(',') + msBuildInfo.m_version;

            if (i != m_optionsWidget->buildCount() - 1)
                msBuildInformations += QLatin1Char(';');
        }

        QSettings *settings = Core::ICore::settings();
        settings->beginGroup(QLatin1String(VcProjectManager::Constants::VC_PROJECT_SETTINGS_GROUP));
        settings->setValue(QLatin1String(VcProjectManager::Constants::VC_PROJECT_MS_BUILD_INFORMATIONS), msBuildInformations);
        settings->endGroup();

        loadSettings();
        emit msBuildInfomationsUpdated();
    }
}

void VcProjectBuildOptionsPage::startVersionCheck()
{
    if (m_validator.m_executable.isEmpty()) {
        return;
    }

    m_validator.m_process = new QProcess();
    connect(m_validator.m_process, SIGNAL(finished(int)),
            this, SLOT(versionCheckFinished()));

    m_validator.m_process->start(m_validator.m_executable, QStringList(QLatin1String("/version")));
    m_validator.m_process->waitForStarted();
}

void VcProjectBuildOptionsPage::addNewMsBuild()
{
    QString newMsBuild = QFileDialog::getOpenFileName(0, tr("Select Ms Build"), QString(), QLatin1String("*.exe"));

    if (m_optionsWidget && !newMsBuild.isEmpty()) {
        if (m_optionsWidget->exists(newMsBuild)) {
            QMessageBox::information(0, tr("Ms Build already present"), tr("Selected MSBuild.exe is already present in the list."));
            return;
        }

        m_validator.m_executable = newMsBuild;
        m_validator.m_requestType = VcProjectValidator::ValidationRequest_Add;
        startVersionCheck();
    }
}

void VcProjectBuildOptionsPage::editMsBuild()
{
    if (m_optionsWidget && m_optionsWidget->currentSelectedRow() != -1) {
        MsBuildInformation currentSelectedMsBuild = m_optionsWidget->currentSelectedBuild();
        m_validator.m_originalExecutable = currentSelectedMsBuild.m_executable;

        VcProjectEditMsBuildDialog editDialog;
        editDialog.setPath(m_validator.m_originalExecutable);

        if (editDialog.exec() == QDialog::Accepted) {
            if (editDialog.path() == m_validator.m_originalExecutable)
                return;

            m_validator.m_requestType = VcProjectValidator::ValidationRequest_Edit;
            m_validator.m_executable = editDialog.path();
            startVersionCheck();
        }
    }
}

void VcProjectBuildOptionsPage::deleteMsBuild()
{
    if (m_optionsWidget && m_optionsWidget->buildCount() > 0)
        m_optionsWidget->removeBuild(m_optionsWidget->currentSelectedRow());
}

void VcProjectBuildOptionsPage::versionCheckFinished()
{
    if (m_validator.m_process) {
        QString response = QVariant(m_validator.m_process->readAll()).toString();
        QStringList splitData = response.split(QLatin1Char('\n'));

        if (m_validator.m_requestType == VcProjectValidator::ValidationRequest_Add) {
            MsBuildInformation newMsBuild;
            newMsBuild.m_executable = m_validator.m_executable;
            newMsBuild.m_version = splitData.last();

            if (m_optionsWidget)
                m_optionsWidget->insertMSBuild(newMsBuild);
        }

        else if (m_validator.m_requestType == VcProjectValidator::ValidationRequest_Edit) {
            foreach (const MsBuildInformation *msBuildInfo, m_msBuildInformations) {
                if (msBuildInfo && msBuildInfo->m_executable == m_validator.m_originalExecutable) {
                    MsBuildInformation newInfo;
                    newInfo.m_version = splitData.last();
                    newInfo.m_executable = m_validator.m_executable;

                    // update table data
                    if (m_optionsWidget)
                        m_optionsWidget->updateMsBuild(m_validator.m_originalExecutable, newInfo);

                    break;
                }
            }
        }

        m_validator.m_process->deleteLater();
        m_validator.m_process = 0;
    }
}

} // namespace Internal
} // namespace VcProjectManager
