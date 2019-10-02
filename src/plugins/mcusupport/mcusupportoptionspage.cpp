/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include "mcusupportconstants.h"
#include "mcusupportoptionspage.h"

#include <coreplugin/icore.h>
#include <cmakeprojectmanager/cmakekitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSizePolicy>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

namespace McuSupport {
namespace Internal {

class PackageOptions : public QObject
{
    Q_OBJECT

public:
    enum Status {
        InvalidPath,
        ValidPathInvalidPackage,
        ValidPackage
    };

    PackageOptions(const QString &label, const QString &defaultPath, const QString &detectionPath,
                   const QString &settingsKey);

    QString path() const;
    QString label() const;
    QString detectionPath() const;
    Status status() const;
    void setDownloadUrl(const QUrl &url);
    void setEnvironmentVariableName(const QString &name);
    void setAddToPath(bool addToPath);
    bool addToPath() const;
    void writeToSettings() const;

    QWidget *widget();


    QString environmentVariableName() const;

signals:
    void changed();

private:
    void updateStatus();

    QWidget *m_widget = nullptr;
    Utils::PathChooser *m_fileChooser = nullptr;
    QLabel *m_statusIcon = nullptr;
    QLabel *m_statusLabel = nullptr;

    const QString m_label;
    const QString m_defaultPath;
    const QString m_detectionPath;
    const QString m_settingsKey;

    QString m_path;
    QUrl m_downloadUrl;
    QString m_environmentVariableName;
    bool m_addToPath = false;

    Status m_status = InvalidPath;
};

PackageOptions::PackageOptions(const QString &label, const QString &defaultPath,
                               const QString &detectionPath, const QString &settingsKey)
    : m_label(label)
    , m_defaultPath(defaultPath)
    , m_detectionPath(detectionPath)
    , m_settingsKey(settingsKey)
{
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_GROUP);
    m_path = s->value(QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + m_settingsKey,
                      m_defaultPath).toString();
    s->endGroup();
}

QString PackageOptions::path() const
{
    return m_fileChooser->path();
}

QString PackageOptions::label() const
{
    return m_label;
}

QString PackageOptions::detectionPath() const
{
    return m_detectionPath;
}

QWidget *PackageOptions::widget()
{
    if (m_widget)
        return m_widget;

    m_widget = new QWidget;
    m_fileChooser = new Utils::PathChooser;
    QObject::connect(m_fileChooser, &Utils::PathChooser::pathChanged,
                     [this](){
        updateStatus();
        emit changed();
    });

    auto layout = new QGridLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    m_statusIcon = new QLabel;
    m_statusIcon->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
    m_statusIcon->setAlignment(Qt::AlignTop);
    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    if (!m_downloadUrl.isEmpty()) {
        auto downLoadButton = new QToolButton;
        downLoadButton->setIcon(Utils::Icons::DOWNLOAD.icon());
        downLoadButton->setToolTip(McuSupportOptionsPage::tr("Download from \"%1\"")
                                   .arg(m_downloadUrl.toString()));
        QObject::connect(downLoadButton, &QToolButton::pressed, [this]{
            QDesktopServices::openUrl(m_downloadUrl);
        });
        layout->addWidget(downLoadButton, 0, 2);
    }

    layout->addWidget(m_fileChooser, 0, 0, 1, 2);
    layout->addWidget(m_statusIcon, 1, 0);
    layout->addWidget(m_statusLabel, 1, 1, 1, -1);

    m_fileChooser->setPath(m_path); // Triggers updateStatus() call
    return m_widget;
}

PackageOptions::Status PackageOptions::status() const
{
    return m_status;
}

void PackageOptions::setDownloadUrl(const QUrl &url)
{
    m_downloadUrl = url;
}

void PackageOptions::setEnvironmentVariableName(const QString &name)
{
    m_environmentVariableName = name;
}

QString PackageOptions::environmentVariableName() const
{
    return m_environmentVariableName;
}

void PackageOptions::setAddToPath(bool addToPath)
{
    m_addToPath = addToPath;
}

bool PackageOptions::addToPath() const
{
    return m_addToPath;
}

void PackageOptions::writeToSettings() const
{
    if (m_path.compare(m_defaultPath) == 0)
        return;
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Constants::SETTINGS_GROUP);
    s->setValue(QLatin1String(Constants::SETTINGS_KEY_PACKAGE_PREFIX) + m_settingsKey, m_path);
    s->endGroup();
}

void PackageOptions::updateStatus()
{
    m_path = m_fileChooser->rawPath();
    const bool validPath = m_fileChooser->isValid();
    const Utils::FilePath detectionPath = Utils::FilePath::fromString(
                m_fileChooser->path() + "/" + m_detectionPath);
    const QString displayDetectionPath = Utils::FilePath::fromString(m_detectionPath).toUserOutput();
    const bool validPackage = detectionPath.exists();

    m_status = validPath ? (validPackage ? ValidPackage : ValidPathInvalidPackage) : InvalidPath;

    static const QPixmap okIcon = Utils::Icons::OK.pixmap();
    static const QPixmap notOkIcon = Utils::Icons::BROKEN.pixmap();
    m_statusIcon->setPixmap(m_status == ValidPackage ? okIcon : notOkIcon);

    QString statusText;
    switch (m_status) {
    case ValidPackage:
        statusText = McuSupportOptionsPage::tr(
                    "Path is valid, \"%1\" was found.").arg(displayDetectionPath);
        break;
    case ValidPathInvalidPackage:
        statusText = McuSupportOptionsPage::tr(
                    "Path exists, but does not contain \"%1\".").arg(displayDetectionPath);
        break;
    case InvalidPath:
        statusText = McuSupportOptionsPage::tr("Path does not exist.");
        break;
    }
    m_statusLabel->setText(statusText);
}

class BoardOptions : public QObject
{
    Q_OBJECT

public:
    BoardOptions(const QString &model, const QString &toolChainFile,
                 const QVector<PackageOptions *> &packages);

    QString model() const;
    QString toolChainFile() const;
    QVector<PackageOptions *> packages() const;

private:
    const QString m_model;
    const QString m_toolChainFile;
    const QVector<PackageOptions*> m_packages;
};

BoardOptions::BoardOptions(const QString &model, const QString &toolChainFileName,
                           const QVector<PackageOptions*> &packages)
    : m_model(model)
    , m_toolChainFile(toolChainFileName)
    , m_packages(packages)
{
}

QString BoardOptions::model() const
{
    return m_model;
}

QString BoardOptions::toolChainFile() const
{
    return m_toolChainFile;
}

QVector<PackageOptions *> BoardOptions::packages() const
{
    return m_packages;
}

class McuSupportOptions : public QObject
{
    Q_OBJECT

public:
    McuSupportOptions(QObject *parent = nullptr);
    ~McuSupportOptions() override;

    QVector<BoardOptions*> validBoards() const;

    QVector<PackageOptions*> packages;
    QVector<BoardOptions*> boards;
    PackageOptions* toolchainPackage = nullptr;

signals:
    void changed();
};

McuSupportOptions::McuSupportOptions(QObject *parent)
    : QObject(parent)
{
    auto qulPackage = new PackageOptions(
                McuSupportOptionsPage::tr("Qt MCU SDK"),
                QDir::homePath(),
                Utils::HostOsInfo::withExecutableSuffix("bin/qmltocpp"),
                "qulSdk");
    qulPackage->setEnvironmentVariableName("Qul_DIR");

    const QString armGccDefaultPath =
            Utils::HostOsInfo::isWindowsHost() ?
                QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles(x86)"))
                + "/GNU Tools ARM Embedded/"
              : QString("%{Env:ARMGCC_DIR}");
    auto armGcc = new PackageOptions(
                McuSupportOptionsPage::tr("GNU Arm Embedded Toolchain"),
                armGccDefaultPath,
                Utils::HostOsInfo::withExecutableSuffix("bin/arm-none-eabi-g++"),
                Constants::SETTINGS_KEY_PACKAGE_ARMGCC);
    armGcc->setDownloadUrl(
                QUrl::fromUserInput("https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads"));
    armGcc->setEnvironmentVariableName("ARMGCC_DIR");
    toolchainPackage = armGcc;

    auto stm32CubeFwF7Sdk = new PackageOptions(
                McuSupportOptionsPage::tr("STM32Cube SDK"),
                "%{Env:STM32Cube_FW_F7_SDK_PATH}",
                "Drivers/STM32F7xx_HAL_Driver",
                "stm32CubeFwF7Sdk");
    stm32CubeFwF7Sdk->setDownloadUrl(
                QUrl::fromUserInput("https://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-mcu-packages/stm32cubef7.html"));
    stm32CubeFwF7Sdk->setEnvironmentVariableName("STM32Cube_FW_F7_SDK_PATH");

    const QString stm32CubeProgrammerDefaultPath =
            Utils::HostOsInfo::isWindowsHost() ?
                QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles"))
                + "/STMicroelectronics/STM32Cube/STM32CubeProgrammer/"
              : QDir::homePath();
    auto stm32CubeProgrammer = new PackageOptions(
                McuSupportOptionsPage::tr("STM32CubeProgrammer"),
                stm32CubeProgrammerDefaultPath,
                "bin",
                "stm32CubeProgrammer");
    stm32CubeProgrammer->setDownloadUrl(
                QUrl::fromUserInput("https://www.st.com/en/development-tools/stm32cubeprog.html"));
    stm32CubeProgrammer->setAddToPath(true);

    auto evkbImxrt1050Sdk = new PackageOptions(
                McuSupportOptionsPage::tr("NXP EVKB-IMXRT1050 SDK"),
                "%{Env:EVKB_IMXRT1050_SDK_PATH}",
                "EVKB-IMXRT1050_manifest_v3_5.xml",
                "evkbImxrt1050Sdk");
    evkbImxrt1050Sdk->setDownloadUrl(
                QUrl::fromUserInput("https://mcuxpresso.nxp.com/en/welcome"));

    const QString seggerJLinkDefaultPath =
            Utils::HostOsInfo::isWindowsHost() ?
                QDir::fromNativeSeparators(qEnvironmentVariable("ProgramFiles")) + "/SEGGER/JLink"
              : QString("%{Env:SEGGER_JLINK_SOFTWARE_AND_DOCUMENTATION_PATH}");
    auto seggerJLink = new PackageOptions(
                McuSupportOptionsPage::tr("SEGGER JLink"),
                seggerJLinkDefaultPath,
                Utils::HostOsInfo::withExecutableSuffix("JLink"),
                "seggerJLink");
    seggerJLink->setDownloadUrl(
                QUrl::fromUserInput("https://www.segger.com/downloads/jlink"));

    auto stmPackages = {armGcc, stm32CubeFwF7Sdk, stm32CubeProgrammer, qulPackage};
    auto nxpPackages = {armGcc, evkbImxrt1050Sdk, seggerJLink, qulPackage};
    packages = {armGcc, stm32CubeFwF7Sdk, stm32CubeProgrammer,
                evkbImxrt1050Sdk, seggerJLink,
                qulPackage};

    boards.append(new BoardOptions(
                      "stm32f7508", "CMake/stm32f7508-discovery.cmake", stmPackages));
    boards.append(new BoardOptions(
                      "stm32f769i", "CMake/stm32f769i-discovery.cmake", stmPackages));
    boards.append(new BoardOptions(
                      "evkbimxrt1050", "CMake/evkbimxrt1050-toolchain.cmake", nxpPackages));

    for (auto package : packages)
        connect(package, &PackageOptions::changed, [this](){
            emit changed();
        });
}

McuSupportOptions::~McuSupportOptions()
{
    qDeleteAll(packages);
    packages.clear();
    qDeleteAll(boards);
    boards.clear();
}

QVector<BoardOptions *> McuSupportOptions::validBoards() const
{
    return Utils::filtered(boards, [](BoardOptions *board){
        return !Utils::anyOf(board->packages(), [](PackageOptions *package){
            return package->status() != PackageOptions::ValidPackage;});
    });
}

class McuSupportOptionsWidget : public QWidget
{
public:
    McuSupportOptionsWidget(const McuSupportOptions *options, QWidget *parent = nullptr);

    void updateStatus();
    void showBoardPackages(int boardIndex);

private:
    QString m_armGccPath;
    const McuSupportOptions *m_options;
    int m_currentBoardIndex = 0;
    QMap <PackageOptions*, QWidget*> m_packageWidgets;
    QMap <BoardOptions*, QWidget*> m_boardPacketWidgets;
    QFormLayout *m_packagesLayout = nullptr;
    QLabel *m_statusLabel = nullptr;
};

McuSupportOptionsWidget::McuSupportOptionsWidget(const McuSupportOptions *options, QWidget *parent)
    : QWidget(parent)
    , m_options(options)
{
    auto mainLayout = new QVBoxLayout(this);

    auto boardChooserlayout = new QHBoxLayout;
    auto boardChooserLabel = new QLabel(McuSupportOptionsPage::tr("MCU board:"));
    boardChooserlayout->addWidget(boardChooserLabel);
    auto boardComboBox = new QComboBox;
    boardChooserLabel->setBuddy(boardComboBox);
    boardChooserLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    boardComboBox->addItems(Utils::transform<QStringList>(m_options->boards, [](BoardOptions *b){
                                 return b->model();}));
    boardChooserlayout->addWidget(boardComboBox);
    mainLayout->addLayout(boardChooserlayout);

    auto m_packagesGroupBox = new QGroupBox(McuSupportOptionsPage::tr("Packages"));
    mainLayout->addWidget(m_packagesGroupBox);
    m_packagesLayout = new QFormLayout;
    m_packagesGroupBox->setLayout(m_packagesLayout);

    m_statusLabel = new QLabel;
    mainLayout->addWidget(m_statusLabel);
    m_statusLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    connect(options, &McuSupportOptions::changed, this, &McuSupportOptionsWidget::updateStatus);
    connect(boardComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &McuSupportOptionsWidget::showBoardPackages);

    showBoardPackages(m_currentBoardIndex);
}

static QString ulOfBoardModels(const QVector<BoardOptions*> &validBoards)
{
    return "<ul><li>"
        + Utils::transform<QStringList>(validBoards,[](BoardOptions* board)
                                                    {return board->model();}).join("</li><li>")
        + "</li></ul>";
}

void McuSupportOptionsWidget::updateStatus()
{
    const QVector<BoardOptions*> validBoards = m_options->validBoards();
    m_statusLabel->setText(validBoards.isEmpty()
                           ? McuSupportOptionsPage::tr("No devices and kits can currently be generated. "
                                                       "Select a board and provide the package paths. "
                                                       "Afterwards, press Apply to generate device and kit for "
                                                       "your board.")
                           : McuSupportOptionsPage::tr("Devices and kits for the following boards can be generated: "
                                                       "%1 "
                                                       "Press Apply to generate device and kit for "
                                                       "your board.").arg(ulOfBoardModels(validBoards)));
}

void McuSupportOptionsWidget::showBoardPackages(int boardIndex)
{
    while (m_packagesLayout->rowCount() > 0) {
        QFormLayout::TakeRowResult row = m_packagesLayout->takeRow(0);
        row.labelItem->widget()->hide();
        row.fieldItem->widget()->hide();
    }

    const BoardOptions *currentBoard = m_options->boards.at(boardIndex);

    for (auto package : m_options->packages) {
        QWidget *packageWidget = package->widget();
        if (!currentBoard->packages().contains(package))
            continue;
        m_packagesLayout->addRow(package->label(), packageWidget);
        packageWidget->show();
    }
}

McuSupportOptionsPage::McuSupportOptionsPage(QObject* parent)
    : Core::IOptionsPage(parent)
{
    setId(Core::Id(Constants::SETTINGS_ID));
    setDisplayName(tr("MCU"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
}

QWidget* McuSupportOptionsPage::widget()
{
    if (!m_options)
        m_options = new McuSupportOptions(this);
    if (!m_widget)
        m_widget = new McuSupportOptionsWidget(m_options);
    return m_widget;
}

static ProjectExplorer::ToolChain* armGccToolchain(const Utils::FilePath &path, Core::Id language)
{
    using namespace ProjectExplorer;

    ToolChain *toolChain = ToolChainManager::toolChain([&path, language](const ToolChain *t){
        return t->compilerCommand() == path && t->language() == language;
    });
    if (!toolChain) {
        ToolChainFactory *gccFactory =
            Utils::findOrDefault(ToolChainFactory::allToolChainFactories(), [](ToolChainFactory *f){
            return f->supportedToolChainType() == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;
        });
        if (gccFactory) {
            const QList<ToolChain*> detected = gccFactory->detectForImport({path, language});
            if (!detected.isEmpty()) {
                toolChain = detected.first();
                toolChain->setDetection(ToolChain::ManualDetection);
                toolChain->setDisplayName("Arm GCC");
                ToolChainManager::registerToolChain(toolChain);
            }
        }
    }

    return toolChain;
}

static void setKitProperties(ProjectExplorer::Kit *k, const BoardOptions* board)
{
    using namespace ProjectExplorer;

    k->setUnexpandedDisplayName("Qt MCU - " + board->model());
    k->setValue(Constants::KIT_BOARD_MODEL_KEY, board->model());
    k->setAutoDetected(false);
    k->setIrrelevantAspects({
        SysRootKitAspect::id(),
        "QtSupport.QtInformation" // QtKitAspect::id()
    });
}

static void setKitToolchains(ProjectExplorer::Kit *k, const QString &armGccPath)
{
    using namespace ProjectExplorer;

    const QString compileNameScheme = Utils::HostOsInfo::withExecutableSuffix(
                armGccPath + "/bin/arm-none-eabi-%1");
    ToolChain *cTc = armGccToolchain(
                Utils::FilePath::fromUserInput(compileNameScheme.arg("gcc")),
                ProjectExplorer::Constants::C_LANGUAGE_ID);
    ToolChain *cxxTc = armGccToolchain(
                Utils::FilePath::fromUserInput(compileNameScheme.arg("g++")),
                ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    ToolChainKitAspect::setToolChain(k, cTc);
    ToolChainKitAspect::setToolChain(k, cxxTc);
}

static void setKitDevice(ProjectExplorer::Kit *k)
{
    using namespace ProjectExplorer;

    DeviceTypeKitAspect::setDeviceTypeId(k, Constants::DEVICE_TYPE);
}

static void setKitEnvironment(ProjectExplorer::Kit *k, const BoardOptions* board)
{
    using namespace ProjectExplorer;

    Utils::EnvironmentItems changes;
    QStringList pathAdditions;
    for (auto package : board->packages()) {
        if (package->addToPath())
            pathAdditions.append(QDir::toNativeSeparators(package->path()));
        if (!package->environmentVariableName().isEmpty())
            changes.append({package->environmentVariableName(),
                            QDir::toNativeSeparators(package->path())});
    }
    if (!pathAdditions.isEmpty()) {
        pathAdditions.append("${Path}");
        changes.append({"Path", pathAdditions.join(Utils::HostOsInfo::pathListSeparator())});
    }
    EnvironmentKitAspect::setEnvironmentChanges(k, changes);
}

static void setCMakeOptions(ProjectExplorer::Kit *k, const BoardOptions* board)
{
    using namespace CMakeProjectManager;

    CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);
    config.append(CMakeConfigItem("CMAKE_TOOLCHAIN_FILE",
                                  ("%{CurrentBuild:Env:Qul_DIR}/" +
                                   board->toolChainFile()).toUtf8()));
    CMakeConfigurationKitAspect::setConfiguration(k, config);
}

static ProjectExplorer::Kit* boardKit(const BoardOptions* board, const QString &armGccPath)
{
    using namespace ProjectExplorer;

    Kit *kit = KitManager::kit([board](const Kit *k){
        return board->model() == k->value(Constants::KIT_BOARD_MODEL_KEY).toString();
    });
    if (!kit) {
        const auto init = [board, &armGccPath](Kit *k) {
            KitGuard kitGuard(k);

            setKitProperties(k, board);
            setKitToolchains(k, armGccPath);
            setKitDevice(k);
            setKitEnvironment(k, board);
            setCMakeOptions(k, board);

            k->setup();
            k->fix();
        };
        kit = KitManager::registerKit(init);
    }
    return kit;
}

void McuSupportOptionsPage::apply()
{
    for (auto package : m_options->packages)
        package->writeToSettings();

    QTC_ASSERT(m_options->toolchainPackage, return);

    const QVector<BoardOptions*> validBoards = m_options->validBoards();

    using namespace ProjectExplorer;

    for (auto board : validBoards) {
        Kit *kit = boardKit(board, m_options->toolchainPackage->path());
    }
}

void McuSupportOptionsPage::finish()
{
    delete m_options;
    m_options = nullptr;
    delete m_widget;
}

} // Internal
} // McuSupport

#include "mcusupportoptionspage.moc"
