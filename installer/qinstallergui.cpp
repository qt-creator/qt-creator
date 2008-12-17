/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "qinstallergui.h"

#include "qinstaller.h"
#include "private/qobject_p.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QTimer>

#include <QtGui/QApplication>
#include <QtGui/QCheckBox>
#include <QtGui/QFileDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QTextEdit>
#include <QtGui/QTreeWidget>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>


QT_BEGIN_NAMESPACE

////////////////////////////////////////////////////////////////////
//
// QInstallerGui
//
////////////////////////////////////////////////////////////////////

QInstallerGui::QInstallerGui(QInstaller *installer, QWidget *parent)
  : QWizard(parent)
{
    #ifndef Q_WS_MAC
    setWizardStyle(QWizard::ModernStyle);
    #endif
    setOption(QWizard::IndependentPages);
    connect(button(QWizard::CancelButton), SIGNAL(clicked()),
        this, SLOT(cancelButtonClicked()));

    connect(this, SIGNAL(interrupted()),
        installer, SLOT(interrupt()));
    connect(installer, SIGNAL(installationFinished()),
        this, SLOT(showFinishedPage()));
    connect(installer, SIGNAL(warning(QString)),
        this, SLOT(showWarning(QString)));
}

void QInstallerGui::cancelButtonClicked()
{
    QInstallerPage *page = qobject_cast<QInstallerPage *>(currentPage());
    qDebug() << "CANCEL CLICKED" << currentPage() << page;
    if (page && page->isInterruptible()) {
        QMessageBox::StandardButton bt = QMessageBox::warning(this,
            tr("Warning"),
            tr("Do you want to abort the installation process?"),
                QMessageBox::Yes | QMessageBox::No);
        if (bt == QMessageBox::Yes)
            emit interrupted();
    } else {
        QMessageBox::StandardButton bt = QMessageBox::warning(this,
            tr("Warning"),
            tr("Do you want to abort the installer application?"),
                QMessageBox::Yes | QMessageBox::No);
        if (bt == QMessageBox::Yes)
            QDialog::reject();
    }
}

void QInstallerGui::reject()
{}

void QInstallerGui::showFinishedPage()
{
    qDebug() << "SHOW FINISHED PAGE";
    next();
}

void QInstallerGui::showWarning(const QString &msg)
{
    QMessageBox::warning(this, tr("Warning"), msg);
}


////////////////////////////////////////////////////////////////////
//
// QInstallerPage
//
////////////////////////////////////////////////////////////////////

QInstallerPage::QInstallerPage(QInstaller *installer)
  : m_installer(installer), m_fresh(true)
{
    setSubTitle(QString(" ")); // otherwise the colors will screw up

}

QInstaller *QInstallerPage::installer() const
{
    return m_installer;
}

QPixmap QInstallerPage::watermarkPixmap() const
{
    return QPixmap(m_installer->value("WatermarkPixmap"));
}

QPixmap QInstallerPage::logoPixmap() const
{
    return QPixmap(m_installer->value("LogoPixmap"));
}

QString QInstallerPage::productName() const
{
    return m_installer->value("ProductName");
}

void QInstallerPage::insertWidget(QWidget *widget, const QString &siblingName, int offset)
{
    QWidget *sibling = findChild<QWidget *>(siblingName);
    QWidget *parent = sibling ? sibling->parentWidget() : 0;
    QLayout *layout = parent ? parent->layout() : 0;
    QBoxLayout *blayout = qobject_cast<QBoxLayout *>(layout);
    //qDebug() << "FOUND: " << sibling << parent << layout << blayout;
    if (blayout) {
        int index = blayout->indexOf(sibling) + offset;
        blayout->insertWidget(index, widget);
    }
}

QWidget *QInstallerPage::findWidget(const QString &objectName) const
{
    return findChild<QWidget *>(objectName);
}

void QInstallerPage::setVisible(bool visible)
{   
    QWizardPage::setVisible(visible);
    qApp->processEvents();
    //qDebug() << "VISIBLE: " << visible << objectName() << installer(); 
    if (m_fresh && !visible) {
        //qDebug() << "SUPRESSED...";
        m_fresh = false;
        return;
    }
    if (visible)
        entering();
    else
        leaving();
}

int QInstallerPage::nextId() const
{
    //qDebug() << "NEXTID";
    return QWizardPage::nextId();
}


////////////////////////////////////////////////////////////////////
//
// QInstallerIntroductionPage
//
////////////////////////////////////////////////////////////////////

QInstallerIntroductionPage::QInstallerIntroductionPage(QInstaller *installer)
  : QInstallerPage(installer)
{
    setObjectName("IntroductionPage");
    setTitle(tr("Setup - %1").arg(productName()));
    setPixmap(QWizard::WatermarkPixmap, watermarkPixmap());
    setSubTitle(QString());

    QLabel *msgLabel = new QLabel(this);
    msgLabel->setObjectName("MessageLabel");
    msgLabel->setWordWrap(true);
    msgLabel->setText(QInstaller::tr("Welcome to the %1 Setup Wizard.")
        .arg(productName()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(msgLabel);
    setLayout(layout);
}


////////////////////////////////////////////////////////////////////
//
// QInstallerLicenseAgreementPage
//
////////////////////////////////////////////////////////////////////

QInstallerLicenseAgreementPage::QInstallerLicenseAgreementPage(QInstaller *installer)
  : QInstallerPage(installer)
{
    setObjectName("LicenseAgreementPage");
    setTitle(tr("License Agreement"));
    QString msg = tr("Please read the following License Agreement. "
        "You must accept the terms of this agreement "
        "before continuing with the installation.");
    setPixmap(QWizard::LogoPixmap, logoPixmap());
    setPixmap(QWizard::WatermarkPixmap, QPixmap());

    QTextEdit *textEdit = new QTextEdit(this);
    textEdit->setObjectName("LicenseText");
    QFile file(":/resources/license.txt");
    file.open(QIODevice::ReadOnly);
    textEdit->setText(file.readAll());

    m_acceptRadioButton = new QRadioButton(tr("I accept the agreement"), this);
    m_rejectRadioButton = new QRadioButton(tr("I do not accept the agreement"), this);

    QLabel *msgLabel = new QLabel(msg, this);
    msgLabel->setObjectName("MessageLabel");
    msgLabel->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(msgLabel);
    layout->addWidget(textEdit);
        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->addWidget(new QLabel(tr("Do you accept this License?")));
            QVBoxLayout *vlayout = new QVBoxLayout;
            vlayout->addWidget(m_acceptRadioButton);
            vlayout->addWidget(m_rejectRadioButton);
        hlayout->addLayout(vlayout);
    layout->addLayout(hlayout);
    setLayout(layout);
    connect(m_acceptRadioButton, SIGNAL(toggled(bool)),
        this, SIGNAL(completeChanged()));
    connect(m_rejectRadioButton, SIGNAL(toggled(bool)),
        this, SIGNAL(completeChanged()));
}

bool QInstallerLicenseAgreementPage::isComplete() const
{
    return m_acceptRadioButton->isChecked();
}


////////////////////////////////////////////////////////////////////
//
// QInstallerComponentSelectionPage
//
////////////////////////////////////////////////////////////////////

static QString niceSizeText(const QString &str)
{
    qint64 size = str.toInt();
    QString msg = QInstallerComponentSelectionPage::tr(
        "This component will occupy approximately %1 %2 on your harddisk.");
    if (size < 10000)
        return msg.arg(size).arg("Bytes");
    if (size < 1024 * 10000)
        return msg.arg(size / 1024).arg("kBytes");
    return msg.arg(size / 1024 / 1024).arg("MBytes");
}

class QInstallerComponentSelectionPage::Private : public QObject
{
    Q_OBJECT

public:
    Private(QInstallerComponentSelectionPage *q_, QInstaller *installer)
      : q(q_), m_installer(installer)
    {
        m_treeView = new QTreeWidget(q);
        m_treeView->setObjectName("TreeView");
        m_treeView->setMouseTracking(true);
        m_treeView->header()->hide();

        for (int i = 0; i != installer->componentCount(); ++i) {
            QInstallerComponent *component = installer->component(i);
            QTreeWidgetItem *item = new QTreeWidgetItem(m_treeView);
            item->setText(0, component->value("Name"));
            item->setToolTip(0, component->value("Description"));
            item->setToolTip(1, niceSizeText(component->value("UncompressedSize")));
            //QString current = component->value("CurrentState");
            QString suggested = component->value("SuggestedState");
            if (suggested == "Uninstalled") {
                item->setCheckState(0, Qt::Unchecked);
            } else if (suggested == "AlwaysInstalled") {
                item->setCheckState(0, Qt::PartiallyChecked);
                item->setFlags(item->flags() & ~Qt::ItemIsUserCheckable);
            } else { //if (suggested == "Installed")
                item->setCheckState(0, Qt::Checked);
            }
        }
        
        m_descriptionLabel = new QLabel(q);
        m_descriptionLabel->setWordWrap(true);

        m_sizeLabel = new QLabel(q);
        m_sizeLabel->setWordWrap(true);

        QVBoxLayout *layout = new QVBoxLayout(q);
        //layout->addWidget(msgLabel);
            QHBoxLayout *hlayout = new QHBoxLayout;
            hlayout->addWidget(m_treeView, 3);
                QVBoxLayout *vlayout = new QVBoxLayout;
                vlayout->addWidget(m_descriptionLabel);
                vlayout->addWidget(m_sizeLabel);
                vlayout->addSpacerItem(new QSpacerItem(1, 1,
                    QSizePolicy::MinimumExpanding, 
                    QSizePolicy::MinimumExpanding));
            hlayout->addLayout(vlayout, 2);
        layout->addLayout(hlayout);
        q->setLayout(layout);

        connect(m_treeView,
            SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    }

public slots:
    void currentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *)
    {
        m_descriptionLabel->setText(item->toolTip(0));
        m_sizeLabel->setText(item->toolTip(1));
    }

public:
    QInstallerComponentSelectionPage *q;
    QInstaller *m_installer;
    QTreeWidget *m_treeView;
    QLabel *m_descriptionLabel;
    QLabel *m_sizeLabel;
};

 
QInstallerComponentSelectionPage::QInstallerComponentSelectionPage
    (QInstaller *installer)
  : QInstallerPage(installer), d(new Private(this, installer))
{
    setObjectName("ComponentSelectionPage");
    setTitle(tr("Select Components"));
    QString msg = tr("Please select the components you want to install.");
    setSubTitle(msg);
    setPixmap(QWizard::LogoPixmap, logoPixmap());
    setPixmap(QWizard::WatermarkPixmap, QPixmap());
}

QInstallerComponentSelectionPage::~QInstallerComponentSelectionPage()
{
    delete d;
}

void QInstallerComponentSelectionPage::leaving()
{
    int n = d->m_treeView->topLevelItemCount();
    for (int i = 0; i != n; ++i) {
        QTreeWidgetItem *item = d->m_treeView->topLevelItem(i);
        QInstallerComponent *component = installer()->component(i);
        if (item->checkState(0) == Qt::Unchecked)
            component->setValue("WantedState", "Uninstalled");
        else 
            component->setValue("WantedState", "Installed");
    }
}


////////////////////////////////////////////////////////////////////
//
// QInstallerTargetDirectoryPage
//
////////////////////////////////////////////////////////////////////

QInstallerTargetDirectoryPage::QInstallerTargetDirectoryPage(QInstaller *installer)
  : QInstallerPage(installer)
{
    setObjectName("TargetDirectoryPage");
    setTitle(tr("Installation Directory"));
    setPixmap(QWizard::LogoPixmap, logoPixmap());
    setPixmap(QWizard::WatermarkPixmap, QPixmap());

    QLabel *msgLabel = new QLabel(this);
    msgLabel->setText(QInstaller::tr("Please specify the directory where %1 "
        "will be installed.").arg(productName()));
    msgLabel->setWordWrap(true);
    msgLabel->setObjectName("MessageLabel");

    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setObjectName("LineEdit");

    QPushButton *browseButton = new QPushButton(this);
    browseButton->setObjectName("BrowseButton");
    browseButton->setText("Browse...");
    browseButton->setIcon(QIcon(logoPixmap()));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(msgLabel);
        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->addWidget(m_lineEdit);
        hlayout->addWidget(browseButton);
    layout->addLayout(hlayout);
    setLayout(layout);

    QString targetDir = installer->value("TargetDir");
    //targetDir = QDir::currentPath();
    if (targetDir.isEmpty())
        targetDir = QDir::homePath() + QDir::separator() + productName();
    m_lineEdit->setText(targetDir);

    connect(browseButton, SIGNAL(clicked()),
        this, SLOT(dirRequested()));
    connect(m_lineEdit, SIGNAL(textChanged(QString)),
        this, SIGNAL(completeChanged()));
}

QString QInstallerTargetDirectoryPage::targetDir() const
{
    return m_lineEdit->text();
}

void QInstallerTargetDirectoryPage::setTargetDir(const QString &dirName)
{
    m_lineEdit->setText(dirName);
}

void QInstallerTargetDirectoryPage::entering()
{
    connect(wizard(), SIGNAL(customButtonClicked(int)),
            this, SLOT(targetDirSelected()));
}

void QInstallerTargetDirectoryPage::leaving()
{
    installer()->setValue("TargetDir", targetDir());
    disconnect(wizard(), SIGNAL(customButtonClicked(int)),
               this, SLOT(targetDirSelected()));
}

void QInstallerTargetDirectoryPage::targetDirSelected()
{
    //qDebug() << "TARGET DIRECTORY";
    QDir dir(targetDir());
    if (dir.exists() && dir.isReadable()) {
        QMessageBox::StandardButton bt = QMessageBox::warning(this,
            tr("Warning"),
            tr("The directory you slected exists already.\n"
               "Do you want to continue?"),
                QMessageBox::Yes | QMessageBox::No);
        if (bt == QMessageBox::Yes)
            wizard()->next();
        return;
    }
    dir.cdUp();
    if (dir.exists() && dir.isReadable()) {
        wizard()->next();
        return;
    }
    wizard()->next();
}

void QInstallerTargetDirectoryPage::dirRequested()
{
    //qDebug() << "DIR REQUESTED";
    QString newDirName = QFileDialog::getExistingDirectory(this,
        tr("Select Installation Directory"), targetDir()
        /*, Options options = ShowDirsOnly*/);
    if (newDirName.isEmpty() || newDirName == targetDir())
        return;
    m_lineEdit->setText(newDirName);
}


////////////////////////////////////////////////////////////////////
//
// QInstallerReadyForInstallationPage
//
////////////////////////////////////////////////////////////////////

QInstallerReadyForInstallationPage::
    QInstallerReadyForInstallationPage(QInstaller *installer)
  : QInstallerPage(installer)
{
    setObjectName("ReadyForInstallationPage");
    setTitle(tr("Ready to Install"));
    setCommitPage(true);
    setButtonText(QWizard::CommitButton, tr("Install"));

    QLabel *msgLabel = new QLabel(this);
    msgLabel->setObjectName("MessageLabel");
    msgLabel->setText(QInstaller::tr("Setup is now ready to begin installing %1 "
        "on your computer.").arg(productName()));

    QLayout *layout = new QVBoxLayout(this);
    layout->addWidget(msgLabel);
    setLayout(layout);
}


////////////////////////////////////////////////////////////////////
//
// QInstallerPerformInstallationPage
//
////////////////////////////////////////////////////////////////////

QInstallerPerformInstallationPage::QInstallerPerformInstallationPage(QInstaller *gui)
  : QInstallerPage(gui)
{
    setObjectName("InstallationPage");
    setTitle(tr("Installing %1").arg(installer()->value("ProductName")));
    setCommitPage(true);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName("ProgressBar");
    m_progressBar->setRange(1, 100);

    m_progressLabel = new QLabel(this);
    m_progressLabel->setObjectName("ProgressLabel");
    
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, SIGNAL(timeout()),
            this, SLOT(updateProgress()));
    m_updateTimer->setInterval(50);

    QLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_progressLabel);
    setLayout(layout);

    connect(installer(), SIGNAL(installationStarted()),
            this, SLOT(installationStarted()));
    connect(installer(), SIGNAL(installationFinished()),
            this, SLOT(installationFinished()));
}

void QInstallerPerformInstallationPage::initializePage()
{
    QWizardPage::initializePage();
    QTimer::singleShot(30, installer(), SLOT(runInstaller()));
}

// FIXME: remove function
bool QInstallerPerformInstallationPage::isComplete() const
{
    return true;
}

void QInstallerPerformInstallationPage::installationStarted()
{
    qDebug() << "INSTALLATION STARTED";
    m_updateTimer->start();
    updateProgress();
}

void QInstallerPerformInstallationPage::installationFinished()
{
    qDebug() << "INSTALLATION FINISHED";
    m_updateTimer->stop();
    updateProgress();
}

void QInstallerPerformInstallationPage::updateProgress()
{
    int progress = installer()->installationProgress();
    if (progress != m_progressBar->value())
        m_progressBar->setValue(progress);
    QString progressText = installer()->installationProgressText();
    if (progressText != m_progressLabel->text())
        m_progressLabel->setText(progressText);
}


////////////////////////////////////////////////////////////////////
//
// QInstallerPerformUninstallationPage
//
////////////////////////////////////////////////////////////////////

QInstallerPerformUninstallationPage::QInstallerPerformUninstallationPage
        (QInstaller *gui)
  : QInstallerPage(gui)
{
    setObjectName("UninstallationPage");
    setTitle(tr("Uninstalling %1").arg(installer()->value("ProductName")));
    setCommitPage(true);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName("ProgressBar");
    m_progressBar->setRange(1, 100);

    m_progressLabel = new QLabel(this);
    m_progressLabel->setObjectName("ProgressLabel");
    
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, SIGNAL(timeout()),
            this, SLOT(updateProgress()));
    m_updateTimer->setInterval(50);

    QLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_progressLabel);
    setLayout(layout);

    connect(installer(), SIGNAL(uninstallationStarted()),
            this, SLOT(uninstallationStarted()));
    connect(installer(), SIGNAL(uninstallationFinished()),
            this, SLOT(uninstallationFinished()));
}

void QInstallerPerformUninstallationPage::initializePage()
{
    QWizardPage::initializePage();
    QTimer::singleShot(30, installer(), SLOT(runUninstaller()));
}

// FIXME: remove function
bool QInstallerPerformUninstallationPage::isComplete() const
{
    return true;
}

void QInstallerPerformUninstallationPage::uninstallationStarted()
{
    m_updateTimer->start();
    updateProgress();
}

void QInstallerPerformUninstallationPage::uninstallationFinished()
{
    m_updateTimer->stop();
    updateProgress();
}

void QInstallerPerformUninstallationPage::updateProgress()
{
    int progress = installer()->installationProgress();
    if (progress != m_progressBar->value())
        m_progressBar->setValue(progress);
    QString progressText = installer()->installationProgressText();
    if (progressText != m_progressLabel->text())
        m_progressLabel->setText(progressText);
}


////////////////////////////////////////////////////////////////////
//
// QInstallerFinishedPage
//
////////////////////////////////////////////////////////////////////

QInstallerFinishedPage::QInstallerFinishedPage(QInstaller *installer)
  : QInstallerPage(installer)
{
    setObjectName("FinishedPage");
    setTitle(tr("Completing the %1 Setup Wizard").arg(productName()));
    setPixmap(QWizard::WatermarkPixmap, watermarkPixmap());
    setSubTitle(QString());

    QLabel *msgLabel = new QLabel(this);
    msgLabel->setObjectName("MessageLabel");
    msgLabel->setWordWrap(true);
    msgLabel->setText(tr("Click Finish to exit the Setup Wizard"));

    m_runItCheckBox = new QCheckBox(this);
    m_runItCheckBox->setObjectName("RunItCheckBox");
    m_runItCheckBox->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(msgLabel);
    if (m_runItCheckBox)
        layout->addWidget(m_runItCheckBox);
    setLayout(layout);
}

void QInstallerFinishedPage::entering()
{
    qDebug() << "FINISHED ENTERING: ";
    connect(wizard()->button(QWizard::FinishButton), SIGNAL(clicked()),
        this, SLOT(handleFinishClicked()));
    if (installer()->status() == QInstaller::InstallerSucceeded) {
        m_runItCheckBox->show();
        m_runItCheckBox->setText(tr("Run %1 now.").arg(productName()));
    } else {
        setTitle(tr("The %1 Setup Wizard failed").arg(productName()));
        m_runItCheckBox->hide();
        m_runItCheckBox->setChecked(false);
    }
}

void QInstallerFinishedPage::leaving()
{
    disconnect(wizard(), SIGNAL(customButtonClicked(int)),
        this, SLOT(handleFinishClicked()));
}

void QInstallerFinishedPage::handleFinishClicked()
{
    if (!m_runItCheckBox->isChecked())
        return;
    QString program = installer()->value("RunProgram");
    if (program.isEmpty())
        return;
    program = installer()->replaceVariables(program);
    qDebug() << "STARTING " << program;
    QProcess *process = new QProcess;
    process->start(program);
    process->waitForFinished();
}

#include "qinstallergui.moc"

QT_END_NAMESPACE
