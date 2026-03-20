// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggeritemmanager.h"

#include "debuggerkitaspect.h"
#include "debuggertr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <nanotrace/nanotrace.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/devicemanagermodel.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/groupedmodel.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>

#include <QDebug>
#include <QDir>
#include <QFont>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QTreeView>
#include <QVersionNumber>
#include <QWidget>

#include <QtTaskTree/QSingleTaskTreeRunner>

using namespace Core;
using namespace Debugger;
using namespace Debugger::Internal;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

static DebuggerItem makeAutoDetectedDebuggerItem(
    const FilePath &command,
    const DebuggerItem::TechnicalData &technicalData,
    const DetectionSource &detectionSource)
{
    DebuggerItem item;
    item.createId();
    item.setCommand(command);
    item.setDetectionSource(detectionSource);
    item.setEngineType(technicalData.engineType);
    item.setAbis(technicalData.abis);
    item.setVersion(technicalData.version);
    const QString name = detectionSource.id.isEmpty() ? Tr::tr("System %1 at %2")
                                                      : Tr::tr("Detected %1 at %2");
    item.setUnexpandedDisplayName(name.arg(item.engineTypeName()).arg(command.toUserOutput()));
    item.setLastModified(command.lastModified());
    return item;
}

static Result<DebuggerItem> makeAutoDetectedDebuggerItem(
    const FilePath &command, const DetectionSource &detectionSource)
{
    Result<DebuggerItem::TechnicalData> technicalData
        = DebuggerItem::TechnicalData::extract(command, {});

    if (!technicalData)
        return make_unexpected(std::move(technicalData).error());

    return makeAutoDetectedDebuggerItem(command, *technicalData, detectionSource);
}

namespace Debugger {
namespace Internal {

const char DEBUGGER_COUNT_KEY[] = "DebuggerItem.Count";
const char DEBUGGER_DATA_KEY[] = "DebuggerItem.";
const char DEBUGGER_FILE_VERSION_KEY[] = "Version";
const char DEBUGGER_FILENAME[] = "debuggers.xml";
const char debuggingToolsWikiLinkC[] = "http://wiki.qt.io/Qt_Creator_Windows_Debugging";

static FilePath userSettingsFileName()
{
    return ICore::userResourcePath(DEBUGGER_FILENAME);
}

// -----------------------------------------------------------------------
// DebuggerItemConfigWidget
// -----------------------------------------------------------------------

class DebuggerItemConfigWidget : public QWidget
{
public:
    DebuggerItemConfigWidget();

    void load(const DebuggerItem &item);
    void store() const;
    void setDeviceRootPath(const FilePath &rootPath);

private:
    void binaryPathHasChanged();
    DebuggerItem item() const;
    void setAbis(const QStringList &abiNames);

    QLineEdit *m_displayNameLineEdit;
    QLabel *m_cdbLabel;
    PathChooser *m_binaryChooser;
    DetectionSource m_detectionSource;
    DebuggerEngineType m_engineType = NoEngineType;
    QVariant m_id;

    QLabel *m_abis;
    QLabel *m_version;
    QLabel *m_type;

    PathChooser *m_workingDirectoryChooser;
    QSingleTaskTreeRunner m_taskTreeRunner;
};

// --------------------------------------------------------------------------
// DebuggerItemModel
// --------------------------------------------------------------------------

class DebuggerModel final : public Utils::TypedGroupedModel<DebuggerItem>
{
public:
    DebuggerModel();

    void updateDebugger(const DebuggerItem &item);

    void detectDebuggers(const IDeviceConstPtr &device, const FilePaths &searchPaths);
    void restoreDebuggers();
    void saveDebuggers();

    QVariant registerDebugger(const DebuggerItem &item);
    void deregisterDebugger(const QVariant &id);

    void readDebuggers(const FilePath &fileName, bool isSdk);
    void autoDetectCdbDebuggers();
    void autoDetectGdbOrLldbDebuggers(const FilePaths &searchPaths,
                                      const QString &detectionSource,
                                      QString *logMessage = nullptr);
    void autoDetectUvscDebuggers();
    QString uniqueDisplayName(const QString &base);

    QVariant variantData(const QVariant &item, int column, int role) const final
    {
        return fromVariant(item).data(column, role);
    }

private:
    PersistentSettingsWriter m_writer;
};

static DebuggerModel &debuggerModel()
{
    static DebuggerModel theModel;
    return theModel;
}

DebuggerModel::DebuggerModel()
    : m_writer(userSettingsFileName(), "QtCreatorDebuggers")
{
    setHeader({Tr::tr("Name"), Tr::tr("Path"), Tr::tr("Type")});

    setFilters(ProjectExplorer::Constants::msgAutoDetected(),
               {{ProjectExplorer::Constants::msgManual(), [this](int row) {
                   return !item(row).detectionSource().isAutoDetected();
               }}});

    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &DebuggerModel::saveDebuggers);
    connect(DeviceManager::instance(), &DeviceManager::toolDetectionRequested, this,
            [this](Id devId, const FilePaths &searchPaths, quint64 token) {
        const IDevicePtr dev = DeviceManager::find(devId);
        QTC_ASSERT(dev, return);
        dev->registerToolDetectionTask(token);
        detectDebuggers(dev, searchPaths);
        dev->deregisterToolDetectionTask(token);
    });
}

void DebuggerModel::updateDebugger(const DebuggerItem &ditem)
{
    for (int i = 0; i < itemCount(); ++i) {
        if (item(i).id() == ditem.id()) {
            setVolatileItem(i, ditem);
            notifyRowChanged(i);
            return;
        }
    }
}


DebuggerItemConfigWidget::DebuggerItemConfigWidget()
{
    m_displayNameLineEdit = new QLineEdit(this);

    m_binaryChooser = new PathChooser(this);
    m_binaryChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_binaryChooser->setMinimumWidth(400);
    m_binaryChooser->setHistoryCompleter("DebuggerPaths");
    m_binaryChooser->setValidationFunction(
        [this](const QString &text) -> FancyLineEdit::AsyncValidationFuture {
            return m_binaryChooser->defaultValidationFunction()(text).then(
                [](const FancyLineEdit::AsyncValidationResult &result)
                    -> FancyLineEdit::AsyncValidationResult {
                    if (!result)
                        return result;

                    DebuggerItem item;
                    item.setCommand(FilePath::fromUserInput(result.value()));
                    QString errorMessage;
                    item.reinitializeFromFile(&errorMessage);

                    if (!errorMessage.isEmpty())
                        return make_unexpected(errorMessage);

                    return result.value();
                });
        });
    m_binaryChooser->setAllowPathFromDevice(true);

    m_workingDirectoryChooser = new PathChooser(this);
    m_workingDirectoryChooser->setExpectedKind(PathChooser::Directory);
    m_workingDirectoryChooser->setMinimumWidth(400);
    m_workingDirectoryChooser->setHistoryCompleter("DebuggerPaths");

    auto makeInteractiveLabel = []() {
        auto label = new QLabel;
        label->setTextInteractionFlags(Qt::TextEditorInteraction | Qt::TextBrowserInteraction);
        label->setOpenExternalLinks(true);
        return label;
    };

    m_cdbLabel = makeInteractiveLabel();
    m_version = makeInteractiveLabel();
    m_abis = makeInteractiveLabel();
    m_type = makeInteractiveLabel();

    connect(m_binaryChooser, &PathChooser::textChanged,
            this, &DebuggerItemConfigWidget::binaryPathHasChanged);
    connect(m_workingDirectoryChooser, &PathChooser::textChanged,
            this, &DebuggerItemConfigWidget::store);
    connect(m_displayNameLineEdit, &QLineEdit::textChanged,
            this, &DebuggerItemConfigWidget::store);

    // clang-format off
    using namespace Layouting;
    Form {
        fieldGrowthPolicy(int(QFormLayout::AllNonFixedFieldsGrow)),
        Tr::tr("Name:"), m_displayNameLineEdit, br,
        Tr::tr("Path:"), m_binaryChooser, br,
        m_cdbLabel, br,
        Tr::tr("Type:"), m_type, br,
        Tr::tr("ABIs:"), m_abis, br,
        Tr::tr("Version:"), m_version, br,
        Tr::tr("Working directory:"), m_workingDirectoryChooser, br,
    }.attachTo(this);
    // clang-format on
}

DebuggerItem DebuggerItemConfigWidget::item() const
{
    static const QRegularExpression noAbi("[^A-Za-z0-9-_]+");

    DebuggerItem item(m_id);
    item.setUnexpandedDisplayName(m_displayNameLineEdit->text());
    item.setCommand(m_binaryChooser->filePath());
    item.setWorkingDirectory(m_workingDirectoryChooser->filePath());
    item.setDetectionSource(m_detectionSource);
    Abis abiList;
    const QStringList abis = m_abis->text().split(noAbi);
    for (const QString &a : abis) {
        if (a.isNull())
            continue;
        abiList << Abi::fromString(a);
    }
    item.setAbis(abiList);
    item.setVersion(m_version->text());
    item.setEngineType(m_engineType);
    return item;
}

void DebuggerItemConfigWidget::store() const
{
    if (!m_id.isNull())
        debuggerModel().updateDebugger(item());
}

void DebuggerItemConfigWidget::setDeviceRootPath(const FilePath &rootPath)
{
    m_binaryChooser->setInitialBrowsePathBackup(rootPath);
}

void DebuggerItemConfigWidget::setAbis(const QStringList &abiNames)
{
    m_abis->setText(abiNames.join(", "));
}

void DebuggerItemConfigWidget::load(const DebuggerItem &item)
{
    m_id = QVariant(); // reset Id to avoid intermediate signal handling
    if (!item)
        return;

    // Set values:
    m_detectionSource = item.detectionSource();

    m_displayNameLineEdit->setEnabled(!item.detectionSource().isAutoDetected());
    m_displayNameLineEdit->setText(item.unexpandedDisplayName());

    m_type->setText(item.engineTypeName());

    m_binaryChooser->setReadOnly(item.detectionSource().isAutoDetected());
    m_binaryChooser->setFilePath(item.command());
    m_binaryChooser->setExpectedKind(
        item.isGeneric() ? PathChooser::Any : PathChooser::ExistingCommand);

    m_workingDirectoryChooser->setReadOnly(item.detectionSource().isAutoDetected());
    m_workingDirectoryChooser->setFilePath(item.workingDirectory());

    QString text;
    QString versionCommand;
    if (item.engineType() == CdbEngineType) {
        const bool is64bit = is64BitWindowsSystem();
        const QString versionString = is64bit ? Tr::tr("64-bit version") : Tr::tr("32-bit version");
        //: Label text for path configuration. %2 is "x-bit version".
        text = "<html><body><p>"
                + Tr::tr("Specify the path to the "
                     "<a href=\"%1\">Windows Console Debugger executable</a>"
                     " (%2) here.").arg(QLatin1String(debuggingToolsWikiLinkC), versionString)
                + "</p></body></html>";
        versionCommand = "-version";
    } else {
        versionCommand = "--version";
    }

    m_cdbLabel->setText(text);
    m_cdbLabel->setVisible(!text.isEmpty());
    m_binaryChooser->setCommandVersionArguments(QStringList(versionCommand));
    m_version->setText(item.version());
    setAbis(item.abiNames());
    m_engineType = item.engineType();
    m_id = item.id();
}

void DebuggerItemConfigWidget::binaryPathHasChanged()
{
    // Ignore change if this is no valid DebuggerItem
    if (!m_id.isValid())
        return;

    m_taskTreeRunner.reset();

    if (m_binaryChooser->filePath().isExecutableFile()) {
        const auto onSetup = [this](Async<DebuggerItem> &task) {
            task.setConcurrentCallData([tmp = item()]() mutable {
                tmp.reinitializeFromFile();
                return tmp;
            });
        };
        const auto onDone = [this](const Async<DebuggerItem> &task) {
            if (!task.isResultAvailable())
                return;

            const DebuggerItem tmp = task.result();
            setAbis(tmp.abiNames());
            m_version->setText(tmp.version());
            m_engineType = tmp.engineType();
            m_type->setText(tmp.engineTypeName());
        };
        m_taskTreeRunner.start({AsyncTask<DebuggerItem>(onSetup, onDone)});
    } else {
        const DebuggerItem tmp;
        setAbis(tmp.abiNames());
        m_version->setText(tmp.version());
        m_engineType = tmp.engineType();
        m_type->setText(tmp.engineTypeName());
    }

    store();
}

void DebuggerModel::autoDetectCdbDebuggers()
{
    FilePaths cdbs;

    const QStringList programDirs = {qtcEnvironmentVariable("ProgramFiles"),
                                     qtcEnvironmentVariable("ProgramFiles(x86)"),
                                     qtcEnvironmentVariable("ProgramW6432")};

    QFileInfoList kitFolders;

    for (const QString &dirName : programDirs) {
        if (dirName.isEmpty())
            continue;
        const QDir dir(dirName);
        // Windows SDK's starting from version 8 live in
        // "ProgramDir\Windows Kits\<version>"
        const QString windowsKitsFolderName = "Windows Kits";
        if (dir.exists(windowsKitsFolderName)) {
            QDir windowKitsFolder = dir;
            if (windowKitsFolder.cd(windowsKitsFolderName)) {
                // Check in reverse order (latest first)
                kitFolders.append(windowKitsFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                                 QDir::Time | QDir::Reversed));
            }
        }

        // Pre Windows SDK 8: Check 'Debugging Tools for Windows'
        for (const QFileInfo &fi : dir.entryInfoList({"Debugging Tools for Windows*"},
                                                     QDir::Dirs | QDir::NoDotAndDotDot)) {
            const FilePath filePath = FilePath::fromFileInfo(fi).pathAppended("cdb.exe");
            if (!cdbs.contains(filePath))
                cdbs.append(filePath);
        }
    }


    constexpr char RootVal[]   = "KitsRoot";
    constexpr char RootVal81[] = "KitsRoot81";
    constexpr char RootVal10[] = "KitsRoot10";
    const QSettings installedRoots(
                "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",
                QSettings::NativeFormat);
    for (auto rootVal : {RootVal, RootVal81, RootVal10}) {
        QFileInfo root(installedRoots.value(QLatin1String(rootVal)).toString());
        if (root.exists() && !kitFolders.contains(root))
            kitFolders.append(root);
    }

    for (const QFileInfo &kitFolderFi : kitFolders) {
        const QString path = kitFolderFi.absoluteFilePath();
        QStringList abis = {"x86", "x64"};
        if (HostOsInfo::hostArchitecture() == Utils::OsArchArm64)
            abis << "arm64";
        for (const QString &abi: std::as_const(abis)) {
            const QFileInfo cdbBinary(path + "/Debuggers/" + abi + "/cdb.exe");
            if (cdbBinary.isExecutable())
                cdbs.append(FilePath::fromString(cdbBinary.absoluteFilePath()));
        }
    }

    for (const FilePath &cdb : std::as_const(cdbs)) {
        if (DebuggerItemManager::findByCommand(cdb))
            continue;
        DebuggerItem item;
        item.createId();
        item.setDetectionSource(DetectionSource::FromSystem);
        item.setAbis(Abi::abisOfBinary(cdb));
        item.setCommand(cdb);
        item.setEngineType(CdbEngineType);
        item.setUnexpandedDisplayName(
            uniqueDisplayName(Tr::tr("Auto-detected CDB at \"%1\"").arg(cdb.toUserOutput())));
        appendItem(item);
    }
}

static Utils::FilePaths searchGdbPathsFromRegistry()
{
    if (!HostOsInfo::isWindowsHost())
        return {};

    // Registry token for the "GNU Tools for ARM Embedded Processors".
    static const char kRegistryToken[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\" \
                                         "Windows\\CurrentVersion\\Uninstall\\";

    Utils::FilePaths searchPaths;

    QSettings registry(kRegistryToken, QSettings::NativeFormat);
    const auto productGroups = registry.childGroups();
    for (const QString &productKey : productGroups) {
        if (!productKey.startsWith("GNU Tools for ARM Embedded Processors"))
            continue;
        registry.beginGroup(productKey);
        QString uninstallFilePath = registry.value("UninstallString").toString();
        if (uninstallFilePath.startsWith(QLatin1Char('"')))
            uninstallFilePath.remove(0, 1);
        if (uninstallFilePath.endsWith(QLatin1Char('"')))
            uninstallFilePath.remove(uninstallFilePath.size() - 1, 1);
        registry.endGroup();

        const QString toolkitRootPath = QFileInfo(uninstallFilePath).path();
        const QString toolchainPath = toolkitRootPath + QLatin1String("/bin");
        searchPaths.push_back(FilePath::fromString(toolchainPath));
    }

    return searchPaths;
}

void DebuggerModel::autoDetectGdbOrLldbDebuggers(
    const FilePaths &searchPaths, const QString &detectionSourceId, QString *logMessage)
{
    const DetectionSource detectionSource{DetectionSource::FromSystem, detectionSourceId};

    QStringList filters
        = {"gdb-i686-pc-mingw32",
           "gdb-i686-pc-mingw32.exe",
           "gdb",
           "gdb.exe",
           "lldb",
           "lldb.exe",
           "lldb-[1-9]*",
           "arm-none-eabi-gdb-py.exe",
           "*-*-*-gdb"};

    if (nativeDapDebuggersEnabled()) {
        filters.append({
            "lldb-dap",
            "lldb-dap.exe",
            "lldb-dap-*",
            // LLDB DAP server was named lldb-vscode prior LLVM 18.0.0
            "lldb-vscode",
            "lldb-vscode.exe",
            "lldb-vscode-*",
        });
    }

    if (searchPaths.isEmpty())
        return;

    FilePaths suspects;

    // FIXME: Devicify.
    if (searchPaths.front().osType() == OsTypeMac) {
        Process proc;
        proc.setCommand({"xcrun", {"--find", "lldb"}});
        using namespace std::chrono_literals;
        proc.runBlocking(2s);
        if (proc.result() == ProcessResult::FinishedWithSuccess) {
            const FilePath lPath = FilePath::fromUserInput(proc.allOutput().trimmed());
            if (lPath.isExecutableFile())
                suspects.append(lPath);
        }
    }

    FilePaths paths = searchPaths;
    if (searchPaths.front().isLocal()) {
        paths.append(searchGdbPathsFromRegistry());

        const Result<FilePath> lldb = Core::ICore::lldbExecutable(CLANG_BINDIR);
        if (lldb)
            suspects.append(*lldb);
    }

    paths = Utils::filteredUnique(paths);

    for (const FilePath &path : paths)
        suspects.append(path.dirEntries({filters, QDir::Files | QDir::Executable}));

    QStringList logMessages{Tr::tr("Searching debuggers...")};
    for (const FilePath &command : std::as_const(suspects)) {
        int existingRow = -1;
        for (int i = 0; i < itemCount(); ++i) {
            if (item(i).command() == command) {
                existingRow = i;
                break;
            }
        }
        if (existingRow >= 0) {
            DebuggerItem existingItem = item(existingRow);
            if (command.lastModified() != existingItem.lastModified()) {
                existingItem.reinitializeFromFile();
                setVolatileItem(existingRow, existingItem);
                notifyRowChanged(existingRow);
            }

            if (nativeDapDebuggersEnabled()) {
                if (existingItem.engineType() != GdbEngineType)
                    continue;

                // GDB starting version 14.1.0 supports DAP interface, but unlike LLDB,
                // it uses the same binary, hence this hack.
                const QVersionNumber dapSupportMinVersion{14, 1, 0};
                if (QVersionNumber::fromString(existingItem.version()) < dapSupportMinVersion)
                    continue;
                // This is the "update" path: there's already a capable GDB in the settings,
                // we only need to add a corresponding DAP entry if it's missing.
                const bool hasDap = Utils::anyOf(volatileItems(), [&command](const DebuggerItem &item) {
                    return item.command() == command && item.engineType() == GdbDapEngineType;
                });
                if (hasDap)
                    continue;

                const DebuggerItem dapItem = makeAutoDetectedDebuggerItem(
                    command,
                    {
                        .engineType = GdbDapEngineType,
                        .abis = existingItem.abis(),
                        .version = existingItem.version(),
                    },
                    detectionSource);
                appendItem(dapItem);
                logMessages.append(
                    Tr::tr("Added a surrogate GDB DAP item for existing entry \"%1\".")
                        .arg(command.toUserOutput()));
            }
            continue;
        }

        const Result<DebuggerItem> item
            = makeAutoDetectedDebuggerItem(command, detectionSource);
        if (!item) {
            logMessages.append(item.error());
            continue;
        }

        appendItem(*item);
        logMessages.append(Tr::tr("Found: \"%1\".").arg(command.toUserOutput()));
        if (nativeDapDebuggersEnabled()) {
            if (item->engineType() != GdbEngineType)
                continue;

            // GDB starting version 14.1.0 supports DAP interface, but unlike LLDB,
            // it uses the same binary, hence this hack
            const QVersionNumber dapSupportMinVersion{14, 1, 0};
            if (QVersionNumber::fromString(item->version()) < dapSupportMinVersion)
                continue;

            const DebuggerItem dapItem = makeAutoDetectedDebuggerItem(
                command,
                {
                    .engineType = GdbDapEngineType,
                    .abis = item->abis(),
                    .version = item->version(),
                },
                detectionSource);
            appendItem(dapItem);
            logMessages.append(
                Tr::tr("Added a surrogate GDB DAP item for \"%1\".").arg(command.toUserOutput()));
        }
    }
    if (logMessage)
        *logMessage = logMessages.join('\n');
}

void DebuggerModel::autoDetectUvscDebuggers()
{
    if (!HostOsInfo::isWindowsHost())
        return;

    // Registry token for the "KEIL uVision" instance.
    static const char kRegistryToken[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\" \
                                         "Windows\\CurrentVersion\\Uninstall\\Keil \u00B5Vision4";

    QSettings registry(QLatin1String(kRegistryToken), QSettings::NativeFormat);
    const auto productGroups = registry.childGroups();
    for (const QString &productKey : productGroups) {
        if (!productKey.startsWith("App"))
            continue;
        registry.beginGroup(productKey);
        const QDir rootPath(registry.value("Directory").toString());
        registry.endGroup();
        const FilePath uVision = FilePath::fromString(
                    rootPath.absoluteFilePath("UV4/UV4.exe"));
        if (!uVision.exists())
            continue;
        if (DebuggerItemManager::findByCommand(uVision))
            continue;

        QString errorMsg;
        const QString uVisionVersion = winGetDLLVersion(
                    WinDLLFileVersion, uVision.toUrlishString(), &errorMsg);

        DebuggerItem item;
        item.createId();
        item.setDetectionSource(DetectionSource::FromSystem);
        item.setCommand(uVision);
        item.setVersion(uVisionVersion);
        item.setEngineType(UvscEngineType);
        item.setUnexpandedDisplayName(uniqueDisplayName(
            Tr::tr("Auto-detected uVision at \"%1\"").arg(uVision.toUserOutput())));
        appendItem(item);
    }
}

QString DebuggerModel::uniqueDisplayName(const QString &base)
{
    for (const DebuggerItem &item : volatileItems())
        if (item.unexpandedDisplayName() == base)
            return uniqueDisplayName(base + " (1)");
    return base;
}

QVariant DebuggerModel::registerDebugger(const DebuggerItem &item)
{
    // Try re-using existing item first.
    for (const DebuggerItem &d : volatileItems()) {
        if (d.command() == item.command()
            && d.detectionSource().isAutoDetected() == item.detectionSource().isAutoDetected()
            && d.engineType() == item.engineType()
            && d.unexpandedDisplayName() == item.unexpandedDisplayName()
            && d.abis() == item.abis())
            return d.id();
    }

    // If item already has an id, use it. Otherwise, create a new id.
    DebuggerItem di = item;
    if (!di.id().isValid())
        di.createId();

    appendItem(di);
    return di.id();
}

void DebuggerModel::deregisterDebugger(const QVariant &id)
{
    for (int i = 0; i < itemCount(); ++i) {
        if (item(i).id() == id) {
            removeItem(i);
            return;
        }
    }
}

void DebuggerModel::readDebuggers(const FilePath &fileName, bool isSdk)
{
    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return;
    Store data = reader.restoreValues();

    // Check version
    int version = data.value(DEBUGGER_FILE_VERSION_KEY, 0).toInt();
    if (version < 1)
        return;

    int count = data.value(DEBUGGER_COUNT_KEY, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const Key key = numberedKey(DEBUGGER_DATA_KEY, i);
        if (!data.contains(key))
            continue;
        const Store dbMap = storeFromVariant(data.value(key));
        DebuggerItem item(dbMap);
        if (isSdk) {
            item.setDetectionSource(DetectionSource::FromSdk);
            // SDK debuggers are always considered to be up-to-date, so no need to recheck them.
        } else {
            // User settings.
            if (item.detectionSource().isAutoDetected()) {
                if (!item.isValid() || item.engineType() == NoEngineType) {
                    qWarning() << QString("DebuggerItem \"%1\" (%2) read from \"%3\" dropped since it is not valid.")
                                  .arg(item.command().toUserOutput(), item.id().toString(), fileName.toUserOutput());
                    continue;
                }
                // FIXME: During startup, devices are not yet available, so we cannot check if the file still exists.
                if (item.command().isLocal() && !item.command().isExecutableFile()) {
                    qWarning() << QString("DebuggerItem \"%1\" (%2) read from \"%3\" dropped since the command is not executable.")
                                  .arg(item.command().toUserOutput(), item.id().toString(), fileName.toUserOutput());
                    continue;
                }
            }
        }
        registerDebugger(item);
    }
}

void DebuggerModel::restoreDebuggers()
{
    // Read debuggers from SDK
    readDebuggers(ICore::installerResourcePath(DEBUGGER_FILENAME), true);

    // Read all debuggers from user file.
    readDebuggers(userSettingsFileName(), false);

    // Auto detect current.
    const IDeviceConstPtr desktopDevice = DeviceManager::defaultDesktopDevice();
    if (QTC_GUARD(desktopDevice))
        detectDebuggers(desktopDevice, desktopDevice->systemEnvironment().path());
}

void DebuggerModel::detectDebuggers(const IDeviceConstPtr &device, const FilePaths &searchPaths)
{
    QTC_ASSERT(device, return);

    autoDetectGdbOrLldbDebuggers(searchPaths, {});
    if (device->id() == ProjectExplorer::Constants::DESKTOP_DEVICE_ID) {
        autoDetectCdbDebuggers();
        autoDetectUvscDebuggers();
    }
}

void DebuggerModel::saveDebuggers()
{
    Store data;
    data.insert(DEBUGGER_FILE_VERSION_KEY, 1);

    int count = 0;
    for (const DebuggerItem &item : items()) {
        if (item.detectionSource().isTemporary())
            continue;
        if (item.isGeneric()) // do not store generic debuggers, these get added automatically
            continue;
        if (item.isValid() && item.engineType() != NoEngineType) {
            Store tmp = item.toMap();
            if (!tmp.isEmpty()) {
                data.insert(numberedKey(DEBUGGER_DATA_KEY, count), variantFromStore(tmp));
                ++count;
            }
        }
    }
    data.insert(DEBUGGER_COUNT_KEY, count);
    m_writer.save(data);

    // Do not save default debuggers as they are set by the SDK.
}

using ExecutableItem = QtTaskTree::ExecutableItem; // trick lupdate, QTBUG-140636

ExecutableItem autoDetectDebuggerRecipe(
    ProjectExplorer::Kit *kit,
    const Utils::FilePaths &searchPaths,
    const DetectionSource &detectionSource,
    const LogCallback &logCallback)
{
    QStringList searchFilters
        = {"gdb-i686-pc-mingw32",
           "gdb-i686-pc-mingw32.exe",
           "gdb",
           "gdb.exe",
           "lldb",
           "lldb.exe",
           "lldb-[1-9]*",
           "arm-none-eabi-gdb-py.exe",
           "*-*-*-gdb"};

    if (nativeDapDebuggersEnabled()) {
        searchFilters.append({
            "lldb-dap",
            "lldb-dap.exe",
            "lldb-dap-*",
            // LLDB DAP server was named lldb-vscode prior LLVM 18.0.0
            "lldb-vscode",
            "lldb-vscode.exe",
            "lldb-vscode-*",
        });
    }

    static const auto searchDebuggers = [](QPromise<DebuggerItem> &promise,
                                           const FilePaths &searchPaths,
                                           const DetectionSource &detectionSource,
                                           const QStringList &searchFilters) {
        FilePaths suspects;

        for (const FilePath &path : searchPaths)
            suspects.append(path.dirEntries({searchFilters, QDir::Files | QDir::Executable}));

        for (const FilePath &command : std::as_const(suspects)) {
            const Result<DebuggerItem> item = makeAutoDetectedDebuggerItem(command, detectionSource);

            if (item)
                promise.addResult(*item);
            else
                qWarning() << "Failed to auto-detect debugger from" << command.toUserOutput() << ":"
                           << item.error();
        }
    };

    auto setupSearch = [searchPaths, detectionSource, searchFilters](Async<DebuggerItem> &async) {
        async.setConcurrentCallData(searchDebuggers, searchPaths, detectionSource, searchFilters);
    };

    auto searchDone = [kit, logCallback](const Async<DebuggerItem> &async) {
        QList<DebuggerItem> items = async.results();
        for (const DebuggerItem &item : items) {
            if (item.isValid() && item.engineType() != NoEngineType) {
                logCallback(Tr::tr("Found debugger: \"%1\".").arg(item.command().toUserOutput()));
                DebuggerItemManager::registerDebugger(item);
                DebuggerKitAspect::setDebugger(kit, item.id());
            } else
                qWarning() << "Invalid debugger item detected?!";
        }
    };

    return AsyncTask<DebuggerItem>(setupSearch, searchDone);
}

ExecutableItem removeAutoDetected(const QString &detectionSourceId, const LogCallback &logCallback)
{
    return QSyncTask([detectionSourceId, logCallback]() {
        const auto debuggers = filtered(
            DebuggerItemManager::debuggers(), [detectionSourceId](const DebuggerItem &item) {
                return item.detectionSource().id == detectionSourceId;
            });

        for (const auto &debugger : debuggers) {
            logCallback(Tr::tr("Removing debugger: \"%1\".").arg(debugger.displayName()));
            DebuggerItemManager::deregisterDebugger(debugger.id());
        }
    });
}

Utils::Result<ExecutableItem> createAspectFromJson(
    const DetectionSource &detectionSource,
    const Utils::FilePath &rootPath,
    ProjectExplorer::Kit *kit,
    const QJsonValue &json,
    const ProjectExplorer::LogCallback &logCallback)
{
    if (!json.isString())
        return ResultError(Tr::tr("Invalid JSON value for debugger: \"%1\".").arg(json.toString()));

    const FilePath command = rootPath.withNewPath(json.toString());

    if (command.isEmpty())
        return ResultError(Tr::tr("Empty command for debugger."));

    const auto setup = [command, detectionSource, logCallback](Async<Result<DebuggerItem>> &async) {
        async.setConcurrentCallData(
            [](QPromise<Result<DebuggerItem>> &promise,
               const FilePath &command,
               const DetectionSource &detectionSource) {
                promise.addResult(makeAutoDetectedDebuggerItem(command, detectionSource));
            },
            command,
            detectionSource);
    };

    const auto registerDebugger = [kit, logCallback](const Async<Result<DebuggerItem>> &async) {
        Result<DebuggerItem> item = async.result();
        if (!item) {
            logCallback(Tr::tr("Failed to create debugger from JSON: %1").arg(item.error()));
            return;
        }

        DebuggerItemManager::registerDebugger(*item);
        DebuggerKitAspect::setDebugger(kit, item->id());
    };

    return AsyncTask<Result<DebuggerItem>>(setup, registerDebugger);
}

} // namespace Internal

// --------------------------------------------------------------------------
// DebuggerItemManager
// --------------------------------------------------------------------------

void DebuggerItemManager::restoreDebuggers()
{
    NANOTRACE_SCOPE("Debugger", "DebuggerItemManager::restoreDebuggers");
    debuggerModel().restoreDebuggers();
}

const QList<DebuggerItem> DebuggerItemManager::debuggers()
{
    QList<DebuggerItem> result;
    for (const DebuggerItem &item : debuggerModel().items())
        result.append(item);
    return result;
}

DebuggerItem DebuggerItemManager::findByCommand(const FilePath &command)
{
    for (const DebuggerItem &item : debuggerModel().volatileItems())
        if (item.command() == command)
            return item;
    return {};
}

DebuggerItem DebuggerItemManager::findById(const QVariant &id)
{
    for (const DebuggerItem &item : debuggerModel().volatileItems())
        if (item.id() == id)
            return item;
    return {};
}

DebuggerItem DebuggerItemManager::findByEngineType(DebuggerEngineType engineType)
{
    for (const DebuggerItem &item : debuggerModel().volatileItems())
        if (item.engineType() == engineType)
            return item;
    return {};
}

QVariant DebuggerItemManager::registerDebugger(const DebuggerItem &item)
{
    return debuggerModel().registerDebugger(item);
}

void DebuggerItemManager::deregisterDebugger(const QVariant &id)
{
    return debuggerModel().deregisterDebugger(id);
}

// DebuggerSettingsPageWidget

class DebuggerSettingsPageWidget : public IOptionsPageWidget
{
public:
    DebuggerSettingsPageWidget()
    {
        m_addButton = new QPushButton(Tr::tr("Add"), this);

        m_cloneButton = new QPushButton(Tr::tr("Clone"), this);
        m_cloneButton->setEnabled(false);

        m_delButton = new QPushButton(this);
        m_delButton->setEnabled(false);

        m_detectButton = new QPushButton(Tr::tr("Re-detect"), this);

        m_container = new DetailsWidget(this);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        m_deviceModel.showAllEntry();

        m_deviceComboBox = new QComboBox(this);
        setIgnoreForDirtyHook(m_deviceComboBox);
        m_deviceComboBox->setModel(&m_deviceModel);

        m_groupedView.view().setSortingEnabled(true);
        m_groupedView.view().sortByColumn(0, Qt::AscendingOrder);

        auto header = m_groupedView.view().header();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(2, QHeaderView::Stretch);

        auto buttonLayout = new QVBoxLayout();
        buttonLayout->setSpacing(6);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_cloneButton);
        buttonLayout->addWidget(m_delButton);
        buttonLayout->addWidget(m_detectButton);
        buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        const auto deviceLayout = new QHBoxLayout;
        deviceLayout->addWidget(new QLabel(Tr::tr("Device:")));
        deviceLayout->addWidget(m_deviceComboBox);
        deviceLayout->addStretch(1);

        auto debuggersLayout = new QVBoxLayout();
        debuggersLayout->addWidget(&m_groupedView.view());
        debuggersLayout->addWidget(m_container);

        auto horizontalLayout = new QHBoxLayout;
        horizontalLayout->addLayout(debuggersLayout);
        horizontalLayout->addLayout(buttonLayout);

        const auto mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(deviceLayout);
        mainLayout->addLayout(horizontalLayout);

        connect(&m_groupedView, &GroupedView::currentRowChanged,
                this, [this](int, int) { updateButtons(); }, Qt::QueuedConnection);

        connect(m_addButton, &QAbstractButton::clicked,
                this, &DebuggerSettingsPageWidget::addDebugger, Qt::QueuedConnection);
        connect(m_cloneButton, &QAbstractButton::clicked,
                this, &DebuggerSettingsPageWidget::cloneDebugger, Qt::QueuedConnection);
        connect(m_delButton, &QAbstractButton::clicked,
                this, &DebuggerSettingsPageWidget::removeDebugger, Qt::QueuedConnection);
        connect(m_detectButton , &QAbstractButton::clicked,
                this, [this] {
            QList<IDeviceConstPtr> devices;
            if (const IDeviceConstPtr dev = currentDevice()) {
                devices << dev;
            } else {
                for (int i = 0; i < DeviceManager::deviceCount(); ++i)
                    devices << DeviceManager::deviceAt(i);
            }
            for (const IDeviceConstPtr &dev : std::as_const(devices))
                debuggerModel().detectDebuggers(dev, dev->toolSearchPaths());
        });

        m_deviceComboBox->setCurrentIndex(0);
        const auto updateDevice = [this](int idx) {
            const IDeviceConstPtr device = m_deviceModel.device(idx);
            const FilePath deviceRoot = device ? device->rootPath() : FilePath{};
            debuggerModel().setExtraFilter(deviceRoot.isEmpty()
                ? GroupedModel::Filter{}
                : GroupedModel::Filter{[deviceRoot](int row) {
                      const FilePath path = debuggerModel().item(row).command();
                      return path.isEmpty() || path.isSameDevice(deviceRoot);
                  }});
        };
        connect(m_deviceComboBox, &QComboBox::currentIndexChanged, this, updateDevice);
        updateDevice(m_deviceComboBox->currentIndex());

        m_itemConfigWidget = new DebuggerItemConfigWidget;
        m_container->setWidget(m_itemConfigWidget);
        updateButtons();

        installMarkSettingsDirtyTriggerRecursively(this);
    }

    void apply() final
    {
        m_itemConfigWidget->store();
        debuggerModel().apply();
        m_groupedView.view().expandAll();
    }

    void cancel() final
    {
        debuggerModel().cancel();
        m_groupedView.view().expandAll();
    }

    IDeviceConstPtr currentDevice() const;

    void cloneDebugger();
    void addDebugger();
    void removeDebugger();
    void updateButtons();

    DeviceManagerModel m_deviceModel;
    QComboBox *m_deviceComboBox;
    GroupedView m_groupedView{debuggerModel()};
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    QPushButton *m_detectButton;
    DetailsWidget *m_container;
    DebuggerItemConfigWidget *m_itemConfigWidget;
};

IDeviceConstPtr DebuggerSettingsPageWidget::currentDevice() const
{
    return m_deviceModel.device(m_deviceComboBox->currentIndex());
}

void DebuggerSettingsPageWidget::cloneDebugger()
{
    const int row = m_groupedView.currentRow();
    if (row < 0)
        return;
    const DebuggerItem item = debuggerModel().item(row);
    if (!item)
        return;

    DebuggerItem newItem;
    newItem.createId();
    newItem.setCommand(item.command());
    newItem.setUnexpandedDisplayName(debuggerModel().uniqueDisplayName(Tr::tr("Clone of %1").arg(item.displayName())));
    newItem.reinitializeFromFile();
    newItem.setDetectionSource({DetectionSource::Manual, item.detectionSource().id});
    newItem.setEngineType(item.engineType());
    m_groupedView.selectRow(debuggerModel().appendVolatileItem(newItem));
    markSettingsDirty();
}

void DebuggerSettingsPageWidget::addDebugger()
{
    DebuggerItem item;
    item.createId();
    item.setEngineType(NoEngineType);
    item.setUnexpandedDisplayName(debuggerModel().uniqueDisplayName(Tr::tr("New Debugger")));
    m_groupedView.selectRow(debuggerModel().appendVolatileItem(item));
    markSettingsDirty();
}

void DebuggerSettingsPageWidget::removeDebugger()
{
    const int row = m_groupedView.currentRow();
    QTC_ASSERT(row >= 0, return);
    debuggerModel().markRemoved(row);
    updateButtons();
    markSettingsDirty(); // TODO restore should maybe undo dirtying
}

void DebuggerSettingsPageWidget::updateButtons()
{
    const int row = m_groupedView.currentRow();
    const DebuggerItem item = row >= 0 ? debuggerModel().item(row) : DebuggerItem{};

    m_itemConfigWidget->load(item);
    const IDeviceConstPtr dev = currentDevice();
    if (dev)
        m_itemConfigWidget->setDeviceRootPath(dev->rootPath());
    m_container->setVisible(bool(item));
    m_cloneButton->setEnabled(item && item.canClone());
    m_delButton->setEnabled(item && !item.detectionSource().isAutoDetected());
    m_delButton->setText(item && debuggerModel().isRemoved(row) ? Tr::tr("Restore") : Tr::tr("Remove"));
}


// DebuggerSettingsPage

class DebuggerSettingsPage : public Core::IOptionsPage
{
public:
    DebuggerSettingsPage() {
        setId(ProjectExplorer::Constants::DEBUGGER_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("Debuggers"));
        setCategory(ProjectExplorer::Constants::KITS_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new DebuggerSettingsPageWidget; });
    }
};

const DebuggerSettingsPage settingsPage;

} // namespace Debugger
