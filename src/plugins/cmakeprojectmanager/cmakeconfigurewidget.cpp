#include "cmakeconfigurewidget.h"
#include "cmakeprojectmanager.h"
#include <projectexplorer/environment.h>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

CMakeConfigureWidget::CMakeConfigureWidget(QWidget *parent, CMakeManager *manager, const QString &sourceDirectory)
    : QWidget(parent), m_configureSucceded(false), m_cmakeManager(manager), m_sourceDirectory(sourceDirectory)
{
    m_ui.setupUi(this);
    m_ui.buildDirectoryLineEdit->setPath(sourceDirectory + "/qtcreator-build");

    connect(m_ui.configureButton, SIGNAL(clicked()), this, SLOT(runCMake()));
    // TODO make the configure button do stuff
    // TODO set initial settings
    // TODO note if there's already a build in that directory
    // detect which generators we have
    // let the user select generator
}

QString CMakeConfigureWidget::buildDirectory()
{
    return m_ui.buildDirectoryLineEdit->path();
}

QStringList CMakeConfigureWidget::arguments()
{
    return ProjectExplorer::Environment::parseCombinedArgString(m_ui.cmakeArgumentsLineEdit->text());
}

bool CMakeConfigureWidget::configureSucceded()
{
    return m_configureSucceded;
}

void CMakeConfigureWidget::runCMake()
{
    // TODO run project createCbp()
    // get output and display it

    // TODO analyse wheter this worked out
    m_ui.cmakeOutput->setPlainText(tr("Waiting for cmake..."));
    QString string = m_cmakeManager->createXmlFile(arguments(), m_sourceDirectory, buildDirectory());
    m_ui.cmakeOutput->setPlainText(string);
}

//////
// CMakeConfigureDialog
/////

CMakeConfigureDialog::CMakeConfigureDialog(QWidget *parent, CMakeManager *manager, const QString &sourceDirectory)
    : QDialog(parent)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);
    setLayout(vbox);

    m_cmakeConfigureWidget = new CMakeConfigureWidget(this, manager, sourceDirectory);
    vbox->addWidget(m_cmakeConfigureWidget);

    QHBoxLayout *hboxlayout = new QHBoxLayout(this);
    hboxlayout->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Fixed));


    QPushButton *okButton = new QPushButton(this);
    okButton->setText(tr("Ok"));
    okButton->setDefault(true);
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));

    hboxlayout->addWidget(okButton);
    vbox->addLayout(hboxlayout);
}

QString CMakeConfigureDialog::buildDirectory()
{
    return m_cmakeConfigureWidget->buildDirectory();
}

QStringList CMakeConfigureDialog::arguments()
{
    return m_cmakeConfigureWidget->arguments();
}

bool CMakeConfigureDialog::configureSucceded()
{
    return m_cmakeConfigureWidget->configureSucceded();
}
