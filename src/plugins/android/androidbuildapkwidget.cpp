/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "androidbuildapkwidget.h"

#include "androidbuildapkstep.h"
#include "androidconfigurations.h"
#include "androidextralibrarylistmodel.h"
#include "androidcreatekeystorecertificate.h"
#include "androidmanager.h"
#include "androidsdkmanager.h"
#include "createandroidmanifestwizard.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/infolabel.h>
#include <utils/fancylineedit.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

AndroidBuildApkWidget::AndroidBuildApkWidget(AndroidBuildApkStep *step)
    : BuildStepConfigWidget(step), m_step(step)
{
    setDisplayName("<b>" + tr("Build Android APK") + "</b>");
    setSummaryText(displayName());

    auto vbox = new QVBoxLayout(this);
    vbox->addWidget(createSignPackageGroup());
    vbox->addWidget(createApplicationGroup());
    vbox->addWidget(createAdvancedGroup());
    vbox->addWidget(createCreateTemplatesGroup());
    vbox->addWidget(createAdditionalLibrariesGroup());

    connect(m_step->buildConfiguration(), &BuildConfiguration::buildTypeChanged,
            this, &AndroidBuildApkWidget::updateSigningWarning);

    connect(m_signPackageCheckBox, &QAbstractButton::clicked,
            m_addDebuggerCheckBox, &QWidget::setEnabled);

    signPackageCheckBoxToggled(m_step->signPackage());
    updateSigningWarning();
}

QWidget *AndroidBuildApkWidget::createApplicationGroup()
{
    const int minApiSupported = AndroidManager::apiLevelRange().first;
    QStringList targets = AndroidConfig::apiLevelNamesFor(AndroidConfigurations::sdkManager()->
                                                          filteredSdkPlatforms(minApiSupported));
    targets.removeDuplicates();

    auto group = new QGroupBox(tr("Application"), this);

    auto targetSDKComboBox = new QComboBox(group);
    targetSDKComboBox->addItems(targets);
    targetSDKComboBox->setCurrentIndex(targets.indexOf(m_step->buildTargetSdk()));

    const auto cbActivated = QOverload<int>::of(&QComboBox::activated);
    connect(targetSDKComboBox, cbActivated, this, [this, targetSDKComboBox](int idx) {
       const QString sdk = targetSDKComboBox->itemText(idx);
       m_step->setBuildTargetSdk(sdk);
       AndroidManager::updateGradleProperties(step()->target(), QString()); // FIXME: Use real key.
   });

    auto hbox = new QHBoxLayout(group);
    hbox->addWidget(new QLabel(tr("Android build SDK:"), group));
    hbox->addWidget(targetSDKComboBox);

    return group;
}

QWidget *AndroidBuildApkWidget::createSignPackageGroup()
{
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);

    auto group = new QGroupBox(tr("Sign package"), this);

    auto keystoreLocationLabel = new QLabel(tr("Keystore:"), group);
    keystoreLocationLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    auto keystoreLocationChooser = new PathChooser(group);
    keystoreLocationChooser->setExpectedKind(PathChooser::File);
    keystoreLocationChooser->lineEdit()->setReadOnly(true);
    keystoreLocationChooser->setPath(m_step->keystorePath().toUserOutput());
    keystoreLocationChooser->setInitialBrowsePathBackup(QDir::homePath());
    keystoreLocationChooser->setPromptDialogFilter(tr("Keystore files (*.keystore *.jks)"));
    keystoreLocationChooser->setPromptDialogTitle(tr("Select Keystore File"));
    connect(keystoreLocationChooser, &PathChooser::pathChanged, this, [this](const QString &path) {
        FilePath file = FilePath::fromString(path);
        m_step->setKeystorePath(file);
        m_signPackageCheckBox->setChecked(!file.isEmpty());
        if (!file.isEmpty())
            setCertificates();
    });

    auto keystoreCreateButton = new QPushButton(tr("Create..."), group);
    connect(keystoreCreateButton, &QAbstractButton::clicked, this, [this, keystoreLocationChooser] {
        AndroidCreateKeystoreCertificate d;
        if (d.exec() != QDialog::Accepted)
            return;
        keystoreLocationChooser->setPath(d.keystoreFilePath().toUserOutput());
        m_step->setKeystorePath(d.keystoreFilePath());
        m_step->setKeystorePassword(d.keystorePassword());
        m_step->setCertificateAlias(d.certificateAlias());
        m_step->setCertificatePassword(d.certificatePassword());
        setCertificates();
    });

    m_signPackageCheckBox = new QCheckBox(tr("Sign package"), group);
    m_signPackageCheckBox->setChecked(m_step->signPackage());

    m_signingDebugWarningLabel = new Utils::InfoLabel(tr("Signing a debug package"),
                                                      Utils::InfoLabel::Warning, group);
    m_signingDebugWarningLabel->hide();

    auto certificateAliasLabel = new QLabel(tr("Certificate alias:"), group);
    certificateAliasLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    m_certificatesAliasComboBox = new QComboBox(group);
    m_certificatesAliasComboBox->setEnabled(false);
    QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(0);
    m_certificatesAliasComboBox->setSizePolicy(sizePolicy2);
    m_certificatesAliasComboBox->setMinimumSize(QSize(300, 0));

    auto horizontalLayout_2 = new QHBoxLayout;
    horizontalLayout_2->addWidget(keystoreLocationLabel);
    horizontalLayout_2->addWidget(keystoreLocationChooser);
    horizontalLayout_2->addWidget(keystoreCreateButton);

    auto horizontalLayout_3 = new QHBoxLayout;
    horizontalLayout_3->addWidget(m_signingDebugWarningLabel);
    horizontalLayout_3->addWidget(certificateAliasLabel);
    horizontalLayout_3->addWidget(m_certificatesAliasComboBox);

    auto vbox = new QVBoxLayout(group);
    vbox->addLayout(horizontalLayout_2);
    vbox->addWidget(m_signPackageCheckBox);
    vbox->addLayout(horizontalLayout_3);

    connect(m_signPackageCheckBox, &QAbstractButton::toggled,
            this, &AndroidBuildApkWidget::signPackageCheckBoxToggled);

    auto updateAlias = [this](int idx) {
        QString alias = m_certificatesAliasComboBox->itemText(idx);
        if (!alias.isEmpty())
            m_step->setCertificateAlias(alias);
    };

    const auto cbActivated = QOverload<int>::of(&QComboBox::activated);
    const auto cbCurrentIndexChanged = QOverload<int>::of(&QComboBox::currentIndexChanged);

    connect(m_certificatesAliasComboBox, cbActivated, this, updateAlias);
    connect(m_certificatesAliasComboBox, cbCurrentIndexChanged, this, updateAlias);

    return group;
}

QWidget *AndroidBuildApkWidget::createAdvancedGroup()
{
    auto group = new QGroupBox(tr("Advanced Actions"), this);

    auto openPackageLocationCheckBox = new QCheckBox(tr("Open package location after build"), group);
    openPackageLocationCheckBox->setChecked(m_step->openPackageLocation());
    connect(openPackageLocationCheckBox, &QAbstractButton::toggled,
            this, [this](bool checked) { m_step->setOpenPackageLocation(checked); });

    m_addDebuggerCheckBox = new QCheckBox(tr("Add debug server"), group);
    m_addDebuggerCheckBox->setEnabled(false);
    m_addDebuggerCheckBox->setToolTip(tr("Packages debug server with "
           "the APK to enable debugging. For the signed APK this option is unchecked by default."));
    m_addDebuggerCheckBox->setChecked(m_step->addDebugger());
    connect(m_addDebuggerCheckBox, &QAbstractButton::toggled,
            m_step, &AndroidBuildApkStep::setAddDebugger);

    auto verboseOutputCheckBox = new QCheckBox(tr("Verbose output"), group);
    verboseOutputCheckBox->setChecked(m_step->verboseOutput());

    auto vbox = new QVBoxLayout(group);
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(step()->target()->kit());
    if (version && version->qtVersion() >= QtSupport::QtVersionNumber{5,14}) {
        auto buildAAB = new QCheckBox(tr("Build .aab (Android App Bundle)"), group);
        buildAAB->setChecked(m_step->buildAAB());
        connect(buildAAB, &QAbstractButton::toggled, m_step, &AndroidBuildApkStep::setBuildAAB);
        vbox->addWidget(buildAAB);
    }
    vbox->addWidget(openPackageLocationCheckBox);
    vbox->addWidget(verboseOutputCheckBox);
    vbox->addWidget(m_addDebuggerCheckBox);

    connect(verboseOutputCheckBox, &QAbstractButton::toggled,
            this, [this](bool checked) { m_step->setVerboseOutput(checked); });

    return group;
}

QWidget *AndroidBuildApkWidget::createCreateTemplatesGroup()
{
    auto createTemplatesGroupBox = new QGroupBox(tr("Android"));
    createTemplatesGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto createAndroidTemplatesButton = new QPushButton(tr("Create Templates"));
    connect(createAndroidTemplatesButton, &QAbstractButton::clicked, this, [this] {
        CreateAndroidManifestWizard wizard(m_step->buildSystem());
        wizard.exec();
    });

    auto horizontalLayout = new QHBoxLayout(createTemplatesGroupBox);
    horizontalLayout->addWidget(createAndroidTemplatesButton);
    horizontalLayout->addStretch(1);

    return createTemplatesGroupBox;
}

QWidget *AndroidBuildApkWidget::createAdditionalLibrariesGroup()
{
    auto group = new QGroupBox(tr("Additional Libraries"));
    group->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    auto libsModel = new AndroidExtraLibraryListModel(m_step->buildSystem(), this);
    connect(libsModel, &AndroidExtraLibraryListModel::enabledChanged, this,
            [this, group](const bool enabled) {
                group->setEnabled(enabled);
                m_openSslCheckBox->setChecked(isOpenSslLibsIncluded());
    });

    auto libsView = new QListView;
    libsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    libsView->setToolTip(tr("List of extra libraries to include in Android package and load on startup."));
    libsView->setModel(libsModel);

    auto addLibButton = new QToolButton;
    addLibButton->setText(tr("Add..."));
    addLibButton->setToolTip(tr("Select library to include in package."));
    addLibButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    addLibButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    connect(addLibButton, &QAbstractButton::clicked, this, [this, libsModel] {
        QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                                              tr("Select additional libraries"),
                                                              QDir::homePath(),
                                                              tr("Libraries (*.so)"));
        if (!fileNames.isEmpty())
            libsModel->addEntries(fileNames);
    });

    auto removeLibButton = new QToolButton;
    removeLibButton->setText(tr("Remove"));
    removeLibButton->setToolTip(tr("Remove currently selected library from list."));
    connect(removeLibButton, &QAbstractButton::clicked, this, [libsModel, libsView] {
        QModelIndexList removeList = libsView->selectionModel()->selectedIndexes();
        libsModel->removeEntries(removeList);
    });

    auto libsButtonLayout = new QVBoxLayout;
    libsButtonLayout->addWidget(addLibButton);
    libsButtonLayout->addWidget(removeLibButton);
    libsButtonLayout->addStretch(1);

    m_openSslCheckBox = new QCheckBox(tr("Include prebuilt OpenSSL libraries"));
    m_openSslCheckBox->setToolTip(tr("This is useful for apps that use SSL operations. The path "
                                     "can be defined in Tools > Options > Devices > Android."));
    connect(m_openSslCheckBox, &QAbstractButton::clicked, this,
            &AndroidBuildApkWidget::onOpenSslCheckBoxChanged);

    auto grid = new QGridLayout(group);
    grid->addWidget(m_openSslCheckBox, 0, 0);
    grid->addWidget(libsView, 1, 0);
    grid->addLayout(libsButtonLayout, 1, 1);

    QItemSelectionModel *libSelection = libsView->selectionModel();
    connect(libSelection, &QItemSelectionModel::selectionChanged, this, [libSelection, removeLibButton] {
        removeLibButton->setEnabled(libSelection->hasSelection());
    });

    Target *target = m_step->target();
    const QString buildKey = target->activeBuildKey();
    const ProjectNode *node = target->project()->findNodeForBuildKey(buildKey);
    group->setEnabled(node && !node->parseInProgress());

    return group;
}

void AndroidBuildApkWidget::signPackageCheckBoxToggled(bool checked)
{
    m_certificatesAliasComboBox->setEnabled(checked);
    m_step->setSignPackage(checked);
    m_addDebuggerCheckBox->setChecked(!checked);
    updateSigningWarning();
    if (!checked)
        return;
    if (!m_step->keystorePath().isEmpty())
        setCertificates();
}

void AndroidBuildApkWidget::onOpenSslCheckBoxChanged()
{
    Utils::FilePath projectPath = m_step->buildConfiguration()->buildSystem()->projectFilePath();
    QFile projectFile(projectPath.toString());
    if (!projectFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qWarning() << "Cound't open project file to add OpenSSL extra libs: " << projectPath;
        return;
    }

    const QString searchStr = openSslIncludeFileContent(projectPath);
    QTextStream textStream(&projectFile);

    QString fileContent = textStream.readAll();
    if (!m_openSslCheckBox->isChecked()) {
        fileContent.remove("\n" + searchStr);
    } else if (!fileContent.contains(searchStr, Qt::CaseSensitive)) {
        fileContent.append(searchStr + "\n");
    }

    projectFile.resize(0);
    textStream << fileContent;
    projectFile.close();
}

bool AndroidBuildApkWidget::isOpenSslLibsIncluded()
{
    Utils::FilePath projectPath = m_step->buildConfiguration()->buildSystem()->projectFilePath();
    const QString searchStr = openSslIncludeFileContent(projectPath);
    QFile projectFile(projectPath.toString());
    projectFile.open(QIODevice::ReadOnly);
    QTextStream textStream(&projectFile);
    QString fileContent = textStream.readAll();
    projectFile.close();
    return fileContent.contains(searchStr, Qt::CaseSensitive);
}

QString AndroidBuildApkWidget::openSslIncludeFileContent(const Utils::FilePath &projectPath)
{
    QString openSslPath = AndroidConfigurations::currentConfig().openSslLocation().toString();
    if (projectPath.endsWith(".pro"))
        return "android: include(" + openSslPath + "/openssl.pri)";
    if (projectPath.endsWith("CMakeLists.txt"))
        return "if (ANDROID)\n    include(" + openSslPath + "/CMakeLists.txt)\nendif()";

    return QString();
}

void AndroidBuildApkWidget::setCertificates()
{
    QAbstractItemModel *certificates = m_step->keystoreCertificates();
    if (certificates) {
        m_signPackageCheckBox->setChecked(certificates);
        m_certificatesAliasComboBox->setModel(certificates);
    }
}

void AndroidBuildApkWidget::updateSigningWarning()
{
    bool nonRelease = m_step->buildType() != BuildConfiguration::Release;
    bool visible = m_step->signPackage() && nonRelease;
    m_signingDebugWarningLabel->setVisible(visible);
}

} // Internal
} // Android
