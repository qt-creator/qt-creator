// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakekitaspect.h"

#include "cmakeconfigitem.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmaketool.h"
#include "cmaketoolmanager.h"

#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>

#include <ios/iosconstants.h>

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/commandline.h>
#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/guard.h>
#include <utils/guiutils.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGuiApplication>
#include <QJsonArray>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>

#ifdef WITH_TESTS
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#endif

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

static FilePath deviceCMake(const IDevice::ConstPtr &device)
{
    return device ? device->deviceToolPath(Constants::CMAKE_TOOL_ID) : FilePath();
}

// The device's own detection searches the PATH, too.
static bool isFoundOnPath(const CMakeTool &tool)
{
    const DetectionSource source = tool.detectionSource();
    return source.isAutoDetected() && !source.isSdkProvided();
}

static FilePath cmakeForNewKit(const IDevice::ConstPtr &device)
{
    if (!device)
        return {};
    if (const FilePath fromDevice = deviceCMake(device); !fromDevice.isEmpty())
        return fromDevice;
    const CMakeTool *def = CMakeToolManager::defaultCMakeTool();
    if (!def || isFoundOnPath(*def) || !def->filePath().isSameDevice(device->rootPath()))
        return {};
    return def->filePath();
}

static bool isIos(const Kit *k)
{
    const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(k);
    return deviceType == Ios::Constants::IOS_DEVICE_TYPE
           || deviceType == Ios::Constants::IOS_SIMULATOR_TYPE;
}

// Factories

class CMakeKitAspectFactory : public KitAspectFactory
{
public:
    CMakeKitAspectFactory();

    // KitAspect interface
    Tasks validate(const Kit *k) const final;
    void setup(Kit *k) final;
    void upgrade(Kit *k) final;
    void fix(Kit *k) final;
    ItemList toUserOutput(const Kit *k) const final;
    KitAspect *createKitAspect(Kit *k) const final;

    void addToMacroExpander(Kit *k, Utils::MacroExpander *expander) const final;

    QSet<Utils::Id> availableFeatures(const Kit *k) const final;

    std::optional<QtTaskTree::ExecutableItem> removeAutoDetected(
        const QString &detectionSource, const LogCallback &logCallback) const override;

    Utils::Result<QtTaskTree::ExecutableItem> createAspectFromJson(
        const DetectionSource &detectionSource,
        const FilePath &rootPath,
        Kit *kit,
        const QJsonValue &json,
        const LogCallback &logCallback) const override;
};

class CMakeGeneratorKitAspectFactory : public KitAspectFactory
{
public:
    CMakeGeneratorKitAspectFactory();

    Tasks validate(const Kit *k) const final;
    void setup(Kit *k) final;
    void fix(Kit *k) final;
    void upgrade(Kit *k) final;
    ItemList toUserOutput(const Kit *k) const final;
    KitAspect *createKitAspect(Kit *k) const final;
    void addToBuildEnvironment(const Kit *k, Utils::Environment &env) const final;

private:
    QVariant defaultValue(const Kit *k) const;
    bool isNinjaPresent(const Kit *k) const;
};

class CMakeConfigurationKitAspectFactory : public KitAspectFactory
{
public:
    CMakeConfigurationKitAspectFactory();

    // KitAspect interface
    Tasks validate(const Kit *k) const final;
    void setup(Kit *k) final;
    void fix(Kit *k) final;
    ItemList toUserOutput(const Kit *k) const final;
    KitAspect *createKitAspect(Kit *k) const final;

private:
    QVariant defaultValue(const Kit *k) const;
};

// Implementations

class CMakeKitAspectImpl final : public KitAspect
{
public:
    CMakeKitAspectImpl(Kit *k, const KitAspectFactory *factory)
        : KitAspect(k, factory)
        , m_chooser(createSubWidget<PathChooser>())
    {
        setManagingPage(ProjectExplorer::Constants::DEVICE_SETTINGS_PAGE_ID);

        m_chooser->setExpectedKind(PathChooserKind::ExistingCommand);
        m_chooser->setAllowPathFromDevice(true);
        m_chooser->setHistoryCompleter("CMake.Command.History");
        refresh();

        const auto apply = [this] {
            FilePath path = m_chooser->filePath();
            if (const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit());
                device && path.isLocal() && !device->rootPath().isLocal()) {
                path = device->rootPath().withNewMappedPath(path);
            }
            {
                const GuardLocker locker(m_ignoreChanges);
                CMakeKitAspect::setCMakeExecutable(kit(), path);
            }
            // fix() may have dropped a path that is not on the build device.
            refresh();
        };
        connect(m_chooser, &PathChooser::editingFinished, this, apply);
        connect(m_chooser, &PathChooser::browsingFinished, this, apply);
        connect(DeviceManager::instance(), &DeviceManager::deviceUpdated, this, [this](Id id) {
            if (id == BuildDeviceKitAspect::deviceId(kit()))
                updateDeviceParts();
        });
    }

    ~CMakeKitAspectImpl() override { delete m_chooser; }

private:
    Id settingsPageItemToPreselect() const override
    {
        return BuildDeviceKitAspect::deviceId(kit());
    }

    void makeReadOnly(bool readOnly) override { m_chooser->setReadOnly(readOnly); }

    void addToInnerLayout(Layouting::Layout &layout) override
    {
        addMutableAction(m_chooser);
        layout.addItem(m_chooser);
    }

    void refresh() override
    {
        if (m_ignoreChanges.isLocked())
            return;

        updateDeviceParts();
        m_chooser->setFilePath(FilePath::fromSettings(kit()->value(Constants::TOOL_ID)));
    }

    void updateDeviceParts()
    {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(kit());
        m_chooser->setBaseDirectory(device ? device->rootPath() : FilePath());
        m_chooser->setPlaceholderText(deviceCMake(device).toUserOutput());
    }

    PathChooser * const m_chooser;
    Guard m_ignoreChanges;
};

CMakeKitAspectFactory::CMakeKitAspectFactory()
{
    setId(Constants::TOOL_ID);
    setJsonKeys({"cmake", "cmake-tool"});
    setDisplayName(Tr::tr("CMake Tool"));
    setDescription(Tr::tr("The CMake Tool to use when building a project with CMake.<br>"
                      "Leave empty to use the one configured for the build device.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(20000);
}

} // Internal

using namespace Internal;

static FilePath effectiveCMakeExecutable(const Kit *k)
{
    if (!k)
        return {};
    if (!k->isAspectRelevant(CMakeKitAspect::id()))
        return {};
    const IDevice::ConstPtr device = BuildDeviceKitAspect::device(k);
    const FilePath executable = FilePath::fromSettings(k->value(Constants::TOOL_ID));
    if (executable.isEmpty() || (device && !executable.isSameDevice(device->rootPath())))
        return deviceCMake(device);
    return executable;
}

CMakeTool *CMakeKitAspect::cmakeTool(const Kit *k)
{
    return CMakeToolManager::cmakeToolForPath(effectiveCMakeExecutable(k));
}

Id CMakeKitAspect::id()
{
    return Constants::TOOL_ID;
}

FilePath CMakeKitAspect::cmakeExecutable(const Kit *k)
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    return tool ? tool->cmakeExecutable() : FilePath();
}

FilePath CMakeKitAspect::cmakeExecutableForNewKit(const IDevice::ConstPtr &buildDevice)
{
    return cmakeForNewKit(buildDevice);
}

CMakeKeywords CMakeKitAspect::cmakeKeywords(const Kit *k)
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    return tool ? tool->keywords() : CMakeKeywords();
}

void CMakeKitAspect::setCMakeExecutable(Kit *k, const FilePath &cmakeExecutable)
{
    if (!k)
        return;
    if (cmakeExecutable.isEmpty())
        k->removeKey(Constants::TOOL_ID);
    else
        k->setValue(Constants::TOOL_ID, cmakeExecutable.toSettings());
}

Tasks CMakeKitAspectFactory::validate(const Kit *k) const
{
    Tasks result;
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (tool && tool->isValid()) {
        CMakeTool::Version version = tool->version();
        if (version.major < 3 || (version.major == 3 && version.minor < 16)) {
            result << BuildSystemTask(Task::Warning,
                CMakeKitAspect::msgUnsupportedVersion(version.fullVersion));
        }
    }
    return result;
}

void CMakeKitAspectFactory::setup(Kit *k)
{
    QTC_ASSERT(k, return);

    if (k->hasValue(Constants::TOOL_ID))
        return;
    const IDevice::ConstPtr device = BuildDeviceKitAspect::device(k);
    if (deviceCMake(device).isEmpty())
        CMakeKitAspect::setCMakeExecutable(k, cmakeForNewKit(device));
}

void CMakeKitAspectFactory::upgrade(Kit *k)
{
    QTC_ASSERT(k, return);

    const QVariant value = k->value(Constants::TOOL_ID);
    if (!value.isValid() || FilePath::fromSettings(value).isAbsolutePath())
        return; // Already a path.

    if (const CMakeTool *tool = CMakeToolManager::findById(Id::fromSetting(value))) {
        const IDevice::ConstPtr device = BuildDeviceKitAspect::device(k);
        const FilePath path = tool->filePath();
        // Kits got the default when they were set up, which says nothing about a choice.
        const bool isPathDefault = tool == CMakeToolManager::defaultCMakeTool()
                                   && isFoundOnPath(*tool);
        if (device && (!path.isSameDevice(device->rootPath()) || isPathDefault
                       || path == device->deviceToolPath(Constants::CMAKE_TOOL_ID))) {
            k->removeKey(Constants::TOOL_ID); // Follow the build device.
        } else {
            k->setValue(Constants::TOOL_ID, path.toSettings());
        }
    } else {
        k->removeKey(Constants::TOOL_ID); // An id whose tool is gone.
    }
}

void CMakeKitAspectFactory::fix(Kit *k)
{
    QTC_ASSERT(k, return);

    const FilePath executable = FilePath::fromSettings(k->value(Constants::TOOL_ID));
    if (executable.isEmpty())
        return;
    const IDevice::ConstPtr device = BuildDeviceKitAspect::device(k);
    if (device && !executable.isSameDevice(device->rootPath()))
        k->removeKey(Constants::TOOL_ID);
}

KitAspectFactory::ItemList CMakeKitAspectFactory::toUserOutput(const Kit *k) const
{
    const CMakeTool *const tool = CMakeKitAspect::cmakeTool(k);
    return {{Tr::tr("CMake"), tool ? tool->displayName() : Tr::tr("Unconfigured")}};
}

KitAspect *CMakeKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new CMakeKitAspectImpl(k, this);
}

void CMakeKitAspectFactory::addToMacroExpander(Kit *k, MacroExpander *expander) const
{
    QTC_ASSERT(k, return);
    expander->registerFileVariables("CMake:Executable", Tr::tr("Path to the CMake executable"),
        [k] {
            return CMakeKitAspect::cmakeExecutable(k);
        });
}

static bool isKitSuitableForQtForMCUs(const Kit *k)
{
    // Some Boot2Qt kits are also suitable for Qt for MCUs
    return k
           && RunDeviceTypeKitAspect::deviceTypeId(k)
                  == ProjectExplorer::Constants::BOOT2QT_DEVICE_TYPE
           && k->id().name().startsWith("mcu.");
}

QSet<Id> CMakeKitAspectFactory::availableFeatures(const Kit *k) const
{
    QSet<Id> features = {};
    if (CMakeKitAspect::cmakeTool(k))
        features.insert(CMakeProjectManager::Constants::CMAKE_FEATURE_ID);

    if (isKitSuitableForQtForMCUs(k))
        features.insert("MCU");

    return features;
}

std::optional<QtTaskTree::ExecutableItem> CMakeKitAspectFactory::removeAutoDetected(
    const QString &detectionSource, const LogCallback &logCallback) const
{
    using namespace QtTaskTree;

    return QSyncTask([detectionSource, logCallback]() {
        CMakeToolManager::instance()->removeDetectedCMake(detectionSource, logCallback);
    });
}

Utils::Result<QtTaskTree::ExecutableItem> CMakeKitAspectFactory::createAspectFromJson(
    const DetectionSource &,
    const Utils::FilePath &rootPath,
    Kit *kit,
    const QJsonValue &json,
    const LogCallback &logCallback) const
{
    using ResultType = Result<FilePath>;

    const auto setup = [json, rootPath](Async<ResultType> &async) {
        async.setConcurrentCallData(
            [](QPromise<ResultType> &promise,
               const QJsonValue &json,
               const Utils::FilePath &rootPath) {
                if (!json.isObject()) {
                    promise.addResult(ResultError(
                        Tr::tr("Expected JSON Object, got: \"%1\".").arg(json.toString())));
                    return;
                }

                const QJsonValue binaryValue = json.toObject().value("binary");
                if (!binaryValue.isString()) {
                    promise.addResult(ResultError(
                        Tr::tr("Expected JSON Object with key \"binary\" to be a string.")));
                    return;
                }
                const FilePath cmakeExecutable = rootPath.withNewPath(binaryValue.toString());
                CMakeTool tool(DetectionSource::FromSystem, CMakeTool::createId());
                tool.setFilePath(cmakeExecutable);
                if (!tool.isValid()) {
                    promise.addResult(ResultError(
                        Tr::tr("CMake tool \"%1\" is not valid.")
                            .arg(cmakeExecutable.toUserOutput())));
                    return;
                }

                promise.addResult(cmakeExecutable);
            },
            json,
            rootPath);
    };

    const auto onDone = [logCallback, kit, json](const Async<ResultType> &async) {
        const ResultType cmake = async.takeResult();
        if (!cmake) {
            logCallback(Tr::tr("Cannot create CMake tool from JSON: %1").arg(cmake.error()));
            return;
        }
        const QJsonObject obj = json.toObject();

        logCallback(Tr::tr("Found CMake tool: \"%1\".").arg(cmake->toUserOutput()));
        CMakeKitAspect::setCMakeExecutable(kit, *cmake);

        if (obj.contains("generator"))
            CMakeGeneratorKitAspect::setGenerator(kit, obj.value("generator").toString());

        const QJsonValue variables = obj.value("variables");
        if (!variables.isObject())
            return;

        const QJsonObject variablesObj = variables.toObject();
        CMakeConfig config = CMakeConfigurationKitAspect::defaultConfiguration(kit);

        for (auto it = variablesObj.begin(); it != variablesObj.end(); ++it) {
            const QString key = it.key();
            QJsonValue value = it.value();
            auto valueType = CMakeConfigItem::STRING;

            if (value.isObject()) {
                QJsonObject valueObj = value.toObject();
                if (valueObj.contains("type")) {
                    valueType = CMakeConfigItem::typeStringToType(
                        valueObj.value("type").toString().toUpper().toUtf8());
                }
                value = valueObj.value("value");
            }

            if (value.isArray()) {
                QStringList valueList;
                for (const QJsonValue &v : value.toArray()) {
                    if (v.isString())
                        valueList.append(v.toString());
                }
                CMakeConfigItem item(
                    key.toUtf8(), valueType, QByteArray(), valueList.join(';').toUtf8(), valueList);
                config.insert(key.toUtf8(), item);
                continue;
            }

            if (!value.isString())
                continue;

            config.insert(
                key.toUtf8(),
                CMakeConfigItem(key.toUtf8(), valueType, QByteArray(), value.toString().toUtf8()));
        }

        CMakeConfigurationKitAspect::setConfiguration(kit, config);
    };

    return AsyncTask<ResultType>(setup, onDone);
}

QString CMakeKitAspect::msgUnsupportedVersion(const QByteArray &versionString)
{
    return Tr::tr("CMake version %1 is unsupported. Update to "
              "version 3.15 (with file-api) or later.")
        .arg(QString::fromUtf8(versionString));
}

static QString documentationUrl(const CMakeTool::Version &version, bool online)
{
    if (online) {
        QString helpVersion = "latest";
        if (!(version.major == 0 && version.minor == 0))
            helpVersion = QString("v%1.%2").arg(version.major).arg(version.minor);

        return QString("https://cmake.org/cmake/help/%1").arg(helpVersion);
    }

    return QString("qthelp://org.cmake.%1.%2.%3/doc")
        .arg(version.major)
        .arg(version.minor)
        .arg(version.patch);
}

void CMakeKitAspect::openCMakeHelpUrl(Kit *k, const QString &target)
{
    const CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (!tool)
        return;
    if (!tool->isValid())
        return;

    bool online = tool->qchFilePath().isEmpty();
    Core::HelpManager::showHelpUrl(target.arg(documentationUrl(tool->version(), online)));
}

// --------------------------------------------------------------------
// CMakeGeneratorKitAspect:
// --------------------------------------------------------------------

const char GENERATOR_ID[] = "CMake.GeneratorKitInformation";

const char GENERATOR_KEY[] = "Generator";
const char EXTRA_GENERATOR_KEY[] = "ExtraGenerator";
const char PLATFORM_KEY[] = "Platform";
const char TOOLSET_KEY[] = "Toolset";

class CMakeGeneratorKitAspectImpl final : public KitAspect
{
public:
    CMakeGeneratorKitAspectImpl(Kit *kit, const KitAspectFactory *factory)
        : KitAspect(kit, factory),
          m_label(createSubWidget<ElidingLabel>()),
          m_changeButton(createSubWidget<QPushButton>())
    {
        connect(this, &KitAspect::labelLinkActivated, this, [kit](const QString &) {
            CMakeKitAspect::openCMakeHelpUrl(kit, "%1/manual/cmake-generators.7.html");
        });

        m_label->setToolTip(factory->description());
        m_changeButton->setText(Tr::tr("Change..."));
        refresh();
        connect(m_changeButton, &QPushButton::clicked,
                this, &CMakeGeneratorKitAspectImpl::changeGenerator);
    }

    ~CMakeGeneratorKitAspectImpl() override
    {
        delete m_label;
        delete m_changeButton;
    }

private:
    // KitAspectWidget interface
    void makeReadOnly(bool readOnly) override
    {
        m_changeButton->setEnabled(!readOnly);
    }

    void addToInnerLayout(Layouting::Layout &layout) override
    {
        addMutableAction(m_label);
        layout.addItem(m_label);
        layout.addItem(m_changeButton);
    }

    void refresh() override
    {
        CMakeTool *const tool = CMakeKitAspect::cmakeTool(kit());
        if (tool != m_currentTool)
            m_currentTool = tool;

        m_changeButton->setEnabled(m_currentTool);
        const QString generator = CMakeGeneratorKitAspect::generator(kit());
        const QString platform = CMakeGeneratorKitAspect::platform(kit());
        const QString toolset = CMakeGeneratorKitAspect::toolset(kit());

        QStringList messageLabel;
        messageLabel << generator;

        if (!platform.isEmpty())
            messageLabel << ", " << Tr::tr("Platform") << ": " << platform;
        if (!toolset.isEmpty())
            messageLabel << ", " << Tr::tr("Toolset") << ": " << toolset;

        m_label->setText(messageLabel.join(""));
    }

    void changeGenerator()
    {
        QPointer<QDialog> changeDialog = new QDialog(m_changeButton);

        // Disable help button in titlebar on windows:
        Qt::WindowFlags flags = changeDialog->windowFlags();
        flags |= Qt::MSWindowsFixedSizeDialogHint;
        changeDialog->setWindowFlags(flags);

        changeDialog->setWindowTitle(Tr::tr("CMake Generator"));

        auto layout = new QGridLayout(changeDialog);
        layout->setSizeConstraint(QLayout::SetFixedSize);

        auto cmakeLabel = new QLabel;
        cmakeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto generatorCombo = new QComboBox;
        auto platformEdit = new QLineEdit;
        auto toolsetEdit = new QLineEdit;

        int row = 0;
        layout->addWidget(new QLabel(QLatin1String("Executable:")));
        layout->addWidget(cmakeLabel, row, 1);

        ++row;
        layout->addWidget(new QLabel(Tr::tr("Generator:")), row, 0);
        layout->addWidget(generatorCombo, row, 1);

        ++row;
        layout->addWidget(new QLabel(Tr::tr("Platform:")), row, 0);
        layout->addWidget(platformEdit, row, 1);

        ++row;
        layout->addWidget(new QLabel(Tr::tr("Toolset:")), row, 0);
        layout->addWidget(toolsetEdit, row, 1);

        ++row;
        auto bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        layout->addWidget(bb, row, 0, 1, 2);

        connect(bb, &QDialogButtonBox::accepted, changeDialog.data(), &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, changeDialog.data(), &QDialog::reject);

        cmakeLabel->setText(m_currentTool->cmakeExecutable().toUserOutput());

        const QList<CMakeTool::Generator> generatorList = Utils::sorted(
                    m_currentTool->supportedGenerators(), &CMakeTool::Generator::name);

        for (auto it = generatorList.constBegin(); it != generatorList.constEnd(); ++it)
            generatorCombo->addItem(it->name);

        auto updateDialog = [&generatorList, generatorCombo,
                platformEdit, toolsetEdit](const QString &name) {
            const auto it = std::find_if(generatorList.constBegin(), generatorList.constEnd(),
                                   [name](const CMakeTool::Generator &g) { return g.name == name; });
            QTC_ASSERT(it != generatorList.constEnd(), return);
            generatorCombo->setCurrentText(name);

            platformEdit->setEnabled(it->supportsPlatform);
            toolsetEdit->setEnabled(it->supportsToolset);
        };

        updateDialog(CMakeGeneratorKitAspect::generator(kit()));

        generatorCombo->setCurrentText(CMakeGeneratorKitAspect::generator(kit()));
        platformEdit->setText(platformEdit->isEnabled() ? CMakeGeneratorKitAspect::platform(kit()) : QString());
        toolsetEdit->setText(toolsetEdit->isEnabled() ? CMakeGeneratorKitAspect::toolset(kit()) : QString());

        connect(generatorCombo, &QComboBox::currentTextChanged, updateDialog);

        if (changeDialog->exec() == QDialog::Accepted) {
            if (!changeDialog)
                return;

            CMakeGeneratorKitAspect::set(kit(), generatorCombo->currentText(),
                                         platformEdit->isEnabled() ? platformEdit->text() : QString(),
                                         toolsetEdit->isEnabled() ? toolsetEdit->text() : QString());

            refresh();
        }
    }

    ElidingLabel *m_label;
    QPushButton *m_changeButton;
    CMakeTool *m_currentTool = nullptr;
};

namespace {

class GeneratorInfo
{
public:
    GeneratorInfo() = default;
    GeneratorInfo(const QString &generator_,
                  const QString &platform_ = QString(),
                  const QString &toolset_ = QString())
        : generator(generator_)
        , platform(platform_)
        , toolset(toolset_)
    {}

    QVariant toVariant() const {
        QVariantMap result;
        result.insert(GENERATOR_KEY, generator);
        result.insert(EXTRA_GENERATOR_KEY, extraGenerator);
        result.insert(PLATFORM_KEY, platform);
        result.insert(TOOLSET_KEY, toolset);
        return result;
    }
    void fromVariant(const QVariant &v) {
        const QVariantMap value = v.toMap();

        generator = value.value(GENERATOR_KEY).toString();
        extraGenerator = value.value(EXTRA_GENERATOR_KEY).toString();
        platform = value.value(PLATFORM_KEY).toString();
        toolset = value.value(TOOLSET_KEY).toString();
    }

    QString generator;
    QString extraGenerator;
    QString platform;
    QString toolset;
};

} // namespace

static GeneratorInfo generatorInfo(const Kit *k)
{
    GeneratorInfo info;
    if (!k)
        return info;

    info.fromVariant(k->value(GENERATOR_ID));
    return info;
}

static void setGeneratorInfo(Kit *k, const GeneratorInfo &info)
{
    if (!k)
        return;
    k->setValue(GENERATOR_ID, info.toVariant());
}

CMakeGeneratorKitAspectFactory::CMakeGeneratorKitAspectFactory()
{
    setId(GENERATOR_ID);
    setDisplayName(Tr::tr("CMake <a href=\"generator\">generator</a>"));
    setDescription(Tr::tr("CMake generator defines how a project is built when using CMake.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(19000);

    connect(DeviceManager::instance(), &DeviceManager::deviceUpdated, this, [this](Id deviceId) {
        if (!KitManager::isLoaded())
            return;
        for (Kit *k : KitManager::kits()) {
            if (BuildDeviceKitAspect::deviceId(k) == deviceId)
                fix(k);
        }
    });
}

QString CMakeGeneratorKitAspect::generator(const Kit *k)
{
    return generatorInfo(k).generator;
}

QString CMakeGeneratorKitAspect::platform(const Kit *k)
{
    return generatorInfo(k).platform;
}

QString CMakeGeneratorKitAspect::toolset(const Kit *k)
{
    return generatorInfo(k).toolset;
}

void CMakeGeneratorKitAspect::setGenerator(Kit *k, const QString &generator)
{
    GeneratorInfo info = generatorInfo(k);
    info.generator = generator;
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspect::setPlatform(Kit *k, const QString &platform)
{
    GeneratorInfo info = generatorInfo(k);
    info.platform = platform;
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspect::setToolset(Kit *k, const QString &toolset)
{
    GeneratorInfo info = generatorInfo(k);
    info.toolset = toolset;
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspect::set(Kit *k,
                                  const QString &generator,
                                  const QString &platform,
                                  const QString &toolset)
{
    GeneratorInfo info(generator, platform, toolset);
    setGeneratorInfo(k, info);
}

CMakeConfig CMakeGeneratorKitAspect::generatorCMakeConfig(const Kit *k)
{
    CMakeConfig config;

    GeneratorInfo info = generatorInfo(k);
    if (info.generator.isEmpty())
        return config;

    config.insert(CMakeConfigItem("CMAKE_GENERATOR", info.generator.toUtf8()));

    if (info.generator.startsWith("Ninja", Qt::CaseInsensitive)) {
        if (auto device = BuildDeviceKitAspect::device(k)) {
            const FilePath ninjaPath = device->deviceToolPath(
                ProjectExplorer::Constants::TOOL_TYPE_NINJA);
            if (ninjaPath.isExecutableFile())
                config.insert(CMakeConfigItem("CMAKE_MAKE_PROGRAM", ninjaPath.path().toUtf8()));
        }
    }

    if (!info.platform.isEmpty())
        config.insert(CMakeConfigItem("CMAKE_GENERATOR_PLATFORM", info.platform.toUtf8()));

    if (!info.toolset.isEmpty())
        config.insert(CMakeConfigItem("CMAKE_GENERATOR_TOOLSET", info.toolset.toUtf8()));

    return config;
}

bool CMakeGeneratorKitAspect::isMultiConfigGenerator(const Kit *k)
{
    const QString generator = CMakeGeneratorKitAspect::generator(k);
    return generator.indexOf("Visual Studio") != -1 ||
           generator == "Xcode" ||
           generator == "Ninja Multi-Config";
}

QVariant CMakeGeneratorKitAspectFactory::defaultValue(const Kit *k) const
{
    QTC_ASSERT(k, return QVariant());

    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (!tool)
        return QVariant();

    if (isIos(k))
        return GeneratorInfo("Xcode").toVariant();

    const QList<CMakeTool::Generator> known = tool->supportedGenerators();
    auto it = std::find_if(known.constBegin(), known.constEnd(), [](const CMakeTool::Generator &g) {
        return g.matches("Ninja");
    });
    if (it != known.constEnd()) {
        if (isNinjaPresent(k))
            return GeneratorInfo("Ninja").toVariant();
    }

    if (tool->filePath().osType() == OsTypeWindows) {
        // *sigh* Windows with its zoo of incompatible stuff again...
        Toolchain *tc = ToolchainKitAspect::cxxToolchain(k);
        if (tc && tc->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID) {
            it = std::find_if(known.constBegin(),
                              known.constEnd(),
                              [](const CMakeTool::Generator &g) {
                                  return g.matches("MinGW Makefiles");
                              });
        } else {
            it = std::find_if(known.constBegin(),
                              known.constEnd(),
                              [](const CMakeTool::Generator &g) {
                                  return g.matches("NMake Makefiles")
                                         || g.matches("NMake Makefiles JOM");
                              });
            if (globalProjectExplorerSettings().useJom()) {
                it = std::find_if(known.constBegin(),
                                  known.constEnd(),
                                  [](const CMakeTool::Generator &g) {
                                      return g.matches("NMake Makefiles JOM");
                                  });
            }

            if (it == known.constEnd()) {
                it = std::find_if(known.constBegin(),
                                  known.constEnd(),
                                  [](const CMakeTool::Generator &g) {
                                      return g.matches("NMake Makefiles");
                                  });
            }
        }
    } else {
        // Unix-oid OSes:
        it = std::find_if(known.constBegin(), known.constEnd(), [](const CMakeTool::Generator &g) {
            return g.matches("Unix Makefiles");
        });
    }
    if (it == known.constEnd())
        it = known.constBegin(); // Fallback to the first generator...
    if (it == known.constEnd())
        return QVariant();

    return GeneratorInfo(it->name).toVariant();
}

bool CMakeGeneratorKitAspectFactory::isNinjaPresent(const Kit *k) const
{
    const CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);
    const FilePath makeProgram = config.filePathValueOf("CMAKE_MAKE_PROGRAM");
    if (makeProgram.baseName().startsWith("ninja", Qt::CaseInsensitive))
        return true;

    if (auto device = BuildDeviceKitAspect::device(k)) {
        if (device->deviceToolPath(ProjectExplorer::Constants::TOOL_TYPE_NINJA).isExecutableFile())
            return true;
    }

    return false;
}

Tasks CMakeGeneratorKitAspectFactory::validate(const Kit *k) const
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (!tool)
        return {};

    Tasks result;
    const auto addWarning = [&result](const QString &desc) {
        result << BuildSystemTask(Task::Warning, desc);
    };

    if (!tool->isValid()) {
        addWarning(Tr::tr("CMake Tool is unconfigured, CMake generator will be ignored."));
    } else {
        const GeneratorInfo info = generatorInfo(k);
        QList<CMakeTool::Generator> known = tool->supportedGenerators();
        auto it = std::find_if(known.constBegin(), known.constEnd(), [info](const CMakeTool::Generator &g) {
            return g.matches(info.generator);
        });
        if (it == known.constEnd()) {
            addWarning(Tr::tr("CMake Tool does not support the configured generator."));
        } else {
            if (!it->supportsPlatform && !info.platform.isEmpty())
                addWarning(Tr::tr("Platform is not supported by the selected CMake generator."));
            if (!it->supportsToolset && !info.toolset.isEmpty())
                addWarning(Tr::tr("Toolset is not supported by the selected CMake generator."));
        }
        if (!tool->hasFileApi()) {
            addWarning(Tr::tr("The selected CMake binary does not support file-api. "
                              "%1 will not be able to parse CMake projects.")
                           .arg(QGuiApplication::applicationDisplayName()));
        }
    }

    return result;
}

void CMakeGeneratorKitAspectFactory::setup(Kit *k)
{
    if (!k || k->hasValue(id()))
        return;
    GeneratorInfo info;
    info.fromVariant(defaultValue(k));
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspectFactory::fix(Kit *k)
{
    const CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    const GeneratorInfo info = generatorInfo(k);

    if (!tool || !tool->isValid())
        return;
    QList<CMakeTool::Generator> known = tool->supportedGenerators();
    auto it = std::find_if(known.constBegin(), known.constEnd(),
                           [info](const CMakeTool::Generator &g) {
        return g.matches(info.generator);
    });
    if (it == known.constEnd()) {
        GeneratorInfo dv;
        dv.fromVariant(defaultValue(k));
        setGeneratorInfo(k, dv);
    } else {
        const GeneratorInfo dv(info.generator,
                               it->supportsPlatform ? info.platform : QString(),
                               it->supportsToolset ? info.toolset : QString());
        setGeneratorInfo(k, dv);
    }
}

void CMakeGeneratorKitAspectFactory::upgrade(Kit *k)
{
    QTC_ASSERT(k, return);

    const QVariant value = k->value(GENERATOR_ID);
    if (value.typeId() != QMetaType::QVariantMap) {
        GeneratorInfo info;
        const QString fullName = value.toString();
        const int pos = fullName.indexOf(" - ");
        if (pos >= 0) {
            info.generator = fullName.mid(pos + 3);
            info.extraGenerator = fullName.mid(0, pos);
        } else {
            info.generator = fullName;
        }
        setGeneratorInfo(k, info);
    }
}

KitAspectFactory::ItemList CMakeGeneratorKitAspectFactory::toUserOutput(const Kit *k) const
{
    const GeneratorInfo info = generatorInfo(k);
    QString message;
    if (info.generator.isEmpty()) {
        message = Tr::tr("<Use Default Generator>");
    } else {
        message = Tr::tr("Generator: %1<br>Extra generator: %2").arg(info.generator).arg(info.extraGenerator);
        if (!info.platform.isEmpty())
            message += "<br/>" + Tr::tr("Platform: %1").arg(info.platform);
        if (!info.toolset.isEmpty())
            message += "<br/>" + Tr::tr("Toolset: %1").arg(info.toolset);
    }
    return {{Tr::tr("CMake Generator"), message}};
}

KitAspect *CMakeGeneratorKitAspectFactory::createKitAspect(Kit *k) const
{
    return new CMakeGeneratorKitAspectImpl(k, this);
}

void CMakeGeneratorKitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    GeneratorInfo info = generatorInfo(k);
    if (info.generator == "NMake Makefiles JOM") {
        env.appendOrSetPath(Core::ICore::libexecPath());
        env.appendOrSetPath(Core::ICore::libexecPath("jom"));
    }

    if (info.generator.contains("Ninja")) {
        CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
        if (tool) {
            const FilePath cmakeBinDir = tool->filePath().parentDir();
            env.appendOrSetPath(cmakeBinDir);
        }
    }
}

// --------------------------------------------------------------------
// CMakeConfigurationKitAspect:
// --------------------------------------------------------------------

const char CONFIGURATION_ID[] = "CMake.ConfigurationKitInformation";
const char ADDITIONAL_CONFIGURATION_ID[] = "CMake.AdditionalConfigurationParameters";

const char CMAKE_C_TOOLCHAIN_KEY[] = "CMAKE_C_COMPILER";
const char CMAKE_CXX_TOOLCHAIN_KEY[] = "CMAKE_CXX_COMPILER";
const char CMAKE_QMAKE_KEY[] = "QT_QMAKE_EXECUTABLE";
const char CMAKE_PREFIX_PATH_KEY[] = "CMAKE_PREFIX_PATH";
const char QTC_CMAKE_PRESET_KEY[] = "QTC_CMAKE_PRESET";

class CMakeConfigurationKitAspectImpl final : public KitAspect
{
public:
    CMakeConfigurationKitAspectImpl(Kit *kit, const KitAspectFactory *factory)
        : KitAspect(kit, factory),
          m_summaryLabel(createSubWidget<ElidingLabel>()),
          m_manageButton(createSubWidget<QPushButton>())
    {
        refresh();
        m_manageButton->setText(Tr::tr("Change..."));
        setIgnoreForDirtyHook(m_manageButton);
        connect(m_manageButton, &QAbstractButton::clicked,
                this, &CMakeConfigurationKitAspectImpl::editConfigurationChanges);
    }

private:
    // KitAspectWidget interface
    void addToInnerLayout(Layouting::Layout &layout) override
    {
        addMutableAction(m_summaryLabel);
        layout.addItem(m_summaryLabel);
        layout.addItem(m_manageButton);
    }

    void makeReadOnly(bool readOnly) override
    {
        m_manageButton->setEnabled(!readOnly);
        if (readOnly && m_dialog)
            m_dialog->reject();
    }

    void refresh() override
    {
        const QStringList current = CMakeConfigurationKitAspect::toArgumentsList(kit());
        const QString additionalText = CMakeConfigurationKitAspect::additionalConfiguration(kit());
        const QString labelText = additionalText.isEmpty()
                                      ? current.join(' ')
                                      : current.join(' ') + " " + additionalText;

        m_summaryLabel->setText(labelText);

        if (m_editor)
            m_editor->setPlainText(current.join('\n'));

        if (m_additionalEditor)
            m_additionalEditor->setText(additionalText);
    }

    void editConfigurationChanges()
    {
        if (m_dialog) {
            m_dialog->activateWindow();
            m_dialog->raise();
            return;
        }

        QTC_ASSERT(!m_editor, return);

        m_dialog = new QDialog(m_summaryLabel->window());
        m_dialog->setWindowTitle(Tr::tr("Edit CMake Configuration"));
        auto layout = new QVBoxLayout(m_dialog);
        m_editor = new QPlainTextEdit;
        auto editorLabel = new QLabel(m_dialog);
        editorLabel->setText(Tr::tr("Enter one CMake <a href=\"variable\">variable</a> per line.<br/>"
                                "To set a variable, use -D&lt;variable&gt;:&lt;type&gt;=&lt;value&gt;.<br/>"
                                "&lt;type&gt; can have one of the following values: FILEPATH, PATH, "
                                "BOOL, INTERNAL, or STRING."));
        connect(editorLabel, &QLabel::linkActivated, this, [this](const QString &) {
            CMakeKitAspect::openCMakeHelpUrl(kit(), "%1/manual/cmake-variables.7.html");
        });
        m_editor->setMinimumSize(800, 200);

        auto chooser = new VariableChooser(m_dialog);
        chooser->addSupportedWidget(m_editor);
        chooser->addMacroExpanderProvider({this, [this] { return kit()->macroExpander(); }});

        m_additionalEditor = new QLineEdit;
        auto additionalLabel = new QLabel(m_dialog);
        additionalLabel->setText(Tr::tr("Additional CMake <a href=\"options\">options</a>:"));
        connect(additionalLabel, &QLabel::linkActivated, this, [this](const QString &) {
            CMakeKitAspect::openCMakeHelpUrl(kit(), "%1/manual/cmake.1.html#options");
        });

        auto additionalChooser = new VariableChooser(m_dialog);
        additionalChooser->addSupportedWidget(m_additionalEditor);
        additionalChooser->addMacroExpanderProvider({this, [this] { return kit()->macroExpander(); }});

        auto additionalLayout = new QHBoxLayout();
        additionalLayout->addWidget(additionalLabel);
        additionalLayout->addWidget(m_additionalEditor);

        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply
                                            |QDialogButtonBox::Reset|QDialogButtonBox::Cancel);

        layout->addWidget(m_editor);
        layout->addWidget(editorLabel);
        layout->addLayout(additionalLayout);
        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, m_dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, m_dialog, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::clicked, m_dialog, [buttons, this](QAbstractButton *button) {
            if (button != buttons->button(QDialogButtonBox::Reset))
                return;
            KitGuard guard(kit());
            CMakeConfigurationKitAspect::setConfiguration(kit(),
                                                          CMakeConfigurationKitAspect::defaultConfiguration(kit()));
            CMakeConfigurationKitAspect::setAdditionalConfiguration(kit(), QString());
        });
        connect(m_dialog, &QDialog::accepted, this, &CMakeConfigurationKitAspectImpl::acceptChangesDialog);
        connect(m_dialog, &QDialog::rejected, this, &CMakeConfigurationKitAspectImpl::closeChangesDialog);
        connect(buttons->button(QDialogButtonBox::Apply), &QAbstractButton::clicked,
                this, &CMakeConfigurationKitAspectImpl::applyChanges);

        refresh();
        m_dialog->open();
    }

    void applyChanges()
    {
        QTC_ASSERT(m_editor, return);
        KitGuard guard(kit());

        QStringList unknownOptions;
        const CMakeConfig config = CMakeConfig::fromArguments(m_editor->toPlainText().split('\n'),
                                                              unknownOptions);
        CMakeConfigurationKitAspect::setConfiguration(kit(), config);

        QString additionalConfiguration = m_additionalEditor->text();
        if (!unknownOptions.isEmpty()) {
            if (!additionalConfiguration.isEmpty())
                additionalConfiguration += " ";
            additionalConfiguration += ProcessArgs::joinArgs(unknownOptions);
        }
        CMakeConfigurationKitAspect::setAdditionalConfiguration(kit(), additionalConfiguration);
    }
    void closeChangesDialog()
    {
        m_dialog->deleteLater();
        m_dialog = nullptr;
        m_editor = nullptr;
        m_additionalEditor = nullptr;
    }
    void acceptChangesDialog()
    {
        applyChanges();
        closeChangesDialog();
    }

    QLabel *m_summaryLabel;
    QPushButton *m_manageButton;
    QDialog *m_dialog = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QLineEdit *m_additionalEditor = nullptr;
};


CMakeConfigurationKitAspectFactory::CMakeConfigurationKitAspectFactory()
{
    setId(CONFIGURATION_ID);
    setDisplayName(Tr::tr("CMake Configuration"));
    setDescription(Tr::tr("Default configuration passed to CMake when setting up a project."));
    setPriority(18000);
}

CMakeConfig CMakeConfigurationKitAspect::configuration(const Kit *k)
{
    if (!k)
        return CMakeConfig();
    const QStringList tmp = k->value(CONFIGURATION_ID).toStringList();
    return Utils::transform(tmp, &CMakeConfigItem::fromString);
}

void CMakeConfigurationKitAspect::setConfiguration(Kit *k, const CMakeConfig &config)
{
    if (!k)
        return;
    const QStringList tmp = Utils::transform(config.toList(),
                                             [](const CMakeConfigItem &i) { return i.toString(); });
    k->setValue(CONFIGURATION_ID, tmp);
}

QString CMakeConfigurationKitAspect::additionalConfiguration(const Kit *k)
{
    if (!k)
        return QString();
    return k->value(ADDITIONAL_CONFIGURATION_ID).toString();
}

void CMakeConfigurationKitAspect::setAdditionalConfiguration(Kit *k, const QString &config)
{
    if (!k)
        return;
    k->setValue(ADDITIONAL_CONFIGURATION_ID, config);
}

QStringList CMakeConfigurationKitAspect::toStringList(const Kit *k)
{
    QStringList current = Utils::transform(CMakeConfigurationKitAspect::configuration(k).toList(),
                                           [](const CMakeConfigItem &i) { return i.toString(); });
    current = Utils::filtered(current, [](const QString &s) { return !s.isEmpty(); });
    return current;
}

void CMakeConfigurationKitAspect::fromStringList(Kit *k, const QStringList &in)
{
    CMakeConfig result;
    for (const QString &s : in) {
        const CMakeConfigItem item = CMakeConfigItem::fromString(s);
        if (!item.key.isEmpty())
            result.insert(item);
    }
    setConfiguration(k, result);
}

QStringList CMakeConfigurationKitAspect::toArgumentsList(const Kit *k)
{
    QStringList current = Utils::transform(CMakeConfigurationKitAspect::configuration(k).toList(),
                                           [](const CMakeConfigItem &i) {
                                               return i.toArgument(nullptr);
                                           });
    current = Utils::filtered(current, [](const QString &s) { return s != "-D" && s != "-U"; });
    return current;
}

CMakeConfig CMakeConfigurationKitAspect::defaultConfiguration(const Kit *k)
{
    Q_UNUSED(k)
    CMakeConfig config;
    // Qt4:
    config.insert(
        CMakeConfigItem(CMAKE_QMAKE_KEY, CMakeConfigItem::FILEPATH, "%{Qt:qmakeExecutable}"));
    // Qt5:
    config.insert(
        CMakeConfigItem(CMAKE_PREFIX_PATH_KEY, CMakeConfigItem::PATH, "%{Qt:QT_INSTALL_PREFIX}"));

    config.insert(
        CMakeConfigItem(CMAKE_C_TOOLCHAIN_KEY, CMakeConfigItem::FILEPATH, "%{Compiler:Executable:C}"));
    config.insert(CMakeConfigItem(
        CMAKE_CXX_TOOLCHAIN_KEY, CMakeConfigItem::FILEPATH, "%{Compiler:Executable:Cxx}"));

    return config;
}

void CMakeConfigurationKitAspect::setCMakePreset(Kit *k, const QString &presetName)
{
    CMakeConfig config = configuration(k);
    config.insert(
        CMakeConfigItem(QTC_CMAKE_PRESET_KEY, CMakeConfigItem::INTERNAL, presetName.toUtf8()));

    setConfiguration(k, config);
}

CMakeConfigItem CMakeConfigurationKitAspect::cmakePresetConfigItem(const Kit *k)
{
    const CMakeConfig config = configuration(k);
    return config.value(QTC_CMAKE_PRESET_KEY);
}

Id CMakeConfigurationKitAspect::cmakePresetKitId(
    const QString &projectName, const QString &presetName)
{
    return Id::fromString(QString("%1:CMakePresets:%2").arg(projectName).arg(presetName));
}

QVariant CMakeConfigurationKitAspectFactory::defaultValue(const Kit *k) const
{
    // FIXME: Convert preload scripts
    CMakeConfig config = CMakeConfigurationKitAspect::defaultConfiguration(k);
    const QStringList tmp = Utils::transform(config.toList(),
                                             [](const CMakeConfigItem &i) { return i.toString(); });
    return tmp;
}

// A link to the compiler that keeps its name is the same compiler. One under another name
// is not: clang++ is often a link to clang, and behaves differently.
static bool isSameCompiler(const FilePath &configured, const FilePath &compiler)
{
    return configured == compiler || configured == configured.withNewMappedPath(compiler)
           || (configured.fileName() == compiler.fileName()
               && configured.isSameExecutable(compiler));
}

Tasks CMakeConfigurationKitAspectFactory::validate(const Kit *k) const
{
    QTC_ASSERT(k, return Tasks());

    const FilePath cmakeExecutable = CMakeKitAspect::cmakeExecutable(k);
    if (cmakeExecutable.isEmpty())
        return Tasks();

    const QtSupport::QtVersion *const version = QtSupport::QtKitAspect::qtVersion(k);
    const Toolchain *const tcC = ToolchainKitAspect::cToolchain(k);
    const Toolchain *const tcCxx = ToolchainKitAspect::cxxToolchain(k);
    const CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);

    const bool isQt4 = version && version->qtVersion() < QVersionNumber(5, 0, 0);
    FilePath qmakePath; // This is relative to the cmake used for building.
    QStringList qtInstallDirs; // This is relativ to the cmake used for building.
    FilePath tcCPath;
    FilePath tcCxxPath;
    for (const CMakeConfigItem &i : config) {
        // Do not use expand(QByteArray) as we cannot be sure the input is latin1
        const QString expandedValue = k->macroExpander()->expand(QString::fromUtf8(i.value));
        if (i.key == CMAKE_QMAKE_KEY)
            qmakePath = cmakeExecutable.withNewPath(expandedValue);
        else if (i.key == CMAKE_C_TOOLCHAIN_KEY)
            tcCPath = cmakeExecutable.withNewPath(expandedValue);
        else if (i.key == CMAKE_CXX_TOOLCHAIN_KEY)
            tcCxxPath = cmakeExecutable.withNewPath(expandedValue);
        else if (i.key == CMAKE_PREFIX_PATH_KEY)
            qtInstallDirs = CMakeConfigItem::cmakeSplitValue(expandedValue);
    }

    Tasks result;
    const auto addWarning = [&result](const QString &desc) {
        result << BuildSystemTask(Task::Warning, desc);
    };

    // Validate Qt:
    if (qmakePath.isEmpty()) {
        if (version && version->isValid() && isQt4) {
            addWarning(Tr::tr("CMake configuration has no path to qmake binary set, "
                          "even though the kit has a valid Qt version."));
        }
    } else {
        if (!version || !version->isValid()) {
            addWarning(Tr::tr("CMake configuration has a path to a qmake binary set, "
                          "even though the kit has no valid Qt version."));
        } else if (qmakePath != version->qmakeFilePath() && isQt4) {
            addWarning(Tr::tr("CMake configuration has a path to a qmake binary set "
                          "that does not match the qmake binary path "
                          "configured in the Qt version."));
        }
    }
    if (version && !qtInstallDirs.contains(version->prefix().path()) && !isQt4) {
        if (version->isValid()) {
            addWarning(Tr::tr("CMake configuration has no CMAKE_PREFIX_PATH set "
                          "that points to the kit Qt version."));
        }
    }

    // Validate Toolchains:
    if (tcCPath.isEmpty()) {
        if (tcC && tcC->isValid()) {
            addWarning(Tr::tr("CMake configuration has no path to a C compiler set, "
                           "even though the kit has a valid tool chain."));
        }
    } else {
        if (!tcC || !tcC->isValid()) {
            addWarning(Tr::tr("CMake configuration has a path to a C compiler set, "
                          "even though the kit has no valid tool chain."));
        } else if (!isSameCompiler(tcCPath, tcC->compilerCommand())) {
            addWarning(Tr::tr("CMake configuration has a path to a C compiler set "
                          "that does not match the compiler path "
                          "configured in the tool chain of the kit."));
        }
    }

    if (tcCxxPath.isEmpty()) {
        if (tcCxx && tcCxx->isValid()) {
            addWarning(Tr::tr("CMake configuration has no path to a C++ compiler set, "
                          "even though the kit has a valid tool chain."));
        }
    } else {
        if (!tcCxx || !tcCxx->isValid()) {
            addWarning(Tr::tr("CMake configuration has a path to a C++ compiler set, "
                          "even though the kit has no valid tool chain."));
        } else if (!isSameCompiler(tcCxxPath, tcCxx->compilerCommand())) {
            addWarning(Tr::tr("CMake configuration has a path to a C++ compiler set "
                          "that does not match the compiler path "
                          "configured in the tool chain of the kit."));
        }
    }

    return result;
}

void CMakeConfigurationKitAspectFactory::setup(Kit *k)
{
    if (k && !k->hasValue(CONFIGURATION_ID))
        k->setValue(CONFIGURATION_ID, defaultValue(k));
}

void CMakeConfigurationKitAspectFactory::fix(Kit *k)
{
    Q_UNUSED(k)
}

KitAspectFactory::ItemList CMakeConfigurationKitAspectFactory::toUserOutput(const Kit *k) const
{
    return {{Tr::tr("CMake Configuration"), CMakeConfigurationKitAspect::toStringList(k).join("<br>")}};
}

KitAspect *CMakeConfigurationKitAspectFactory::createKitAspect(Kit *k) const
{
    if (!k)
        return nullptr;
    return new CMakeConfigurationKitAspectImpl(k, this);
}

// Factory instances;

static CMakeKitAspectFactory &cmakeKitAspectFactory()
{
    static CMakeKitAspectFactory theCMakeKitAspectFactory;
    return theCMakeKitAspectFactory;
}

static CMakeGeneratorKitAspectFactory &cmakeGeneratorKitAspectFactory()
{
    static CMakeGeneratorKitAspectFactory theCMakeGeneratorKitAspectFactory;
    return theCMakeGeneratorKitAspectFactory;
}

static CMakeConfigurationKitAspectFactory &cmakeConfigurationKitAspectFactory()
{
    static CMakeConfigurationKitAspectFactory theCMakeConfigurationKitAspectFactory;
    return theCMakeConfigurationKitAspectFactory;
}

KitAspect *CMakeKitAspect::createKitAspect(Kit *k)
{
    return cmakeKitAspectFactory().createKitAspect(k);
}

KitAspect *CMakeGeneratorKitAspect::createKitAspect(Kit *k)
{
    return cmakeGeneratorKitAspectFactory().createKitAspect(k);
}

KitAspect *CMakeConfigurationKitAspect::createKitAspect(Kit *k)
{
    return cmakeConfigurationKitAspectFactory().createKitAspect(k);
}

void Internal::setupCMakeKitAspects()
{
    cmakeKitAspectFactory();
    cmakeGeneratorKitAspectFactory();
    cmakeConfigurationKitAspectFactory();
}

#ifdef WITH_TESTS

class CMakeKitAspectTest final : public QObject
{
    Q_OBJECT

    static IDevice::Ptr addTestDevice(const QString &deviceTool)
    {
        class TestDevice final : public IDevice
        {
        public:
            TestDevice()
            {
                setupId(AutoDetected, Id::generate());
                setType("CMakeProjectManager.TestDevice");
                setOsType(OsTypeLinux);
            }
            void initToolAspects() { initDeviceToolAspects(); }

        private:
            IDeviceWidget *createWidget() final { return nullptr; }
        };

        const auto device = std::make_shared<TestDevice>();
        device->initToolAspects();
        DeviceToolAspect *aspect = Utils::findOrDefault(
            device->deviceToolAspects(DeviceToolAspect::AllTools), [](DeviceToolAspect *a) {
                return a->toolId() == Constants::CMAKE_TOOL_ID;
            });
        QTC_ASSERT(aspect, return {});
        if (!deviceTool.isEmpty())
            aspect->setToolPath(device->rootPath().withNewPath(deviceTool));
        DeviceManager::addDevice(device);
        return device;
    }

private slots:
    void testUpgrade_data()
    {
        QTest::addColumn<QVariant>("value");
        QTest::addColumn<QVariant>("expected");

        const QVariant unknownId(
            "CMakeProjectManager.CMakeTool.{9f2c1d4e-0000-4000-8000-000000000000}");
        const QVariant windowsId("C:\\Program Files\\CMake\\bin\\cmake.exe");
        const QVariant deviceId("docker://4711/usr/bin/cmake");
        const QVariant path = FilePath::fromString("/usr/bin/cmake").toSettings();

        QTest::newRow("id without tool") << unknownId << QVariant();
        QTest::newRow("windows id") << windowsId << windowsId;
        QTest::newRow("device id") << deviceId << deviceId;
        QTest::newRow("path") << path << path;
    }

    void testUpgrade()
    {
        QFETCH(QVariant, value);
        QFETCH(QVariant, expected);

        Kit k;
        k.setValue(Constants::TOOL_ID, value);
        cmakeKitAspectFactory().upgrade(&k);
        QCOMPARE(k.value(Constants::TOOL_ID), expected);

        // A second load must not change it again.
        cmakeKitAspectFactory().upgrade(&k);
        QCOMPARE(k.value(Constants::TOOL_ID), expected);
    }

    void testUpgradeRegisteredId_data()
    {
        QTest::addColumn<bool>("withDevice");
        QTest::addColumn<bool>("onDevice"); // Whether the registered tool is on the device.
        QTest::addColumn<QString>("deviceTool");
        QTest::addColumn<bool>("pathDefault"); // Whether it is the default, found on the PATH.
        QTest::addColumn<bool>("kept");

        QTest::newRow("no device") << false << false << "" << false << true;
        QTest::newRow("other tool on device") << true << true << "/usr/bin/cmake" << false << true;
        QTest::newRow("device tool") << true << true << "/opt/cmake/bin/cmake" << false << false;
        QTest::newRow("host tool, remote device")
            << true << false << "/usr/bin/cmake" << false << false;
        QTest::newRow("default from PATH") << true << true << "/usr/bin/cmake" << true << false;
    }

    void testUpgradeRegisteredId()
    {
        QFETCH(bool, withDevice);
        QFETCH(bool, onDevice);
        QFETCH(QString, deviceTool);
        QFETCH(bool, pathDefault);
        QFETCH(bool, kept);

        IDevice::Ptr device;
        if (withDevice) {
            device = addTestDevice(deviceTool);
            QVERIFY(device);
        }
        const Id deviceId = device ? device->id() : Id();
        const auto removeDevice = qScopeGuard([deviceId] {
            if (deviceId.isValid())
                DeviceManager::removeDevice(deviceId);
        });

        const QString path = "/opt/cmake/bin/cmake";
        const FilePath executable = onDevice ? device->rootPath().withNewPath(path)
                                             : FilePath::fromString(path);
        const CMakeTool *oldDefault = CMakeToolManager::defaultCMakeTool();
        const Id oldDefaultId = oldDefault ? oldDefault->id() : Id();
        auto tool = std::make_unique<CMakeTool>(pathDefault ? DetectionSource::FromSystem
                                                            : DetectionSource::Manual,
                                                CMakeTool::createId());
        tool->setFilePath(executable);
        const Id id = tool->id();
        QVERIFY(CMakeToolManager::registerCMakeTool(std::move(tool)));
        if (pathDefault)
            CMakeToolManager::setDefaultCMakeTool(id);
        const auto cleanup = qScopeGuard([id, oldDefaultId] {
            CMakeToolManager::deregisterCMakeTool(id);
            CMakeToolManager::setDefaultCMakeTool(oldDefaultId);
        });

        Kit k;
        if (device)
            BuildDeviceKitAspect::setDeviceId(&k, deviceId);
        k.setValue(Constants::TOOL_ID, id.toSetting());
        cmakeKitAspectFactory().upgrade(&k);
        QCOMPARE(FilePath::fromSettings(k.value(Constants::TOOL_ID)),
                 kept ? executable : FilePath());
    }

    void testUpgradePathLikeId()
    {
        const IDevice::Ptr device = addTestDevice("/usr/bin/cmake");
        QVERIFY(device);
        const Id deviceId = device->id();
        const auto removeDevice = qScopeGuard([deviceId] {
            DeviceManager::removeDevice(deviceId);
        });

        // Tools registered before the kit stored paths could have the path as their id.
        const FilePath executable = FilePath::fromString("/opt/cmake/bin/cmake");
        auto tool = std::make_unique<CMakeTool>(DetectionSource::Manual,
                                                Id::fromString(executable.toUserOutput()));
        tool->setFilePath(executable);
        const Id id = tool->id();
        QVERIFY(CMakeToolManager::registerCMakeTool(std::move(tool)));
        const auto cleanup = qScopeGuard([id] { CMakeToolManager::deregisterCMakeTool(id); });

        Kit k;
        BuildDeviceKitAspect::setDeviceId(&k, deviceId);
        k.setValue(Constants::TOOL_ID, executable.toSettings());
        cmakeKitAspectFactory().upgrade(&k);
        QCOMPARE(k.value(Constants::TOOL_ID), executable.toSettings());
        cmakeKitAspectFactory().upgrade(&k);
        QCOMPARE(k.value(Constants::TOOL_ID), executable.toSettings());
    }

    void testToolForPath()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("Needs Unix symlinks and executable scripts");
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath script = FilePath::fromString(dir.path()).pathAppended("cmake");
        QVERIFY(!CMakeToolManager::cmakeToolForPath(script));
        QVERIFY(script.writeFileContents("#!/bin/sh\n"));
        QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        const FilePath link = script.parentDir().pathAppended("link-to-cmake");
        QVERIFY(QFile::link(script.path(), link.path()));

        QVERIFY(CMakeToolManager::cmakeToolForPath(link));
        QVERIFY(CMakeToolManager::cmakeToolForPath(script));

        // Follows the link when it is pointed elsewhere.
        const FilePath other = script.parentDir().pathAppended("other-cmake");
        QVERIFY(other.writeFileContents("#!/bin/sh\n"));
        QVERIFY(other.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(QFile::remove(link.path()));
        QVERIFY(QFile::link(other.path(), link.path()));
        const CMakeTool *relinked = CMakeToolManager::cmakeToolForPath(link);
        QVERIFY(relinked);
        QCOMPARE(relinked->cmakeExecutable(), other.canonicalPath());
        // A pinned path does not go through someone else's link.
        const CMakeTool *pinned = CMakeToolManager::cmakeToolForPath(script);
        QVERIFY(pinned);
        QCOMPARE(pinned->cmakeExecutable(), script.canonicalPath());
        QVERIFY(QFile::remove(link.path()));
        QVERIFY(QFile::link(script.path(), link.path()));

        // A tool registered later wins over the ad-hoc one.
        auto tool = std::make_unique<CMakeTool>(DetectionSource::Manual, CMakeTool::createId());
        tool->setFilePath(script);
        CMakeTool *registered = tool.get();
        const Id id = tool->id();
        QVERIFY(CMakeToolManager::registerCMakeTool(std::move(tool)));
        const auto cleanup = qScopeGuard([id] { CMakeToolManager::deregisterCMakeTool(id); });
        QCOMPARE(CMakeToolManager::cmakeToolForPath(link), registered);
    }

    void testToolForPathLifetime()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("Needs Unix symlinks and executable scripts");
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const FilePath script = FilePath::fromString(dir.path()).pathAppended("cmake");
        QVERIFY(script.writeFileContents("#!/bin/sh\n"));
        QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        const FilePath link = script.parentDir().pathAppended("link-to-cmake");
        QVERIFY(QFile::link(script.path(), link.path()));

        const CMakeTool *tool = CMakeToolManager::cmakeToolForPath(link);
        QVERIFY(tool);
        const Id id = tool->id();

        // Kept when the lookup cache is cleared, a caller may still hold the
        // pointer.
        auto unrelated = std::make_unique<CMakeTool>(DetectionSource::Manual,
                                                     CMakeTool::createId());
        unrelated->setFilePath(FilePath::fromString("/opt/unrelated/bin/cmake"));
        const Id unrelatedId = unrelated->id();
        QVERIFY(CMakeToolManager::registerCMakeTool(std::move(unrelated)));
        CMakeToolManager::deregisterCMakeTool(unrelatedId);
        tool = CMakeToolManager::cmakeToolForPath(link);
        QVERIFY(tool);
        QCOMPARE(tool->id(), id);
    }

    void testEffectiveExecutable_data()
    {
        QTest::addColumn<QString>("value");
        QTest::addColumn<QString>("deviceTool");
        QTest::addColumn<QString>("defaultTool"); // "local:" prefix for one on the desktop.
        QTest::addColumn<QString>("expected"); // On the build device, empty for none.

        QTest::newRow("empty, no device tool") << "" << "" << "" << "";
        QTest::newRow("empty, device tool") << "" << "/opt/cmake/bin/cmake" << ""
                                            << "/opt/cmake/bin/cmake";
        QTest::newRow("empty, default tool") << "" << "" << "/usr/local/bin/cmake" << "";
        QTest::newRow("empty, device tool over default")
            << "" << "/opt/cmake/bin/cmake" << "/usr/local/bin/cmake" << "/opt/cmake/bin/cmake";
        QTest::newRow("empty, default on other device")
            << "" << "" << "local:/usr/local/bin/cmake" << "";
        QTest::newRow("local value") << "/usr/bin/cmake" << "/opt/cmake/bin/cmake" << ""
                                     << "/opt/cmake/bin/cmake";
        QTest::newRow("other device") << "docker://4711/usr/bin/cmake" << "/opt/cmake/bin/cmake"
                                      << "" << "/opt/cmake/bin/cmake";
    }

    void testEffectiveExecutable()
    {
        QFETCH(QString, value);
        QFETCH(QString, deviceTool);
        QFETCH(QString, defaultTool);
        QFETCH(QString, expected);

        const IDevice::Ptr device = addTestDevice(deviceTool);
        QVERIFY(device);
        const Id deviceId = device->id();
        const auto cleanup = qScopeGuard([deviceId] { DeviceManager::removeDevice(deviceId); });

        const CMakeTool *oldDefault = CMakeToolManager::defaultCMakeTool();
        const Id oldDefaultId = oldDefault ? oldDefault->id() : Id();
        Id defaultId;
        if (!defaultTool.isEmpty()) {
            const FilePath path = defaultTool.startsWith("local:")
                                      ? FilePath::fromString(defaultTool.mid(6))
                                      : device->rootPath().withNewPath(defaultTool);
            auto tool = std::make_unique<CMakeTool>(DetectionSource::Manual,
                                                    CMakeTool::createId());
            tool->setFilePath(path);
            const CMakeTool *registered = tool.get();
            defaultId = tool->id();
            QVERIFY(CMakeToolManager::registerCMakeTool(std::move(tool)));
            CMakeToolManager::setDefaultCMakeTool(defaultId);
            QCOMPARE(CMakeToolManager::defaultCMakeTool(), registered);
        }
        const auto restoreDefault = qScopeGuard([defaultId, oldDefaultId] {
            if (!defaultId.isValid())
                return;
            CMakeToolManager::deregisterCMakeTool(defaultId);
            CMakeToolManager::setDefaultCMakeTool(oldDefaultId);
        });

        Kit k;
        BuildDeviceKitAspect::setDeviceId(&k, deviceId);
        if (!value.isEmpty())
            k.setValue(Constants::TOOL_ID, FilePath::fromString(value).toSettings());
        cmakeKitAspectFactory().fix(&k);

        const FilePath expectedPath = expected.isEmpty()
                                          ? FilePath()
                                          : device->rootPath().withNewPath(expected);
        QCOMPARE(effectiveCMakeExecutable(&k), expectedPath);
    }

    void testBuildDeviceChange()
    {
        const FilePath share = FilePath::fromString("//server/share/cmake/bin/cmake");
        Kit k;
        BuildDeviceKitAspect::setDeviceId(&k, ProjectExplorer::Constants::DESKTOP_DEVICE_ID);
        QVERIFY(BuildDeviceKitAspect::device(&k));
        CMakeKitAspect::setCMakeExecutable(&k, share);
        cmakeKitAspectFactory().fix(&k);
        // operator==() does not see the server of a UNC path.
        QCOMPARE(effectiveCMakeExecutable(&k).toUrlishString(), share.toUrlishString());

        const IDevice::Ptr device = addTestDevice("/usr/bin/cmake");
        QVERIFY(device);
        const Id deviceId = device->id();
        const auto cleanup = qScopeGuard([deviceId] { DeviceManager::removeDevice(deviceId); });
        BuildDeviceKitAspect::setDeviceId(&k, deviceId);
        cmakeKitAspectFactory().fix(&k);
        QVERIFY(!k.hasValue(Constants::TOOL_ID));
        QCOMPARE(effectiveCMakeExecutable(&k), device->rootPath().withNewPath("/usr/bin/cmake"));
    }

    void testSetup_data()
    {
        QTest::addColumn<QString>("deviceTool");
        QTest::addColumn<QString>("defaultTool"); // "local:" prefix for one on the desktop.
        QTest::addColumn<bool>("pathDefault"); // Whether the default was found on the PATH.
        QTest::addColumn<QString>("expected"); // On the build device, empty for no value.

        QTest::newRow("device tool") << "/opt/cmake/bin/cmake" << "/usr/local/bin/cmake"
                                     << false << "";
        QTest::newRow("registered default") << "" << "/usr/local/bin/cmake" << false
                                            << "/usr/local/bin/cmake";
        QTest::newRow("default from PATH") << "" << "/usr/local/bin/cmake" << true << "";
        QTest::newRow("default on other device") << "" << "local:/usr/local/bin/cmake" << false
                                                 << "";
    }

    void testSetup()
    {
        QFETCH(QString, deviceTool);
        QFETCH(QString, defaultTool);
        QFETCH(bool, pathDefault);
        QFETCH(QString, expected);

        const IDevice::Ptr device = addTestDevice(deviceTool);
        QVERIFY(device);
        const Id deviceId = device->id();
        const auto cleanup = qScopeGuard([deviceId] { DeviceManager::removeDevice(deviceId); });

        const CMakeTool *oldDefault = CMakeToolManager::defaultCMakeTool();
        const Id oldDefaultId = oldDefault ? oldDefault->id() : Id();
        const FilePath path = defaultTool.startsWith("local:")
                                  ? FilePath::fromString(defaultTool.mid(6))
                                  : device->rootPath().withNewPath(defaultTool);
        auto tool = std::make_unique<CMakeTool>(pathDefault ? DetectionSource::FromSystem
                                                            : DetectionSource::Manual,
                                                CMakeTool::createId());
        tool->setFilePath(path);
        const Id defaultId = tool->id();
        QVERIFY(CMakeToolManager::registerCMakeTool(std::move(tool)));
        CMakeToolManager::setDefaultCMakeTool(defaultId);
        const auto restoreDefault = qScopeGuard([defaultId, oldDefaultId] {
            CMakeToolManager::deregisterCMakeTool(defaultId);
            CMakeToolManager::setDefaultCMakeTool(oldDefaultId);
        });

        Kit k;
        BuildDeviceKitAspect::setDeviceId(&k, deviceId);
        cmakeKitAspectFactory().setup(&k);
        QCOMPARE(FilePath::fromSettings(k.value(Constants::TOOL_ID)),
                 expected.isEmpty() ? FilePath() : device->rootPath().withNewPath(expected));
    }
};

QObject *Internal::createCMakeKitAspectTest()
{
    return new CMakeKitAspectTest;
}

#endif // WITH_TESTS

} // namespace CMakeProjectManager

#ifdef WITH_TESTS
#include <cmakekitaspect.moc>
#endif
