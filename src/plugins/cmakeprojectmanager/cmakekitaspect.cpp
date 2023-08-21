// Copyright (C) 2016 Canonical Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakekitaspect.h"

#include "cmakeconfigitem.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "cmaketool.h"
#include "cmaketoolmanager.h"

#include <coreplugin/icore.h>

#include <ios/iosconstants.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager {

static bool isIos(const Kit *k)
{
    const Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
    return deviceType == Ios::Constants::IOS_DEVICE_TYPE
           || deviceType == Ios::Constants::IOS_SIMULATOR_TYPE;
}

static Id defaultCMakeToolId()
{
    CMakeTool *defaultTool = CMakeToolManager::defaultCMakeTool();
    return defaultTool ? defaultTool->id() : Id();
}

class CMakeKitAspectImpl final : public KitAspect
{
public:
    CMakeKitAspectImpl(Kit *kit, const KitAspectFactory *factory) : KitAspect(kit, factory),
        m_comboBox(createSubWidget<QComboBox>()),
        m_manageButton(createManageButton(Constants::Settings::TOOLS_ID))
    {
        m_comboBox->setSizePolicy(QSizePolicy::Ignored, m_comboBox->sizePolicy().verticalPolicy());
        m_comboBox->setEnabled(false);
        m_comboBox->setToolTip(factory->description());

        refresh();

        connect(m_comboBox, &QComboBox::currentIndexChanged,
                this, &CMakeKitAspectImpl::currentCMakeToolChanged);

        CMakeToolManager *cmakeMgr = CMakeToolManager::instance();
        connect(cmakeMgr, &CMakeToolManager::cmakeAdded, this, &CMakeKitAspectImpl::refresh);
        connect(cmakeMgr, &CMakeToolManager::cmakeRemoved, this, &CMakeKitAspectImpl::refresh);
        connect(cmakeMgr, &CMakeToolManager::cmakeUpdated, this, &CMakeKitAspectImpl::refresh);
    }

    ~CMakeKitAspectImpl() override
    {
        delete m_comboBox;
        delete m_manageButton;
    }

private:
    // KitAspectWidget interface
    void makeReadOnly() override { m_comboBox->setEnabled(false); }

    void addToLayout(Layouting::LayoutItem &builder) override
    {
        addMutableAction(m_comboBox);
        builder.addItem(m_comboBox);
        builder.addItem(m_manageButton);
    }

    void refresh() override
    {
        const GuardLocker locker(m_ignoreChanges);
        m_comboBox->clear();

        IDeviceConstPtr device = BuildDeviceKitAspect::device(kit());
        const FilePath rootPath = device->rootPath();

        const auto list = CMakeToolManager::cmakeTools();

        m_comboBox->setEnabled(!list.isEmpty());

        if (list.isEmpty()) {
            m_comboBox->addItem(Tr::tr("<No CMake Tool available>"), Id().toSetting());
            return;
        }

        const QList<CMakeTool *> same = Utils::filtered(list, [rootPath](CMakeTool *item) {
            return item->cmakeExecutable().isSameDevice(rootPath);
        });
        const QList<CMakeTool *> other = Utils::filtered(list, [rootPath](CMakeTool *item) {
            return !item->cmakeExecutable().isSameDevice(rootPath);
        });

        for (CMakeTool *item : same)
            m_comboBox->addItem(item->displayName(), item->id().toSetting());

        if (!same.isEmpty() && !other.isEmpty())
            m_comboBox->insertSeparator(m_comboBox->count());

        for (CMakeTool *item : other)
            m_comboBox->addItem(item->displayName(), item->id().toSetting());

        CMakeTool *tool = CMakeKitAspect::cmakeTool(m_kit);
        m_comboBox->setCurrentIndex(tool ? indexOf(tool->id()) : -1);
    }

    int indexOf(Id id)
    {
        for (int i = 0; i < m_comboBox->count(); ++i) {
            if (id == Id::fromSetting(m_comboBox->itemData(i)))
                return i;
        }
        return -1;
    }

    void currentCMakeToolChanged(int index)
    {
        if (m_ignoreChanges.isLocked())
            return;

        const Id id = Id::fromSetting(m_comboBox->itemData(index));
        CMakeKitAspect::setCMakeTool(m_kit, id);
    }

    Guard m_ignoreChanges;
    QComboBox *m_comboBox;
    QWidget *m_manageButton;
};

CMakeKitAspectFactory::CMakeKitAspectFactory()
{
    setObjectName(QLatin1String("CMakeKitAspect"));
    setId(Constants::TOOL_ID);
    setDisplayName(Tr::tr("CMake Tool"));
    setDescription(Tr::tr("The CMake Tool to use when building a project with CMake.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(20000);

    //make sure the default value is set if a selected CMake is removed
    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeRemoved,
            this, [this] { for (Kit *k : KitManager::kits()) fix(k); });

    //make sure the default value is set if a new default CMake is set
    connect(CMakeToolManager::instance(), &CMakeToolManager::defaultCMakeChanged,
            this, [this] { for (Kit *k : KitManager::kits()) fix(k); });
}

Id CMakeKitAspect::id()
{
    return Constants::TOOL_ID;
}

Id CMakeKitAspect::cmakeToolId(const Kit *k)
{
    if (!k)
        return {};
    return Id::fromSetting(k->value(Constants::TOOL_ID));
}

CMakeTool *CMakeKitAspect::cmakeTool(const Kit *k)
{
    return CMakeToolManager::findById(cmakeToolId(k));
}

void CMakeKitAspect::setCMakeTool(Kit *k, const Id id)
{
    const Id toSet = id.isValid() ? id : defaultCMakeToolId();
    QTC_ASSERT(!id.isValid() || CMakeToolManager::findById(toSet), return);
    if (k)
        k->setValue(Constants::TOOL_ID, toSet.toSetting());
}

Tasks CMakeKitAspectFactory::validate(const Kit *k) const
{
    Tasks result;
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (tool && tool->isValid()) {
        CMakeTool::Version version = tool->version();
        if (version.major < 3 || (version.major == 3 && version.minor < 14)) {
            result << BuildSystemTask(Task::Warning,
                CMakeKitAspect::msgUnsupportedVersion(version.fullVersion));
        }
    }
    return result;
}

void CMakeKitAspectFactory::setup(Kit *k)
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (tool)
        return;

    // Look for a suitable auto-detected one:
    const QString kitSource = k->autoDetectionSource();
    for (CMakeTool *tool : CMakeToolManager::cmakeTools()) {
        const QString toolSource = tool->detectionSource();
        if (!toolSource.isEmpty() && toolSource == kitSource) {
            CMakeKitAspect::setCMakeTool(k, tool->id());
            return;
        }
    }

    CMakeKitAspect::setCMakeTool(k, defaultCMakeToolId());
}

void CMakeKitAspectFactory::fix(Kit *k)
{
    setup(k);
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
    expander->registerFileVariables("CMake:Executable", Tr::tr("Path to the cmake executable"),
        [k] {
            CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
            return tool ? tool->cmakeExecutable() : FilePath();
        });
}

QSet<Id> CMakeKitAspectFactory::availableFeatures(const Kit *k) const
{
    if (CMakeKitAspect::cmakeTool(k))
        return { CMakeProjectManager::Constants::CMAKE_FEATURE_ID };
    return {};
}

QString CMakeKitAspect::msgUnsupportedVersion(const QByteArray &versionString)
{
    return Tr::tr("CMake version %1 is unsupported. Update to "
              "version 3.15 (with file-api) or later.")
        .arg(QString::fromUtf8(versionString));
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
        const CMakeTool *tool = CMakeKitAspect::cmakeTool(kit);
        connect(this, &KitAspect::labelLinkActivated, this, [=](const QString &) {
            CMakeTool::openCMakeHelpUrl(tool, "%1/manual/cmake-generators.7.html");
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
    void makeReadOnly() override { m_changeButton->setEnabled(false); }

    void addToLayout(Layouting::LayoutItem &parent) override
    {
        addMutableAction(m_label);
        parent.addItem(m_label);
        parent.addItem(m_changeButton);
    }

    void refresh() override
    {
        CMakeTool *const tool = CMakeKitAspect::cmakeTool(m_kit);
        if (tool != m_currentTool)
            m_currentTool = tool;

        m_changeButton->setEnabled(m_currentTool);
        const QString generator = CMakeGeneratorKitAspect::generator(kit());
        const QString extraGenerator = CMakeGeneratorKitAspect::extraGenerator(kit());
        const QString platform = CMakeGeneratorKitAspect::platform(kit());
        const QString toolset = CMakeGeneratorKitAspect::toolset(kit());

        QStringList messageLabel;
        if (!extraGenerator.isEmpty())
            messageLabel << extraGenerator << " - ";

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
        auto extraGeneratorCombo = new QComboBox;
        auto platformEdit = new QLineEdit;
        auto toolsetEdit = new QLineEdit;

        int row = 0;
        layout->addWidget(new QLabel(QLatin1String("Executable:")));
        layout->addWidget(cmakeLabel, row, 1);

        ++row;
        layout->addWidget(new QLabel(Tr::tr("Generator:")), row, 0);
        layout->addWidget(generatorCombo, row, 1);

        ++row;
        layout->addWidget(new QLabel(Tr::tr("Extra generator:")), row, 0);
        layout->addWidget(extraGeneratorCombo, row, 1);

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

        auto updateDialog = [&generatorList, generatorCombo, extraGeneratorCombo,
                platformEdit, toolsetEdit](const QString &name) {
            const auto it = std::find_if(generatorList.constBegin(), generatorList.constEnd(),
                                   [name](const CMakeTool::Generator &g) { return g.name == name; });
            QTC_ASSERT(it != generatorList.constEnd(), return);
            generatorCombo->setCurrentText(name);

            extraGeneratorCombo->clear();
            extraGeneratorCombo->addItem(Tr::tr("<none>"), QString());
            for (const QString &eg : std::as_const(it->extraGenerators))
                extraGeneratorCombo->addItem(eg, eg);
            extraGeneratorCombo->setEnabled(extraGeneratorCombo->count() > 1);

            platformEdit->setEnabled(it->supportsPlatform);
            toolsetEdit->setEnabled(it->supportsToolset);
        };

        updateDialog(CMakeGeneratorKitAspect::generator(kit()));

        generatorCombo->setCurrentText(CMakeGeneratorKitAspect::generator(kit()));
        extraGeneratorCombo->setCurrentText(CMakeGeneratorKitAspect::extraGenerator(kit()));
        platformEdit->setText(platformEdit->isEnabled() ? CMakeGeneratorKitAspect::platform(kit()) : QString());
        toolsetEdit->setText(toolsetEdit->isEnabled() ? CMakeGeneratorKitAspect::toolset(kit()) : QString());

        connect(generatorCombo, &QComboBox::currentTextChanged, updateDialog);

        if (changeDialog->exec() == QDialog::Accepted) {
            if (!changeDialog)
                return;

            CMakeGeneratorKitAspect::set(kit(), generatorCombo->currentText(),
                                         extraGeneratorCombo->currentData().toString(),
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
                  const QString &extraGenerator_ = QString(),
                  const QString &platform_ = QString(),
                  const QString &toolset_ = QString())
        : generator(generator_)
        , extraGenerator(extraGenerator_)
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
    setObjectName(QLatin1String("CMakeGeneratorKitAspect"));
    setId(GENERATOR_ID);
    setDisplayName(Tr::tr("CMake <a href=\"generator\">generator</a>"));
    setDescription(Tr::tr("CMake generator defines how a project is built when using CMake.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(19000);
}

QString CMakeGeneratorKitAspect::generator(const Kit *k)
{
    return generatorInfo(k).generator;
}

QString CMakeGeneratorKitAspect::extraGenerator(const Kit *k)
{
    return generatorInfo(k).extraGenerator;
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

void CMakeGeneratorKitAspect::setExtraGenerator(Kit *k, const QString &extraGenerator)
{
    GeneratorInfo info = generatorInfo(k);
    info.extraGenerator = extraGenerator;
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
                                  const QString &extraGenerator,
                                  const QString &platform,
                                  const QString &toolset)
{
    GeneratorInfo info(generator, extraGenerator, platform, toolset);
    setGeneratorInfo(k, info);
}

QStringList CMakeGeneratorKitAspect::generatorArguments(const Kit *k)
{
    QStringList result;
    GeneratorInfo info = generatorInfo(k);
    if (info.generator.isEmpty())
        return result;

    if (info.extraGenerator.isEmpty()) {
        result.append("-G" + info.generator);
    } else {
        result.append("-G" + info.extraGenerator + " - " + info.generator);
    }

    if (!info.platform.isEmpty())
        result.append("-A" + info.platform);

    if (!info.toolset.isEmpty())
        result.append("-T" + info.toolset);

    return result;
}

CMakeConfig CMakeGeneratorKitAspect::generatorCMakeConfig(const ProjectExplorer::Kit *k)
{
    CMakeConfig config;

    GeneratorInfo info = generatorInfo(k);
    if (info.generator.isEmpty())
        return config;

    config << CMakeConfigItem("CMAKE_GENERATOR", info.generator.toUtf8());

    if (!info.extraGenerator.isEmpty())
        config << CMakeConfigItem("CMAKE_EXTRA_GENERATOR", info.extraGenerator.toUtf8());

    if (!info.platform.isEmpty())
        config << CMakeConfigItem("CMAKE_GENERATOR_PLATFORM", info.platform.toUtf8());

    if (!info.toolset.isEmpty())
        config << CMakeConfigItem("CMAKE_GENERATOR_TOOLSET", info.toolset.toUtf8());

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
        const bool hasNinja = [k, tool] {
            if (Internal::settings().ninjaPath().isEmpty()) {
                auto findNinja = [](const Environment &env) -> bool {
                    return !env.searchInPath("ninja").isEmpty();
                };
                if (!findNinja(tool->filePath().deviceEnvironment()))
                    return findNinja(k->buildEnvironment());
            }
            return true;
        }();

        if (hasNinja)
            return GeneratorInfo("Ninja").toVariant();
    }

    if (tool->filePath().osType() == OsTypeWindows) {
        // *sigh* Windows with its zoo of incompatible stuff again...
        ToolChain *tc = ToolChainKitAspect::cxxToolChain(k);
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
            if (ProjectExplorerPlugin::projectExplorerSettings().useJom) {
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
            return g.matches(info.generator, info.extraGenerator);
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

    if (!tool)
        return;
    QList<CMakeTool::Generator> known = tool->supportedGenerators();
    auto it = std::find_if(known.constBegin(), known.constEnd(),
                           [info](const CMakeTool::Generator &g) {
        return g.matches(info.generator, info.extraGenerator);
    });
    if (it == known.constEnd()) {
        GeneratorInfo dv;
        dv.fromVariant(defaultValue(k));
        setGeneratorInfo(k, dv);
    } else {
        const GeneratorInfo dv(isIos(k) ? QString("Xcode") : info.generator,
                               info.extraGenerator,
                               it->supportsPlatform ? info.platform : QString(),
                               it->supportsToolset ? info.toolset : QString());
        setGeneratorInfo(k, dv);
    }
}

void CMakeGeneratorKitAspectFactory::upgrade(Kit *k)
{
    QTC_ASSERT(k, return);

    const QVariant value = k->value(GENERATOR_ID);
    if (value.type() != QVariant::Map) {
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
        if (env.searchInPath("jom.exe").exists())
            return;
        env.appendOrSetPath(Core::ICore::libexecPath());
        env.appendOrSetPath(Core::ICore::libexecPath("jom"));
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

class CMakeConfigurationKitAspectWidget final : public KitAspect
{
public:
    CMakeConfigurationKitAspectWidget(Kit *kit, const KitAspectFactory *factory)
        : KitAspect(kit, factory),
          m_summaryLabel(createSubWidget<ElidingLabel>()),
          m_manageButton(createSubWidget<QPushButton>())
    {
        refresh();
        m_manageButton->setText(Tr::tr("Change..."));
        connect(m_manageButton, &QAbstractButton::clicked,
                this, &CMakeConfigurationKitAspectWidget::editConfigurationChanges);
    }

private:
    // KitAspectWidget interface
    void addToLayout(Layouting::LayoutItem &parent) override
    {
        addMutableAction(m_summaryLabel);
        parent.addItem(m_summaryLabel);
        parent.addItem(m_manageButton);
    }

    void makeReadOnly() override
    {
        m_manageButton->setEnabled(false);
        if (m_dialog)
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

        const CMakeTool *tool = CMakeKitAspect::cmakeTool(kit());

        m_dialog = new QDialog(m_summaryLabel->window());
        m_dialog->setWindowTitle(Tr::tr("Edit CMake Configuration"));
        auto layout = new QVBoxLayout(m_dialog);
        m_editor = new QPlainTextEdit;
        auto editorLabel = new QLabel(m_dialog);
        editorLabel->setText(Tr::tr("Enter one CMake <a href=\"variable\">variable</a> per line.<br/>"
                                "To set a variable, use -D&lt;variable&gt;:&lt;type&gt;=&lt;value&gt;.<br/>"
                                "&lt;type&gt; can have one of the following values: FILEPATH, PATH, "
                                "BOOL, INTERNAL, or STRING."));
        connect(editorLabel, &QLabel::linkActivated, this, [=](const QString &) {
            CMakeTool::openCMakeHelpUrl(tool, "%1/manual/cmake-variables.7.html");
        });
        m_editor->setMinimumSize(800, 200);

        auto chooser = new VariableChooser(m_dialog);
        chooser->addSupportedWidget(m_editor);
        chooser->addMacroExpanderProvider([this] { return kit()->macroExpander(); });

        m_additionalEditor = new QLineEdit;
        auto additionalLabel = new QLabel(m_dialog);
        additionalLabel->setText(Tr::tr("Additional CMake <a href=\"options\">options</a>:"));
        connect(additionalLabel, &QLabel::linkActivated, this, [=](const QString &) {
            CMakeTool::openCMakeHelpUrl(tool, "%1/manual/cmake.1.html#options");
        });

        auto additionalChooser = new VariableChooser(m_dialog);
        additionalChooser->addSupportedWidget(m_additionalEditor);
        additionalChooser->addMacroExpanderProvider([this] { return kit()->macroExpander(); });

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
        connect(m_dialog, &QDialog::accepted, this, &CMakeConfigurationKitAspectWidget::acceptChangesDialog);
        connect(m_dialog, &QDialog::rejected, this, &CMakeConfigurationKitAspectWidget::closeChangesDialog);
        connect(buttons->button(QDialogButtonBox::Apply), &QAbstractButton::clicked,
                this, &CMakeConfigurationKitAspectWidget::applyChanges);

        refresh();
        m_dialog->show();
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
    setObjectName(QLatin1String("CMakeConfigurationKitAspect"));
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

QString CMakeConfigurationKitAspect::additionalConfiguration(const ProjectExplorer::Kit *k)
{
    if (!k)
        return QString();
    return k->value(ADDITIONAL_CONFIGURATION_ID).toString();
}

void CMakeConfigurationKitAspect::setAdditionalConfiguration(ProjectExplorer::Kit *k, const QString &config)
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
            result << item;
    }
    setConfiguration(k, result);
}

QStringList CMakeConfigurationKitAspect::toArgumentsList(const Kit *k)
{
    QStringList current = Utils::transform(CMakeConfigurationKitAspect::configuration(k).toList(),
                                           [](const CMakeConfigItem &i) {
                                               return i.toArgument(nullptr);
                                           });
    current = Utils::filtered(current, [](const QString &s) { return s != "-D" || s != "-U"; });
    return current;
}

CMakeConfig CMakeConfigurationKitAspect::defaultConfiguration(const Kit *k)
{
    Q_UNUSED(k)
    CMakeConfig config;
    // Qt4:
    config << CMakeConfigItem(CMAKE_QMAKE_KEY, CMakeConfigItem::FILEPATH, "%{Qt:qmakeExecutable}");
    // Qt5:
    config << CMakeConfigItem(CMAKE_PREFIX_PATH_KEY, CMakeConfigItem::PATH, "%{Qt:QT_INSTALL_PREFIX}");

    config << CMakeConfigItem(CMAKE_C_TOOLCHAIN_KEY, CMakeConfigItem::FILEPATH, "%{Compiler:Executable:C}");
    config << CMakeConfigItem(CMAKE_CXX_TOOLCHAIN_KEY, CMakeConfigItem::FILEPATH, "%{Compiler:Executable:Cxx}");

    return config;
}

void CMakeConfigurationKitAspect::setCMakePreset(Kit *k, const QString &presetName)
{
    CMakeConfig config = configuration(k);
    config.prepend(
        CMakeConfigItem(QTC_CMAKE_PRESET_KEY, CMakeConfigItem::INTERNAL, presetName.toUtf8()));

    setConfiguration(k, config);
}

CMakeConfigItem CMakeConfigurationKitAspect::cmakePresetConfigItem(const ProjectExplorer::Kit *k)
{
    const CMakeConfig config = configuration(k);
    return Utils::findOrDefault(config, [](const CMakeConfigItem &item) {
        return item.key == QTC_CMAKE_PRESET_KEY;
    });
}

QVariant CMakeConfigurationKitAspectFactory::defaultValue(const Kit *k) const
{
    // FIXME: Convert preload scripts
    CMakeConfig config = CMakeConfigurationKitAspect::defaultConfiguration(k);
    const QStringList tmp = Utils::transform(config.toList(),
                                             [](const CMakeConfigItem &i) { return i.toString(); });
    return tmp;
}

Tasks CMakeConfigurationKitAspectFactory::validate(const Kit *k) const
{
    QTC_ASSERT(k, return Tasks());

    const CMakeTool *const cmake = CMakeKitAspect::cmakeTool(k);
    if (!cmake)
        return Tasks();

    const QtSupport::QtVersion *const version = QtSupport::QtKitAspect::qtVersion(k);
    const ToolChain *const tcC = ToolChainKitAspect::cToolChain(k);
    const ToolChain *const tcCxx = ToolChainKitAspect::cxxToolChain(k);
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
            qmakePath = cmake->cmakeExecutable().withNewPath(expandedValue);
        else if (i.key == CMAKE_C_TOOLCHAIN_KEY)
            tcCPath = cmake->cmakeExecutable().withNewPath(expandedValue);
        else if (i.key == CMAKE_CXX_TOOLCHAIN_KEY)
            tcCxxPath = cmake->cmakeExecutable().withNewPath(expandedValue);
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
        } else if (tcCPath != tcC->compilerCommand() && tcCPath != tcCPath.withNewMappedPath(tcC->compilerCommand())) {
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
        } else if (tcCxxPath != tcCxx->compilerCommand() && tcCxxPath != tcCxxPath.withNewMappedPath(tcCxx->compilerCommand())) {
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
    return new CMakeConfigurationKitAspectWidget(k, this);
}

} // namespace CMakeProjectManager
