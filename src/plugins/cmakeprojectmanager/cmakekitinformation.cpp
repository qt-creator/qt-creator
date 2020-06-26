/****************************************************************************
**
** Copyright (C) 2016 Canonical Ltd.
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

#include "cmakekitinformation.h"

#include "cmakeprojectconstants.h"
#include "cmakeprojectplugin.h"
#include "cmakespecificsettings.h"
#include "cmaketool.h"
#include "cmaketoolmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <app/app_version.h>

#include <utils/algorithm.h>
#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
// --------------------------------------------------------------------
// CMakeKitAspect:
// --------------------------------------------------------------------

static Utils::Id defaultCMakeToolId()
{
    CMakeTool *defaultTool = CMakeToolManager::defaultCMakeTool();
    return defaultTool ? defaultTool->id() : Utils::Id();
}

static const char TOOL_ID[] = "CMakeProjectManager.CMakeKitInformation";

class CMakeKitAspectWidget : public KitAspectWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeKitAspect)
public:
    CMakeKitAspectWidget(Kit *kit, const KitAspect *ki) : KitAspectWidget(kit, ki),
        m_comboBox(new QComboBox),
        m_manageButton(new QPushButton(KitAspectWidget::msgManage()))
    {
        m_comboBox->setSizePolicy(QSizePolicy::Ignored, m_comboBox->sizePolicy().verticalPolicy());
        m_comboBox->setEnabled(false);
        m_comboBox->setToolTip(ki->description());

        foreach (CMakeTool *tool, CMakeToolManager::cmakeTools())
            cmakeToolAdded(tool->id());

        updateComboBox();
        refresh();
        connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &CMakeKitAspectWidget::currentCMakeToolChanged);

        m_manageButton->setContentsMargins(0, 0, 0, 0);
        connect(m_manageButton, &QPushButton::clicked,
                this, &CMakeKitAspectWidget::manageCMakeTools);

        CMakeToolManager *cmakeMgr = CMakeToolManager::instance();
        connect(cmakeMgr, &CMakeToolManager::cmakeAdded,
                this, &CMakeKitAspectWidget::cmakeToolAdded);
        connect(cmakeMgr, &CMakeToolManager::cmakeRemoved,
                this, &CMakeKitAspectWidget::cmakeToolRemoved);
        connect(cmakeMgr, &CMakeToolManager::cmakeUpdated,
                this, &CMakeKitAspectWidget::cmakeToolUpdated);
    }

    ~CMakeKitAspectWidget() override
    {
        delete m_comboBox;
        delete m_manageButton;
    }

private:
    // KitAspectWidget interface
    void makeReadOnly() override { m_comboBox->setEnabled(false); }
    QWidget *mainWidget() const override { return m_comboBox; }
    QWidget *buttonWidget() const override { return m_manageButton; }

    void refresh() override
    {
        CMakeTool *tool = CMakeKitAspect::cmakeTool(m_kit);
        m_comboBox->setCurrentIndex(tool ? indexOf(tool->id()) : -1);
    }

    int indexOf(const Utils::Id &id)
    {
        for (int i = 0; i < m_comboBox->count(); ++i) {
            if (id == Utils::Id::fromSetting(m_comboBox->itemData(i)))
                return i;
        }
        return -1;
    }

    void updateComboBox()
    {
        // remove unavailable cmake tool:
        int pos = indexOf(Utils::Id());
        if (pos >= 0)
            m_comboBox->removeItem(pos);

        if (m_comboBox->count() == 0) {
            m_comboBox->addItem(tr("<No CMake Tool available>"),
                                Utils::Id().toSetting());
            m_comboBox->setEnabled(false);
        } else {
            m_comboBox->setEnabled(true);
        }
    }

    void cmakeToolAdded(const Utils::Id &id)
    {
        const CMakeTool *tool = CMakeToolManager::findById(id);
        QTC_ASSERT(tool, return);

        m_comboBox->addItem(tool->displayName(), tool->id().toSetting());
        updateComboBox();
        refresh();
    }

    void cmakeToolUpdated(const Utils::Id &id)
    {
        const int pos = indexOf(id);
        QTC_ASSERT(pos >= 0, return);

        const CMakeTool *tool = CMakeToolManager::findById(id);
        QTC_ASSERT(tool, return);

        m_comboBox->setItemText(pos, tool->displayName());
    }

    void cmakeToolRemoved(const Utils::Id &id)
    {
        const int pos = indexOf(id);
        QTC_ASSERT(pos >= 0, return);

        // do not handle the current index changed signal
        m_removingItem = true;
        m_comboBox->removeItem(pos);
        m_removingItem = false;

        // update the checkbox and set the current index
        updateComboBox();
        refresh();
    }

    void currentCMakeToolChanged(int index)
    {
        if (m_removingItem)
            return;

        const Utils::Id id = Utils::Id::fromSetting(m_comboBox->itemData(index));
        CMakeKitAspect::setCMakeTool(m_kit, id);
    }

    void manageCMakeTools()
    {
        Core::ICore::showOptionsDialog(Constants::CMAKE_SETTINGS_PAGE_ID, buttonWidget());
    }

    bool m_removingItem = false;
    QComboBox *m_comboBox;
    QPushButton *m_manageButton;
};

CMakeKitAspect::CMakeKitAspect()
{
    setObjectName(QLatin1String("CMakeKitAspect"));
    setId(TOOL_ID);
    setDisplayName(tr("CMake Tool"));
    setDescription(tr("The CMake Tool to use when building a project with CMake.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(20000);

    //make sure the default value is set if a selected CMake is removed
    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeRemoved,
            [this]() { foreach (Kit *k, KitManager::kits()) fix(k); });

    //make sure the default value is set if a new default CMake is set
    connect(CMakeToolManager::instance(), &CMakeToolManager::defaultCMakeChanged,
            [this]() { foreach (Kit *k, KitManager::kits()) fix(k); });
}

Utils::Id CMakeKitAspect::id()
{
    return TOOL_ID;
}

Utils::Id CMakeKitAspect::cmakeToolId(const Kit *k)
{
    if (!k)
        return {};
    return Utils::Id::fromSetting(k->value(TOOL_ID));
}

CMakeTool *CMakeKitAspect::cmakeTool(const Kit *k)
{
    return CMakeToolManager::findById(cmakeToolId(k));
}

void CMakeKitAspect::setCMakeTool(Kit *k, const Utils::Id id)
{
    const Utils::Id toSet = id.isValid() ? id : defaultCMakeToolId();
    QTC_ASSERT(!id.isValid() || CMakeToolManager::findById(toSet), return);
    if (k)
        k->setValue(TOOL_ID, toSet.toSetting());
}

Tasks CMakeKitAspect::validate(const Kit *k) const
{
    Tasks result;
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (tool) {
        CMakeTool::Version version = tool->version();
        if (version.major < 3 || (version.major == 3 && version.minor < 14)) {
            result << BuildSystemTask(Task::Warning,
                                      tr("CMake version %1 is unsupported. Please update to "
                                         "version 3.14 (with file-api) or later.")
                                          .arg(QString::fromUtf8(version.fullVersion)));
        }
    }
    return result;
}

void CMakeKitAspect::setup(Kit *k)
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (!tool)
        setCMakeTool(k, defaultCMakeToolId());
}

void CMakeKitAspect::fix(Kit *k)
{
    setup(k);
}

KitAspect::ItemList CMakeKitAspect::toUserOutput(const Kit *k) const
{
    const CMakeTool *const tool = cmakeTool(k);
    return {{tr("CMake"), tool ? tool->displayName() : tr("Unconfigured")}};
}

KitAspectWidget *CMakeKitAspect::createConfigWidget(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new CMakeKitAspectWidget(k, this);
}

void CMakeKitAspect::addToMacroExpander(Kit *k, Utils::MacroExpander *expander) const
{
    QTC_ASSERT(k, return);
    expander->registerFileVariables("CMake:Executable", tr("Path to the cmake executable"),
                                    [k]() -> QString {
                                        CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
                                        return tool ? tool->cmakeExecutable().toString() : QString();
    });
}

QSet<Utils::Id> CMakeKitAspect::availableFeatures(const Kit *k) const
{
    if (cmakeTool(k))
        return { CMakeProjectManager::Constants::CMAKE_FEATURE_ID };
    return {};
}

// --------------------------------------------------------------------
// CMakeGeneratorKitAspect:
// --------------------------------------------------------------------

static const char GENERATOR_ID[] = "CMake.GeneratorKitInformation";

static const char GENERATOR_KEY[] = "Generator";
static const char EXTRA_GENERATOR_KEY[] = "ExtraGenerator";
static const char PLATFORM_KEY[] = "Platform";
static const char TOOLSET_KEY[] = "Toolset";

class CMakeGeneratorKitAspectWidget : public KitAspectWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeGeneratorKitAspect)
public:
    CMakeGeneratorKitAspectWidget(Kit *kit, const ::KitAspect *ki)
        : KitAspectWidget(kit, ki),
          m_label(new Utils::ElidingLabel),
          m_changeButton(new QPushButton)
    {
        m_label->setToolTip(ki->description());
        m_changeButton->setText(tr("Change..."));
        refresh();
        connect(m_changeButton, &QPushButton::clicked,
                this, &CMakeGeneratorKitAspectWidget::changeGenerator);
    }

    ~CMakeGeneratorKitAspectWidget() override
    {
        delete m_label;
        delete m_changeButton;
    }

private:
    // KitAspectWidget interface
    void makeReadOnly() override { m_changeButton->setEnabled(false); }
    QWidget *mainWidget() const override { return m_label; }
    QWidget *buttonWidget() const override { return m_changeButton; }

    void refresh() override
    {
        if (m_ignoreChange)
            return;

        CMakeTool *const tool = CMakeKitAspect::cmakeTool(m_kit);
        if (tool != m_currentTool)
            m_currentTool = tool;

        m_changeButton->setEnabled(m_currentTool);
        const QString generator = CMakeGeneratorKitAspect::generator(kit());
        const QString extraGenerator = CMakeGeneratorKitAspect::extraGenerator(kit());
        const QString platform = CMakeGeneratorKitAspect::platform(kit());
        const QString toolset = CMakeGeneratorKitAspect::toolset(kit());

        const QString message = tr("%1 - %2, Platform: %3, Toolset: %4")
                .arg(extraGenerator.isEmpty() ? tr("<none>") : extraGenerator)
                .arg(generator.isEmpty() ? tr("<none>") : generator)
                .arg(platform.isEmpty() ? tr("<none>") : platform)
                .arg(toolset.isEmpty() ? tr("<none>") : toolset);
        m_label->setText(message);
    }

    void changeGenerator()
    {
        QPointer<QDialog> changeDialog = new QDialog(m_changeButton);

        // Disable help button in titlebar on windows:
        Qt::WindowFlags flags = changeDialog->windowFlags();
        flags |= Qt::MSWindowsFixedSizeDialogHint;
        changeDialog->setWindowFlags(flags);

        changeDialog->setWindowTitle(tr("CMake Generator"));

        auto *layout = new QGridLayout(changeDialog);
        layout->setSizeConstraint(QLayout::SetFixedSize);

        auto *cmakeLabel = new QLabel;
        cmakeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto *generatorCombo = new QComboBox;
        auto *extraGeneratorCombo = new QComboBox;
        auto *platformEdit = new QLineEdit;
        auto *toolsetEdit = new QLineEdit;

        int row = 0;
        layout->addWidget(new QLabel(QLatin1String("Executable:")));
        layout->addWidget(cmakeLabel, row, 1);

        ++row;
        layout->addWidget(new QLabel(tr("Generator:")), row, 0);
        layout->addWidget(generatorCombo, row, 1);

        ++row;
        layout->addWidget(new QLabel(tr("Extra generator:")), row, 0);
        layout->addWidget(extraGeneratorCombo, row, 1);

        ++row;
        layout->addWidget(new QLabel(tr("Platform:")), row, 0);
        layout->addWidget(platformEdit, row, 1);

        ++row;
        layout->addWidget(new QLabel(tr("Toolset:")), row, 0);
        layout->addWidget(toolsetEdit, row, 1);

        ++row;
        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        layout->addWidget(bb, row, 0, 1, 2);

        connect(bb, &QDialogButtonBox::accepted, changeDialog.data(), &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, changeDialog.data(), &QDialog::reject);

        cmakeLabel->setText(m_currentTool->cmakeExecutable().toUserOutput());

        QList<CMakeTool::Generator> generatorList = m_currentTool->supportedGenerators();
        Utils::sort(generatorList, &CMakeTool::Generator::name);

        for (auto it = generatorList.constBegin(); it != generatorList.constEnd(); ++it)
            generatorCombo->addItem(it->name);

        auto updateDialog = [&generatorList, generatorCombo, extraGeneratorCombo,
                platformEdit, toolsetEdit](const QString &name) {
            auto it = std::find_if(generatorList.constBegin(), generatorList.constEnd(),
                                   [name](const CMakeTool::Generator &g) { return g.name == name; });
            QTC_ASSERT(it != generatorList.constEnd(), return);
            generatorCombo->setCurrentText(name);

            extraGeneratorCombo->clear();
            extraGeneratorCombo->addItem(tr("<none>"), QString());
            foreach (const QString &eg, it->extraGenerators)
                extraGeneratorCombo->addItem(eg, eg);
            extraGeneratorCombo->setEnabled(extraGeneratorCombo->count() > 1);

            platformEdit->setEnabled(it->supportsPlatform);
            toolsetEdit->setEnabled(it->supportsToolset);
        };

        updateDialog(CMakeGeneratorKitAspect::generator(kit()));

        generatorCombo->setCurrentText(CMakeGeneratorKitAspect::generator(kit()));
        extraGeneratorCombo->setCurrentText(CMakeGeneratorKitAspect::extraGenerator(kit()));
        platformEdit->setText(platformEdit->isEnabled() ? CMakeGeneratorKitAspect::platform(kit()) : QLatin1String("<unsupported>"));
        toolsetEdit->setText(toolsetEdit->isEnabled() ? CMakeGeneratorKitAspect::toolset(kit()) : QLatin1String("<unsupported>"));

        connect(generatorCombo, &QComboBox::currentTextChanged, updateDialog);

        if (changeDialog->exec() == QDialog::Accepted) {
            if (!changeDialog)
                return;

            CMakeGeneratorKitAspect::set(kit(), generatorCombo->currentText(),
                                         extraGeneratorCombo->currentData().toString(),
                                         platformEdit->isEnabled() ? platformEdit->text() : QString(),
                                         toolsetEdit->isEnabled() ? toolsetEdit->text() : QString());
        }
    }

    bool m_ignoreChange = false;
    Utils::ElidingLabel *m_label;
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

CMakeGeneratorKitAspect::CMakeGeneratorKitAspect()
{
    setObjectName(QLatin1String("CMakeGeneratorKitAspect"));
    setId(GENERATOR_ID);
    setDisplayName(tr("CMake generator"));
    setDescription(tr("CMake generator defines how a project is built when using CMake.<br>"
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

QVariant CMakeGeneratorKitAspect::defaultValue(const Kit *k) const
{
    QTC_ASSERT(k, return QVariant());

    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (!tool)
        return QVariant();

    const QList<CMakeTool::Generator> known = tool->supportedGenerators();
    auto it = std::find_if(known.constBegin(), known.constEnd(), [](const CMakeTool::Generator &g) {
        return g.matches("Ninja");
    });
    if (it != known.constEnd()) {
        const bool hasNinja = [k]() {
            Internal::CMakeSpecificSettings *settings
                = Internal::CMakeProjectPlugin::projectTypeSpecificSettings();

            if (settings->ninjaPath().isEmpty()) {
                Utils::Environment env = Utils::Environment::systemEnvironment();
                k->addToEnvironment(env);
                return !env.searchInPath("ninja").isEmpty();
            }
            return true;
        }();

        if (hasNinja)
            return GeneratorInfo("Ninja").toVariant();
    }

    if (Utils::HostOsInfo::isWindowsHost()) {
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

Tasks CMakeGeneratorKitAspect::validate(const Kit *k) const
{
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    if (!tool)
        return {};

    Tasks result;
    const auto addWarning = [&result](const QString &desc) {
        result << BuildSystemTask(Task::Warning, desc);
    };

    if (!tool->isValid()) {
        addWarning(tr("CMake Tool is unconfigured, CMake generator will be ignored."));
    } else {
        const GeneratorInfo info = generatorInfo(k);
        QList<CMakeTool::Generator> known = tool->supportedGenerators();
        auto it = std::find_if(known.constBegin(), known.constEnd(), [info](const CMakeTool::Generator &g) {
            return g.matches(info.generator, info.extraGenerator);
        });
        if (it == known.constEnd()) {
            addWarning(tr("CMake Tool does not support the configured generator."));
        } else {
            if (!it->supportsPlatform && !info.platform.isEmpty())
                addWarning(tr("Platform is not supported by the selected CMake generator."));
            if (!it->supportsToolset && !info.toolset.isEmpty())
                addWarning(tr("Toolset is not supported by the selected CMake generator."));
        }
        if (!tool->hasFileApi()) {
            addWarning(tr("The selected CMake binary does not support file-api. "
                          "%1 will not be able to parse CMake projects.")
                           .arg(Core::Constants::IDE_DISPLAY_NAME));
        }
    }

    return result;
}

void CMakeGeneratorKitAspect::setup(Kit *k)
{
    if (!k || k->hasValue(id()))
        return;
    GeneratorInfo info;
    info.fromVariant(defaultValue(k));
    setGeneratorInfo(k, info);
}

void CMakeGeneratorKitAspect::fix(Kit *k)
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
        const GeneratorInfo dv(info.generator,
                               info.extraGenerator,
                               it->supportsPlatform ? info.platform : QString(),
                               it->supportsToolset ? info.toolset : QString());
        setGeneratorInfo(k, dv);
    }
}

void CMakeGeneratorKitAspect::upgrade(Kit *k)
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

KitAspect::ItemList CMakeGeneratorKitAspect::toUserOutput(const Kit *k) const
{
    const GeneratorInfo info = generatorInfo(k);
    QString message;
    if (info.generator.isEmpty()) {
        message = tr("<Use Default Generator>");
    } else {
        message = tr("Generator: %1<br>Extra generator: %2").arg(info.generator).arg(info.extraGenerator);
        if (!info.platform.isEmpty())
            message += "<br/>" + tr("Platform: %1").arg(info.platform);
        if (!info.toolset.isEmpty())
            message += "<br/>" + tr("Toolset: %1").arg(info.toolset);
    }
    return {{tr("CMake Generator"), message}};
}

KitAspectWidget *CMakeGeneratorKitAspect::createConfigWidget(Kit *k) const
{
    return new CMakeGeneratorKitAspectWidget(k, this);
}

void CMakeGeneratorKitAspect::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    GeneratorInfo info = generatorInfo(k);
    if (info.generator == "NMake Makefiles JOM") {
        if (env.searchInPath("jom.exe").exists())
            return;
        env.appendOrSetPath(QCoreApplication::applicationDirPath());
    }
}

// --------------------------------------------------------------------
// CMakeConfigurationKitAspect:
// --------------------------------------------------------------------

static const char CONFIGURATION_ID[] = "CMake.ConfigurationKitInformation";

static const char CMAKE_C_TOOLCHAIN_KEY[] = "CMAKE_C_COMPILER";
static const char CMAKE_CXX_TOOLCHAIN_KEY[] = "CMAKE_CXX_COMPILER";
static const char CMAKE_QMAKE_KEY[] = "QT_QMAKE_EXECUTABLE";
static const char CMAKE_PREFIX_PATH_KEY[] = "CMAKE_PREFIX_PATH";

class CMakeConfigurationKitAspectWidget : public KitAspectWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeConfigurationKitAspect)
public:
    CMakeConfigurationKitAspectWidget(Kit *kit, const KitAspect *ki)
        : KitAspectWidget(kit, ki),
          m_summaryLabel(new Utils::ElidingLabel),
          m_manageButton(new QPushButton)
    {
        refresh();
        m_manageButton->setText(tr("Change..."));
        connect(m_manageButton, &QAbstractButton::clicked,
                this, &CMakeConfigurationKitAspectWidget::editConfigurationChanges);
    }

private:
    // KitAspectWidget interface
    QWidget *mainWidget() const override { return m_summaryLabel; }
    QWidget *buttonWidget() const override { return m_manageButton; }

    void makeReadOnly() override
    {
        m_manageButton->setEnabled(false);
        if (m_dialog)
            m_dialog->reject();
    }

    void refresh() override
    {
        const QStringList current = CMakeConfigurationKitAspect::toStringList(kit());

        m_summaryLabel->setText(current.join("; "));
        if (m_editor)
            m_editor->setPlainText(current.join('\n'));
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
        m_dialog->setWindowTitle(tr("Edit CMake Configuration"));
        auto layout = new QVBoxLayout(m_dialog);
        m_editor = new QPlainTextEdit;
        m_editor->setToolTip(tr("Enter one variable per line with the variable name "
                                "separated from the variable value by \"=\".<br>"
                                "You may provide a type hint by adding \":TYPE\" before the \"=\"."));
        m_editor->setMinimumSize(800, 200);

        auto chooser = new Core::VariableChooser(m_dialog);
        chooser->addSupportedWidget(m_editor);
        chooser->addMacroExpanderProvider([this]() { return kit()->macroExpander(); });

        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply
                                            |QDialogButtonBox::Reset|QDialogButtonBox::Cancel);

        layout->addWidget(m_editor);
        layout->addWidget(buttons);

        connect(buttons, &QDialogButtonBox::accepted, m_dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, m_dialog, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::clicked, m_dialog, [buttons, this](QAbstractButton *button) {
            if (button != buttons->button(QDialogButtonBox::Reset))
                return;
            CMakeConfigurationKitAspect::setConfiguration(kit(),
                                                          CMakeConfigurationKitAspect::defaultConfiguration(kit()));
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
        CMakeConfigurationKitAspect::fromStringList(kit(), m_editor->toPlainText().split(QLatin1Char('\n')));
    }
    void closeChangesDialog()
    {
        m_dialog->deleteLater();
        m_dialog = nullptr;
        m_editor = nullptr;
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
};


CMakeConfigurationKitAspect::CMakeConfigurationKitAspect()
{
    setObjectName(QLatin1String("CMakeConfigurationKitAspect"));
    setId(CONFIGURATION_ID);
    setDisplayName(tr("CMake Configuration"));
    setDescription(tr("Default configuration passed to CMake when setting up a project."));
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
    const QStringList tmp = Utils::transform(config, [](const CMakeConfigItem &i) { return i.toString(); });
    k->setValue(CONFIGURATION_ID, tmp);
}

QStringList CMakeConfigurationKitAspect::toStringList(const Kit *k)
{
    QStringList current
            = Utils::transform(CMakeConfigurationKitAspect::configuration(k),
                               [](const CMakeConfigItem &i) { return i.toString(); });
    current = Utils::filtered(current, [](const QString &s) { return !s.isEmpty(); });
    Utils::sort(current);
    return current;
}

void CMakeConfigurationKitAspect::fromStringList(Kit *k, const QStringList &in)
{
    CMakeConfig result;
    foreach (const QString &s, in) {
        const CMakeConfigItem item = CMakeConfigItem::fromString(s);
        if (!item.key.isEmpty())
            result << item;
    }
    setConfiguration(k, result);
}

QStringList CMakeConfigurationKitAspect::toArgumentsList(const Kit *k)
{
    return Utils::transform(CMakeConfigurationKitAspect::configuration(k),
                            [](const CMakeConfigItem &i) { return i.toArgument(nullptr); });
}

CMakeConfig CMakeConfigurationKitAspect::defaultConfiguration(const Kit *k)
{
    Q_UNUSED(k)
    CMakeConfig config;
    // Qt4:
    config << CMakeConfigItem(CMAKE_QMAKE_KEY, "%{Qt:qmakeExecutable}");
    // Qt5:
    config << CMakeConfigItem(CMAKE_PREFIX_PATH_KEY, "%{Qt:QT_INSTALL_PREFIX}");

    config << CMakeConfigItem(CMAKE_C_TOOLCHAIN_KEY, "%{Compiler:Executable:C}");
    config << CMakeConfigItem(CMAKE_CXX_TOOLCHAIN_KEY, "%{Compiler:Executable:Cxx}");

    return config;
}

QVariant CMakeConfigurationKitAspect::defaultValue(const Kit *k) const
{
    Q_UNUSED(k)

    // FIXME: Convert preload scripts
    CMakeConfig config = defaultConfiguration(k);
    const QStringList tmp
            = Utils::transform(config, [](const CMakeConfigItem &i) { return i.toString(); });
    return tmp;
}

Tasks CMakeConfigurationKitAspect::validate(const Kit *k) const
{
    QTC_ASSERT(k, return Tasks());

    const QtSupport::BaseQtVersion *const version = QtSupport::QtKitAspect::qtVersion(k);
    const ToolChain *const tcC = ToolChainKitAspect::cToolChain(k);
    const ToolChain *const tcCxx = ToolChainKitAspect::cxxToolChain(k);
    const CMakeConfig config = configuration(k);

    const bool isQt4 = version && version->qtVersion() < QtSupport::QtVersionNumber(5, 0, 0);
    Utils::FilePath qmakePath;
    QStringList qtInstallDirs;
    Utils::FilePath tcCPath;
    Utils::FilePath tcCxxPath;
    foreach (const CMakeConfigItem &i, config) {
        // Do not use expand(QByteArray) as we cannot be sure the input is latin1
        const Utils::FilePath expandedValue
            = Utils::FilePath::fromString(k->macroExpander()->expand(QString::fromUtf8(i.value)));
        if (i.key == CMAKE_QMAKE_KEY)
            qmakePath = expandedValue;
        else if (i.key == CMAKE_C_TOOLCHAIN_KEY)
            tcCPath = expandedValue;
        else if (i.key == CMAKE_CXX_TOOLCHAIN_KEY)
            tcCxxPath = expandedValue;
        else if (i.key == CMAKE_PREFIX_PATH_KEY)
            qtInstallDirs = CMakeConfigItem::cmakeSplitValue(expandedValue.toString());
    }

    Tasks result;
    const auto addWarning = [&result](const QString &desc) {
        result << BuildSystemTask(Task::Warning, desc);
    };

    // Validate Qt:
    if (qmakePath.isEmpty()) {
        if (version && version->isValid() && isQt4) {
            addWarning(tr("CMake configuration has no path to qmake binary set, "
                          "even though the kit has a valid Qt version."));
        }
    } else {
        if (!version || !version->isValid()) {
            addWarning(tr("CMake configuration has a path to a qmake binary set, "
                          "even though the kit has no valid Qt version."));
        } else if (qmakePath != version->qmakeCommand() && isQt4) {
            addWarning(tr("CMake configuration has a path to a qmake binary set "
                          "that does not match the qmake binary path "
                          "configured in the Qt version."));
        }
    }
    if (version && !qtInstallDirs.contains(version->prefix().toString()) && !isQt4) {
        if (version->isValid()) {
            addWarning(tr("CMake configuration has no CMAKE_PREFIX_PATH set "
                          "that points to the kit Qt version."));
        }
    }

    // Validate Toolchains:
    if (tcCPath.isEmpty()) {
        if (tcC && tcC->isValid()) {
            addWarning(tr("CMake configuration has no path to a C compiler set, "
                           "even though the kit has a valid tool chain."));
        }
    } else {
        if (!tcC || !tcC->isValid()) {
            addWarning(tr("CMake configuration has a path to a C compiler set, "
                          "even though the kit has no valid tool chain."));
        } else if (tcCPath != tcC->compilerCommand()) {
            addWarning(tr("CMake configuration has a path to a C compiler set "
                          "that does not match the compiler path "
                          "configured in the tool chain of the kit."));
        }
    }

    if (tcCxxPath.isEmpty()) {
        if (tcCxx && tcCxx->isValid()) {
            addWarning(tr("CMake configuration has no path to a C++ compiler set, "
                          "even though the kit has a valid tool chain."));
        }
    } else {
        if (!tcCxx || !tcCxx->isValid()) {
            addWarning(tr("CMake configuration has a path to a C++ compiler set, "
                          "even though the kit has no valid tool chain."));
        } else if (tcCxxPath != tcCxx->compilerCommand()) {
            addWarning(tr("CMake configuration has a path to a C++ compiler set "
                          "that does not match the compiler path "
                          "configured in the tool chain of the kit."));
        }
    }

    return result;
}

void CMakeConfigurationKitAspect::setup(Kit *k)
{
    if (k && !k->hasValue(CONFIGURATION_ID))
        k->setValue(CONFIGURATION_ID, defaultValue(k));
}

void CMakeConfigurationKitAspect::fix(Kit *k)
{
    Q_UNUSED(k)
}

KitAspect::ItemList CMakeConfigurationKitAspect::toUserOutput(const Kit *k) const
{
    return {{tr("CMake Configuration"), toStringList(k).join("<br>")}};
}

KitAspectWidget *CMakeConfigurationKitAspect::createConfigWidget(Kit *k) const
{
    if (!k)
        return nullptr;
    return new CMakeConfigurationKitAspectWidget(k, this);
}

} // namespace CMakeProjectManager
