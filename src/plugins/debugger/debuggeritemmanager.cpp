// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggeritemmanager.h"

#include "debuggeritem.h"
#include "debuggertr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitoptionspage.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/winutils.h>

#include <nanotrace/nanotrace.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QTreeView>
#include <QWidget>

using namespace Debugger::Internal;
using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

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

    void load(const DebuggerItem *item);
    void store() const;

private:
    void binaryPathHasChanged();
    DebuggerItem item() const;
    void setAbis(const QStringList &abiNames);

    QLineEdit *m_displayNameLineEdit;
    QLabel *m_cdbLabel;
    PathChooser *m_binaryChooser;
    bool m_autodetected = false;
    bool m_generic = false;
    DebuggerEngineType m_engineType = NoEngineType;
    QVariant m_id;

    QLabel *m_abis;
    QLabel *m_version;
    QLabel *m_type;

    PathChooser *m_workingDirectoryChooser;
    QFutureWatcher<DebuggerItem> m_updateWatcher;
};

// --------------------------------------------------------------------------
// DebuggerTreeItem
// --------------------------------------------------------------------------

class DebuggerTreeItem : public TreeItem
{
public:
    DebuggerTreeItem(const DebuggerItem &item, bool changed)
        : m_item(item), m_orig(item), m_added(changed), m_changed(changed)
    {}

    QVariant data(int column, int role) const override
    {
        switch (role) {
            case Qt::DisplayRole:
                switch (column) {
                case 0: return m_item.displayName();
                case 1: return m_item.command().toUserOutput();
                case 2: return m_item.engineTypeName();
                }
                break;

            case Qt::FontRole: {
                QFont font;
                if (m_changed)
                    font.setBold(true);
                if (m_removed)
                    font.setStrikeOut(true);
                return font;
            }

            case Qt::DecorationRole:
                if (column == 0)
                    return m_item.decoration();
                break;

            case Qt::ToolTipRole:
                return m_item.validityMessage();
        }
        return QVariant();
    }

    DebuggerItem m_item; // Displayed, possibly unapplied data.
    DebuggerItem m_orig; // Stored original data.
    bool m_added;
    bool m_changed;
    bool m_removed = false;
};

// --------------------------------------------------------------------------
// DebuggerItemModel
// --------------------------------------------------------------------------

class DebuggerItemModel : public TreeModel<TreeItem, StaticTreeItem, DebuggerTreeItem>
{
public:
    DebuggerItemModel();
    enum { Generic, AutoDetected, Manual };

    QModelIndex lastIndex() const;
    void setCurrentIndex(const QModelIndex &index);
    DebuggerTreeItem *addDebuggerItem(const DebuggerItem &item, bool changed = false);
    void updateDebugger(const DebuggerItem &item);
    void apply();
    void cancel();
    DebuggerTreeItem *currentTreeItem();

    void restoreDebuggers();
    void saveDebuggers();

    void addDebugger(const DebuggerItem &item);
    QVariant registerDebugger(const DebuggerItem &item);
    void readDebuggers(const FilePath &fileName, bool isSystem);
    void autoDetectCdbDebuggers();
    void autoDetectGdbOrLldbDebuggers(const FilePaths &searchPaths,
                                      const QString &detectionSource,
                                      QString *logMessage = nullptr);
    void autoDetectUvscDebuggers();
    QString uniqueDisplayName(const QString &base);

    PersistentSettingsWriter m_writer{userSettingsFileName(), "QtCreatorDebuggers"};
    QPersistentModelIndex m_currentIndex;
};

static DebuggerItemModel &itemModel()
{
    static DebuggerItemModel theModel;
    return theModel;
}

template <typename Predicate>
void forAllDebuggers(const Predicate &pred)
{
    itemModel().forItemsAtLevel<2>([pred](DebuggerTreeItem *titem) {
        pred(titem->m_item);
    });
}

template <typename Predicate>
const DebuggerItem *findDebugger(const Predicate &pred)
{
    DebuggerTreeItem *titem = itemModel().findItemAtLevel<2>([pred](DebuggerTreeItem *titem) {
        return pred(titem->m_item);
    });
    return titem ? &titem->m_item : nullptr;
}

static QString genericCategoryDisplayName() { return Tr::tr("Generic"); }

DebuggerItemModel::DebuggerItemModel()
{
    setHeader({Tr::tr("Name"), Tr::tr("Path"), Tr::tr("Type")});

    auto generic = new StaticTreeItem(genericCategoryDisplayName());
    auto autoDetected = new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                                           {ProjectExplorer::Constants::msgAutoDetectedToolTip()});
    rootItem()->appendChild(generic);
    rootItem()->appendChild(autoDetected);
    rootItem()->appendChild(new StaticTreeItem(ProjectExplorer::Constants::msgManual()));

    DebuggerItem genericGdb(QVariant("gdb"));
    genericGdb.setAutoDetected(true);
    genericGdb.setGeneric(true);
    genericGdb.setEngineType(GdbEngineType);
    genericGdb.setAbi(Abi());
    genericGdb.setCommand("gdb");
    genericGdb.setUnexpandedDisplayName(Tr::tr("GDB from PATH on Build Device"));
    generic->appendChild(new DebuggerTreeItem(genericGdb, false));

    DebuggerItem genericLldb(QVariant("lldb"));
    genericLldb.setAutoDetected(true);
    genericLldb.setEngineType(LldbEngineType);
    genericLldb.setGeneric(true);
    genericLldb.setAbi(Abi());
    genericLldb.setCommand("lldb");
    genericLldb.setUnexpandedDisplayName(Tr::tr("LLDB from PATH on Build Device"));
    generic->appendChild(new DebuggerTreeItem(genericLldb, false));

    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &DebuggerItemModel::saveDebuggers);
}

DebuggerTreeItem *DebuggerItemModel::addDebuggerItem(const DebuggerItem &item, bool changed)
{
    QTC_ASSERT(item.id().isValid(), return {});
    int group = item.isGeneric() ? Generic : (item.isAutoDetected() ? AutoDetected : Manual);
    auto treeItem = new DebuggerTreeItem(item, changed);
    rootItem()->childAt(group)->appendChild(treeItem);
    return treeItem;
}

void DebuggerItemModel::updateDebugger(const DebuggerItem &item)
{
    auto matcher = [item](DebuggerTreeItem *n) { return n->m_item.m_id == item.id(); };
    DebuggerTreeItem *treeItem = findItemAtLevel<2>(matcher);
    QTC_ASSERT(treeItem, return);

    TreeItem *parent = treeItem->parent();
    QTC_ASSERT(parent, return);

    treeItem->m_changed = treeItem->m_orig != item;
    treeItem->m_item = item;
    treeItem->update(); // Notify views.
}

QModelIndex DebuggerItemModel::lastIndex() const
{
    TreeItem *manualGroup = rootItem()->lastChild();
    TreeItem *lastItem = manualGroup->lastChild();
    return lastItem ? indexForItem(lastItem) : QModelIndex();
}

void DebuggerItemModel::apply()
{
    QList<DebuggerTreeItem *> toRemove;
    forItemsAtLevel<2>([&toRemove](DebuggerTreeItem *titem) {
        titem->m_added = false;
        if (titem->m_changed) {
            titem->m_changed = false;
            titem->m_orig = titem->m_item;
        }
        if (titem->m_removed)
            toRemove.append(titem);
    });
    for (DebuggerTreeItem *titem : toRemove)
        destroyItem(titem);
}

void DebuggerItemModel::cancel()
{
    QList<DebuggerTreeItem *> toRemove;
    forItemsAtLevel<2>([&toRemove](DebuggerTreeItem *titem) {
        titem->m_removed = false;
        if (titem->m_changed) {
            titem->m_changed = false;
            titem->m_item = titem->m_orig;
        }
        if (titem->m_added)
            toRemove.append(titem);
    });
    for (DebuggerTreeItem *titem : toRemove)
        destroyItem(titem);
}

void DebuggerItemModel::setCurrentIndex(const QModelIndex &index)
{
    m_currentIndex = index;
}

DebuggerTreeItem *DebuggerItemModel::currentTreeItem()
{
    TreeItem *treeItem = itemForIndex(m_currentIndex);
    return treeItem && treeItem->level() == 2 ? static_cast<DebuggerTreeItem *>(treeItem) : nullptr;
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

    connect(&m_updateWatcher, &QFutureWatcher<DebuggerItem>::finished, this, [this] {
        if (m_updateWatcher.future().resultCount() > 0) {
            DebuggerItem tmp = m_updateWatcher.result();
            setAbis(tmp.abiNames());
            m_version->setText(tmp.version());
            m_engineType = tmp.engineType();
            m_type->setText(tmp.engineTypeName());
        }
    });

    // clang-format off
    using namespace Layouting;
    Form {
        fieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow),
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
    item.setAutoDetected(m_autodetected);
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
    item.setGeneric(m_generic);
    return item;
}

void DebuggerItemConfigWidget::store() const
{
    if (!m_id.isNull())
        itemModel().updateDebugger(item());
}

void DebuggerItemConfigWidget::setAbis(const QStringList &abiNames)
{
    m_abis->setText(abiNames.join(", "));
}

void DebuggerItemConfigWidget::load(const DebuggerItem *item)
{
    m_id = QVariant(); // reset Id to avoid intermediate signal handling
    if (!item)
        return;

    // Set values:
    m_generic = item->isGeneric();
    m_autodetected = item->isAutoDetected();

    m_displayNameLineEdit->setEnabled(!item->isAutoDetected());
    m_displayNameLineEdit->setText(item->unexpandedDisplayName());

    m_type->setText(item->engineTypeName());

    m_binaryChooser->setReadOnly(item->isAutoDetected());
    m_binaryChooser->setFilePath(item->command());
    m_binaryChooser->setExpectedKind(m_generic ? PathChooser::Any : PathChooser::ExistingCommand);

    m_workingDirectoryChooser->setReadOnly(item->isAutoDetected());
    m_workingDirectoryChooser->setFilePath(item->workingDirectory());

    QString text;
    QString versionCommand;
    if (item->engineType() == CdbEngineType) {
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
    m_version->setText(item->version());
    setAbis(item->abiNames());
    m_engineType = item->engineType();
    m_id = item->id();
}

void DebuggerItemConfigWidget::binaryPathHasChanged()
{
    // Ignore change if this is no valid DebuggerItem
    if (!m_id.isValid())
        return;

    if (!m_generic) {
        m_updateWatcher.cancel();

        DebuggerItem tmp;
        if (m_binaryChooser->filePath().isExecutableFile()) {
            tmp = item();
            m_updateWatcher.setFuture(Utils::asyncRun([tmp]() mutable {
                tmp.reinitializeFromFile();
                return tmp;
            }));
        } else {
            setAbis(tmp.abiNames());
            m_version->setText(tmp.version());
            m_engineType = tmp.engineType();
            m_type->setText(tmp.engineTypeName());
        }
    }

    store();
}

void DebuggerItemModel::autoDetectCdbDebuggers()
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
        if (HostOsInfo::hostArchitecture() == HostOsInfo::HostArchitectureArm64)
            abis << "arm64";
        for (const QString &abi: abis) {
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
        item.setAutoDetected(true);
        item.setAbis(Abi::abisOfBinary(cdb));
        item.setCommand(cdb);
        item.setEngineType(CdbEngineType);
        item.setUnexpandedDisplayName(uniqueDisplayName(Tr::tr("Auto-detected CDB at %1").arg(cdb.toUserOutput())));
        item.reinitializeFromFile(); // collect version number
        addDebuggerItem(item);
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

void DebuggerItemModel::autoDetectGdbOrLldbDebuggers(const FilePaths &searchPaths,
                                                              const QString &detectionSource,
                                                              QString *logMessage)
{
    const QStringList filters = {"gdb-i686-pc-mingw32", "gdb-i686-pc-mingw32.exe", "gdb",
                                 "gdb.exe", "lldb", "lldb.exe", "lldb-[1-9]*",
                                 "arm-none-eabi-gdb-py.exe", "*-*-*-gdb"};

    if (searchPaths.isEmpty())
        return;

    FilePaths suspects;

    if (searchPaths.front().osType() == OsTypeMac) {
        Process proc;
        proc.setTimeoutS(2);
        proc.setCommand({"xcrun", {"--find", "lldb"}});
        proc.runBlocking();
        // FIXME:
        if (proc.result() == ProcessResult::FinishedWithSuccess) {
            QString lPath = proc.allOutput().trimmed();
            if (!lPath.isEmpty()) {
                const QFileInfo fi(lPath);
                if (fi.exists() && fi.isExecutable() && !fi.isDir())
                    suspects.append(FilePath::fromString(fi.absoluteFilePath()));
            }
        }
    }

    FilePaths paths = searchPaths;
    if (!searchPaths.front().needsDevice())
        paths.append(searchGdbPathsFromRegistry());

    paths = Utils::filteredUnique(paths);

    const auto addSuspect = [&suspects](const FilePath &entry) {
        suspects.append(entry);
        return IterationPolicy::Continue;
    };
    for (const FilePath &path : paths)
        path.iterateDirectory(addSuspect, {filters, QDir::Files | QDir::Executable});

    QStringList logMessages{Tr::tr("Searching debuggers...")};
    for (const FilePath &command : std::as_const(suspects)) {
        const auto commandMatches = [command](const DebuggerTreeItem *titem) {
            return titem->m_item.command() == command;
        };
        if (DebuggerTreeItem *existingItem = findItemAtLevel<2>(commandMatches)) {
            if (command.lastModified() != existingItem->m_item.lastModified())
                existingItem->m_item.reinitializeFromFile();
            continue;
        }
        DebuggerItem item;
        item.createId();
        item.setDetectionSource(detectionSource);
        item.setAutoDetected(true);
        item.setCommand(command);
        item.reinitializeFromFile();
        if (item.engineType() == NoEngineType)
            continue;
        //: %1: Debugger engine type (GDB, LLDB, CDB...), %2: Path
        const QString name = detectionSource.isEmpty() ? Tr::tr("System %1 at %2") : Tr::tr("Detected %1 at %2");
        item.setUnexpandedDisplayName(name.arg(item.engineTypeName()).arg(command.toUserOutput()));
        addDebuggerItem(item);
        logMessages.append(Tr::tr("Found: \"%1\"").arg(command.toUserOutput()));
    }
    if (logMessage)
        *logMessage = logMessages.join('\n');
}

void DebuggerItemModel::autoDetectUvscDebuggers()
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
                    WinDLLFileVersion, uVision.toString(), &errorMsg);

        DebuggerItem item;
        item.createId();
        item.setAutoDetected(true);
        item.setCommand(uVision);
        item.setVersion(uVisionVersion);
        item.setEngineType(UvscEngineType);
        item.setUnexpandedDisplayName(
                    uniqueDisplayName(Tr::tr("Auto-detected uVision at %1")
                                      .arg(uVision.toUserOutput())));
        addDebuggerItem(item);
    }
}

QString DebuggerItemModel::uniqueDisplayName(const QString &base)
{
    const DebuggerItem *item = findDebugger([base](const DebuggerItem &item) {
        return item.unexpandedDisplayName() == base;
    });
    return item ? uniqueDisplayName(base + " (1)") : base;
}

QVariant DebuggerItemModel::registerDebugger(const DebuggerItem &item)
{
    // Try re-using existing item first.
    DebuggerTreeItem *titem = findItemAtLevel<2>([item](DebuggerTreeItem *titem) {
        const DebuggerItem &d = titem->m_item;
        return d.command() == item.command()
                && d.isAutoDetected() == item.isAutoDetected()
                && d.engineType() == item.engineType()
                && d.unexpandedDisplayName() == item.unexpandedDisplayName()
                && d.abis() == item.abis();
    });
    if (titem)
        return titem->m_item.id();

    // If item already has an id, use it. Otherwise, create a new id.
    DebuggerItem di = item;
    if (!di.id().isValid())
        di.createId();

    addDebuggerItem(di);
    return di.id();
}

void DebuggerItemModel::readDebuggers(const FilePath &fileName, bool isSystem)
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
        if (isSystem) {
            item.setAutoDetected(true);
            // SDK debuggers are always considered to be up-to-date, so no need to recheck them.
        } else {
            // User settings.
            if (item.isAutoDetected()) {
                if (!item.isValid() || item.engineType() == NoEngineType) {
                    qWarning() << QString("DebuggerItem \"%1\" (%2) read from \"%3\" dropped since it is not valid.")
                                  .arg(item.command().toUserOutput(), item.id().toString(), fileName.toUserOutput());
                    continue;
                }
                // FIXME: During startup, devices are not yet available, so we cannot check if the file still exists.
                if (!item.command().needsDevice() && !item.command().isExecutableFile()) {
                    qWarning() << QString("DebuggerItem \"%1\" (%2) read from \"%3\" dropped since the command is not executable.")
                                  .arg(item.command().toUserOutput(), item.id().toString(), fileName.toUserOutput());
                    continue;
                }
            }

        }
        registerDebugger(item);
    }
}

void DebuggerItemModel::restoreDebuggers()
{
    // Read debuggers from SDK
    readDebuggers(ICore::installerResourcePath(DEBUGGER_FILENAME), true);

    // Read all debuggers from user file.
    readDebuggers(userSettingsFileName(), false);

    // Auto detect current.
    IDevice::ConstPtr desktop = DeviceManager::defaultDesktopDevice();
    QTC_ASSERT(desktop, return);
    autoDetectGdbOrLldbDebuggers(desktop->systemEnvironment().path(), {});
    autoDetectCdbDebuggers();
    autoDetectUvscDebuggers();
}

void DebuggerItemModel::saveDebuggers()
{
    Store data;
    data.insert(DEBUGGER_FILE_VERSION_KEY, 1);

    int count = 0;
    forAllDebuggers([&count, &data](DebuggerItem &item) {
        if (item.isGeneric()) // do not store generic debuggers, these get added automatically
            return;
        if (item.isValid() && item.engineType() != NoEngineType) {
            Store tmp = item.toMap();
            if (!tmp.isEmpty()) {
                data.insert(numberedKey(DEBUGGER_DATA_KEY, count), variantFromStore(tmp));
                ++count;
            }
        }
    });
    data.insert(DEBUGGER_COUNT_KEY, count);
    m_writer.save(data, ICore::dialogParent());

    // Do not save default debuggers as they are set by the SDK.
}

} // namespace Internal

// --------------------------------------------------------------------------
// DebuggerItemManager
// --------------------------------------------------------------------------

void DebuggerItemManager::restoreDebuggers()
{
    NANOTRACE_SCOPE("Debugger", "DebuggerItemManager::restoreDebuggers");
    itemModel().restoreDebuggers();
}

const QList<DebuggerItem> DebuggerItemManager::debuggers()
{
    QList<DebuggerItem> result;
    forAllDebuggers([&result](const DebuggerItem &item) { result.append(item); });
    return result;
}

const DebuggerItem *DebuggerItemManager::findByCommand(const FilePath &command)
{
    return findDebugger([command](const DebuggerItem &item) {
        return item.command() == command;
    });
}

const DebuggerItem *DebuggerItemManager::findById(const QVariant &id)
{
    return findDebugger([id](const DebuggerItem &item) {
        return item.id() == id;
    });
}

const DebuggerItem *DebuggerItemManager::findByEngineType(DebuggerEngineType engineType)
{
    return findDebugger([engineType](const DebuggerItem &item) {
        return item.engineType() == engineType;
    });
}

QVariant DebuggerItemManager::registerDebugger(const DebuggerItem &item)
{
    return itemModel().registerDebugger(item);
}

void DebuggerItemManager::deregisterDebugger(const QVariant &id)
{
    itemModel().forItemsAtLevel<2>([id](DebuggerTreeItem *titem) {
        if (titem->m_item.id() == id)
            itemModel().destroyItem(titem);
    });
}

void DebuggerItemManager::autoDetectDebuggersForDevice(const FilePaths &searchPaths,
                                                       const QString &detectionSource,
                                                       QString *logMessage)
{
    itemModel().autoDetectGdbOrLldbDebuggers(searchPaths, detectionSource, logMessage);
}

void DebuggerItemManager::removeDetectedDebuggers(const QString &detectionSource,
                                                  QString *logMessage)
{
    QStringList logMessages{Tr::tr("Removing debugger entries...")};
    QList<DebuggerTreeItem *> toBeRemoved;

    itemModel().forItemsAtLevel<2>([detectionSource, &toBeRemoved](DebuggerTreeItem *titem) {
        if (titem->m_item.detectionSource() == detectionSource) {
            toBeRemoved.append(titem);
            return;
        }
        // FIXME: These items appeared in early docker development. Ok to remove for Creator 7.0.
        FilePath filePath = titem->m_item.command();
        if (filePath.scheme() + ':' + filePath.host() == detectionSource)
            toBeRemoved.append(titem);
    });
    for (DebuggerTreeItem *current : toBeRemoved) {
        logMessages.append(Tr::tr("Removed \"%1\"").arg(current->m_item.displayName()));
        itemModel().destroyItem(current);
    }

    if (logMessage)
        *logMessage = logMessages.join('\n');
}

void DebuggerItemManager::listDetectedDebuggers(const QString &detectionSource, QString *logMessage)
{
    QTC_ASSERT(logMessage, return);
    QStringList logMessages{Tr::tr("Debuggers:")};
    itemModel().forItemsAtLevel<2>([detectionSource, &logMessages](DebuggerTreeItem *titem) {
        if (titem->m_item.detectionSource() == detectionSource)
            logMessages.append(titem->m_item.displayName());
    });
    *logMessage = logMessages.join('\n');
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

        m_container = new DetailsWidget(this);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        m_sortModel = new KitSettingsSortModel(this);
        m_sortModel->setSourceModel(&itemModel());
        m_sortModel->setSortedCategories({genericCategoryDisplayName(),
                                          ProjectExplorer::Constants::msgAutoDetected(),
                                          ProjectExplorer::Constants::msgManual()});
        m_debuggerView = new QTreeView(this);
        m_debuggerView->setModel(m_sortModel);
        m_debuggerView->setUniformRowHeights(true);
        m_debuggerView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_debuggerView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_debuggerView->expandAll();
        m_debuggerView->setSortingEnabled(true);
        m_debuggerView->sortByColumn(0, Qt::AscendingOrder);

        auto header = m_debuggerView->header();
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
        buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        auto verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(m_debuggerView);
        verticalLayout->addWidget(m_container);

        auto horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->addLayout(verticalLayout);
        horizontalLayout->addLayout(buttonLayout);

        connect(m_debuggerView->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &DebuggerSettingsPageWidget::currentDebuggerChanged, Qt::QueuedConnection);

        connect(m_addButton, &QAbstractButton::clicked,
                this, &DebuggerSettingsPageWidget::addDebugger, Qt::QueuedConnection);
        connect(m_cloneButton, &QAbstractButton::clicked,
                this, &DebuggerSettingsPageWidget::cloneDebugger, Qt::QueuedConnection);
        connect(m_delButton, &QAbstractButton::clicked,
                this, &DebuggerSettingsPageWidget::removeDebugger, Qt::QueuedConnection);

        m_itemConfigWidget = new DebuggerItemConfigWidget;
        m_container->setWidget(m_itemConfigWidget);
        updateButtons();
    }

    void apply() final
    {
        m_itemConfigWidget->store();
        itemModel().apply();
    }

    void finish() final
    {
        itemModel().cancel();
    }

    void cloneDebugger();
    void addDebugger();
    void removeDebugger();
    void currentDebuggerChanged(const QModelIndex &newCurrent);
    void updateButtons();

    KitSettingsSortModel *m_sortModel;
    QTreeView *m_debuggerView;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    DetailsWidget *m_container;
    DebuggerItemConfigWidget *m_itemConfigWidget;
};

void DebuggerSettingsPageWidget::cloneDebugger()
{
    DebuggerTreeItem *treeItem = itemModel().currentTreeItem();
    if (!treeItem)
        return;

    DebuggerItem *item = &treeItem->m_item;
    DebuggerItem newItem;
    newItem.createId();
    newItem.setCommand(item->command());
    newItem.setUnexpandedDisplayName(itemModel().uniqueDisplayName(Tr::tr("Clone of %1").arg(item->displayName())));
    newItem.reinitializeFromFile();
    newItem.setAutoDetected(false);
    newItem.setGeneric(item->isGeneric());
    newItem.setEngineType(item->engineType());
    auto addedItem = itemModel().addDebuggerItem(newItem, true);
    m_debuggerView->setCurrentIndex(m_sortModel->mapFromSource(itemModel().indexForItem(addedItem)));
}

void DebuggerSettingsPageWidget::addDebugger()
{
    DebuggerItem item;
    item.createId();
    item.setEngineType(NoEngineType);
    item.setUnexpandedDisplayName(itemModel().uniqueDisplayName(Tr::tr("New Debugger")));
    item.setAutoDetected(false);
    auto addedItem = itemModel().addDebuggerItem(item, true);
    m_debuggerView->setCurrentIndex(m_sortModel->mapFromSource(itemModel().indexForItem(addedItem)));
}

void DebuggerSettingsPageWidget::removeDebugger()
{
    DebuggerTreeItem *treeItem = itemModel().currentTreeItem();
    QTC_ASSERT(treeItem, return);
    treeItem->m_removed = !treeItem->m_removed;
    treeItem->update();
    updateButtons();
}

void DebuggerSettingsPageWidget::currentDebuggerChanged(const QModelIndex &newCurrent)
{
    itemModel().setCurrentIndex(m_sortModel->mapToSource(newCurrent));
    updateButtons();
}

void DebuggerSettingsPageWidget::updateButtons()
{
    DebuggerTreeItem *titem = itemModel().currentTreeItem();
    DebuggerItem *item = titem ? &titem->m_item : nullptr;

    m_itemConfigWidget->load(item);
    m_container->setVisible(item != nullptr);
    m_cloneButton->setEnabled(item && item->isValid() && item->canClone());
    m_delButton->setEnabled(item && !item->isAutoDetected());
    m_delButton->setText(item && titem->m_removed ? Tr::tr("Restore") : Tr::tr("Remove"));
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
