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
#include "cmakekitconfigwidget.h"
#include "cmaketoolmanager.h"
#include "cmaketool.h"

#include <projectexplorer/task.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
// --------------------------------------------------------------------
// CMakeKitInformation:
// --------------------------------------------------------------------

static Core::Id defaultCMakeToolId()
{
    CMakeTool *defaultTool = CMakeToolManager::defaultCMakeTool();
    return defaultTool ? defaultTool->id() : Core::Id();
}

static const char TOOL_ID[] = "CMakeProjectManager.CMakeKitInformation";

// --------------------------------------------------------------------
// CMakeKitInformation:
// --------------------------------------------------------------------

CMakeKitInformation::CMakeKitInformation()
{
    setObjectName(QLatin1String("CMakeKitInformation"));
    setId(TOOL_ID);
    setPriority(20000);

    //make sure the default value is set if a selected CMake is removed
    connect(CMakeToolManager::instance(), &CMakeToolManager::cmakeRemoved,
            [this]() { foreach (Kit *k, KitManager::kits()) fix(k); });

    //make sure the default value is set if a new default CMake is set
    connect(CMakeToolManager::instance(), &CMakeToolManager::defaultCMakeChanged,
            [this]() { foreach (Kit *k, KitManager::kits()) fix(k); });
}

CMakeTool *CMakeKitInformation::cmakeTool(const Kit *k)
{
    if (!k)
        return 0;

    const QVariant id = k->value(TOOL_ID);
    return CMakeToolManager::findById(Core::Id::fromSetting(id));
}

void CMakeKitInformation::setCMakeTool(Kit *k, const Core::Id id)
{
    const Core::Id toSet = id.isValid() ? id : defaultCMakeToolId();
    QTC_ASSERT(!id.isValid() || CMakeToolManager::findById(toSet), return);
    if (k)
        k->setValue(TOOL_ID, toSet.toSetting());
}

QVariant CMakeKitInformation::defaultValue(const Kit *k) const
{
    const Core::Id id = k ? defaultCMakeToolId() : Core::Id();
    return id.toSetting();
}

QList<Task> CMakeKitInformation::validate(const Kit *k) const
{
    Q_UNUSED(k);
    return QList<Task>();
}

void CMakeKitInformation::setup(Kit *k)
{
    CMakeTool *tool = CMakeKitInformation::cmakeTool(k);
    if (!tool)
        setCMakeTool(k, defaultCMakeToolId());
}

void CMakeKitInformation::fix(Kit *k)
{
    if (!CMakeKitInformation::cmakeTool(k))
        setup(k);
}

KitInformation::ItemList CMakeKitInformation::toUserOutput(const Kit *k) const
{
    const CMakeTool *const tool = cmakeTool(k);
    return ItemList() << qMakePair(tr("CMake"), tool == 0 ? tr("Unconfigured") : tool->displayName());
}

KitConfigWidget *CMakeKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::CMakeKitConfigWidget(k, this);
}

void CMakeKitInformation::addToMacroExpander(Kit *k, Utils::MacroExpander *expander) const
{
    expander->registerFileVariables("CMake:Executable", tr("Path to the cmake executable"),
                                    [this, k]() -> QString {
                                        CMakeTool *tool = CMakeKitInformation::cmakeTool(k);
                                        return tool ? tool->cmakeExecutable().toString() : QString();
                                    });
}

// --------------------------------------------------------------------
// CMakeGeneratorKitInformation:
// --------------------------------------------------------------------

static const char GENERATOR_ID[] = "CMake.GeneratorKitInformation";

CMakeGeneratorKitInformation::CMakeGeneratorKitInformation()
{
    setObjectName(QLatin1String("CMakeGeneratorKitInformation"));
    setId(GENERATOR_ID);
    setPriority(19000);
}

QString CMakeGeneratorKitInformation::generator(const Kit *k)
{
    if (!k)
        return QString();
    return k->value(GENERATOR_ID).toString();
}

void CMakeGeneratorKitInformation::setGenerator(Kit *k, const QString &generator)
{
    if (!k)
        return;
    k->setValue(GENERATOR_ID, generator);
}

QString CMakeGeneratorKitInformation::generatorArgument(const Kit *k)
{
    QString tmp = generator(k);
    if (tmp.isEmpty())
        return QString::fromLatin1("-G") + tmp;
    return tmp;
}

QVariant CMakeGeneratorKitInformation::defaultValue(const Kit *k) const
{
    CMakeTool *tool = CMakeKitInformation::cmakeTool(k);
    if (!tool)
        return QString();
    QStringList known = tool->supportedGenerators();
    auto it = std::find_if(known.constBegin(), known.constEnd(),
                           [](const QString &s) { return s == QLatin1String("CodeBlocks - Ninja"); });
    if (it != known.constEnd()) {
        Utils::Environment env = Utils::Environment::systemEnvironment();
        k->addToEnvironment(env);
        const Utils::FileName ninjaExec = env.searchInPath(QLatin1String("ninja"));
        if (ninjaExec.isEmpty())
            it = known.constEnd(); // Ignore ninja generator without ninja exectuable
    }
    if (Utils::HostOsInfo::isWindowsHost()) {
        // *sigh* Windows with its zoo of incompatible stuff again...
        ToolChain *tc = ToolChainKitInformation::toolChain(k);
        if (tc && tc->typeId() == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID) {
            if (it == known.constEnd())
                it = std::find_if(known.constBegin(), known.constEnd(),
                                  [](const QString &s) { return s == QLatin1String("CodeBlocks - MinGW Makefiles"); });
        } else {
            if (it == known.constEnd())
                it = std::find_if(known.constBegin(), known.constEnd(),
                                  [](const QString &s) { return s == QLatin1String("CodeBlocks - NMake Makefiles"); });
        }

    } else {
        // Unix-oid OSes:
        if (it == known.constEnd())
            it = std::find_if(known.constBegin(), known.constEnd(),
                              [](const QString &s) { return s == QLatin1String("CodeBlocks - Unix Makefiles"); });
    }
    if (it == known.constEnd())
        it = known.constBegin(); // Fallback to the first generator...
    if (it != known.constEnd())
        return *it;
    return QString();
}

QList<Task> CMakeGeneratorKitInformation::validate(const Kit *k) const
{
    CMakeTool *tool = CMakeKitInformation::cmakeTool(k);
    QString generator = CMakeGeneratorKitInformation::generator(k);

    QList<Task> result;
    if (!tool) {
        if (!generator.isEmpty()) {
            result << Task(Task::Warning, tr("No CMake Tool configured, CMake generator will be ignored."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    } else {
        if (!tool->isValid()) {
            result << Task(Task::Warning, tr("CMake Tool is unconfigured, CMake generator will be ignored."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        } else {
            QStringList known = tool->supportedGenerators();
            if (!known.contains(generator)) {
                result << Task(Task::Error, tr("CMake Tool does not support the configured generator."),
                               Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
            }
            if (!generator.startsWith(QLatin1String("CodeBlocks -"))) {
                result << Task(Task::Warning, tr("CMake generator does not generate CodeBlocks file. "
                                                 "Qt Creator will not be able to parse the CMake project."),
                               Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
            }
        }
    }
    return result;
}

void CMakeGeneratorKitInformation::setup(Kit *k)
{
    CMakeGeneratorKitInformation::setGenerator(k, defaultValue(k).toString());
}

void CMakeGeneratorKitInformation::fix(Kit *k)
{
    const CMakeTool *tool = CMakeKitInformation::cmakeTool(k);
    const QString generator = CMakeGeneratorKitInformation::generator(k);

    if (!tool)
        return;
    QStringList known = tool->supportedGenerators();
    if (generator.isEmpty() || !known.contains(generator))
        CMakeGeneratorKitInformation::setGenerator(k, defaultValue(k).toString());
}

KitInformation::ItemList CMakeGeneratorKitInformation::toUserOutput(const Kit *k) const
{
    const QString generator = CMakeGeneratorKitInformation::generator(k);
    return ItemList() << qMakePair(tr("CMake Generator"),
                                   generator.isEmpty() ? tr("<Use Default Generator>") : generator);
}

KitConfigWidget *CMakeGeneratorKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::CMakeGeneratorKitConfigWidget(k, this);
}

// --------------------------------------------------------------------
// CMakeConfigurationKitInformation:
// --------------------------------------------------------------------

static const char CONFIGURATION_ID[] = "CMake.ConfigurationKitInformation";

static const char CMAKE_QMAKE_KEY[] = "QT_QMAKE_EXECUTABLE";
static const char CMAKE_TOOLCHAIN_KEY[] = "CMAKE_CXX_COMPILER";

CMakeConfigurationKitInformation::CMakeConfigurationKitInformation()
{
    setObjectName(QLatin1String("CMakeConfigurationKitInformation"));
    setId(CONFIGURATION_ID);
    setPriority(18000);
}

CMakeConfig CMakeConfigurationKitInformation::configuration(const Kit *k)
{
    if (!k)
        return CMakeConfig();
    const QStringList tmp = k->value(CONFIGURATION_ID).toStringList();
    return Utils::transform(tmp, [](const QString &s) { return CMakeConfigItem::fromString(s); });
}

void CMakeConfigurationKitInformation::setConfiguration(Kit *k, const CMakeConfig &config)
{
    if (!k)
        return;
    const QStringList tmp = Utils::transform(config, [](const CMakeConfigItem &i) { return i.toString(); });
    k->setValue(CONFIGURATION_ID, tmp);
}

QStringList CMakeConfigurationKitInformation::toStringList(const Kit *k)
{
    QStringList current
            = Utils::transform(CMakeConfigurationKitInformation::configuration(k),
                               [](const CMakeConfigItem &i) { return i.toString(); });
    Utils::sort(current);
    return current;
}

void CMakeConfigurationKitInformation::fromStringList(Kit *k, const QStringList &in)
{
    CMakeConfig result;
    foreach (const QString &s, in) {
        const CMakeConfigItem item = CMakeConfigItem::fromString(s);
        if (!item.key.isEmpty())
            result << item;
    }
    setConfiguration(k, result);
}

QVariant CMakeConfigurationKitInformation::defaultValue(const Kit *k) const
{
    Q_UNUSED(k);

    // FIXME: Convert preload scripts
    CMakeConfig config;
    config << CMakeConfigItem(CMAKE_QMAKE_KEY, "%{Qt:qmakeExecutable}");
    config << CMakeConfigItem(CMAKE_TOOLCHAIN_KEY, "%{Compiler:Executable}");

    const QStringList tmp
            = Utils::transform(config, [](const CMakeConfigItem &i) { return i.toString(); });
    return tmp;
}

QList<Task> CMakeConfigurationKitInformation::validate(const Kit *k) const
{
    const QtSupport::BaseQtVersion *const version = QtSupport::QtKitInformation::qtVersion(k);
    const ToolChain *const tc = ToolChainKitInformation::toolChain(k);
    const CMakeConfig config = configuration(k);

    QByteArray qmakePath;
    QByteArray tcPath;
    foreach (const CMakeConfigItem &i, config) {
        // Do not use expand(QByteArray) as we can not be sure the input is latin1
        const QByteArray expandedValue = k->macroExpander()->expand(QString::fromUtf8(i.value)).toUtf8();
        if (i.key == CMAKE_QMAKE_KEY)
            qmakePath = expandedValue;
        else if (i.key == CMAKE_TOOLCHAIN_KEY)
            tcPath = expandedValue;
    }

    QList<Task> result;
    // Validate Qt:
    if (qmakePath.isEmpty()) {
        if (version && version->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has no path to qmake binary set, "
                                             "even though the kit has a valid Qt version."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    } else {
        if (!version || !version->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a qmake binary set, "
                                             "even though the kit has no valid Qt version."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        } else if (qmakePath != version->qmakeCommand().toString().toUtf8()) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a qmake binary set, "
                                             "which does not match up with the qmake binary path "
                                             "configured in the Qt version."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }

    // Validate Toolchain:
    if (tcPath.isEmpty()) {
        if (tc && tc->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has no path to a C++ compiler set, "
                                             "even though the kit has a valid tool chain."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    } else {
        if (!tc || !tc->isValid()) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a C++ compiler set, "
                                             "even though the kit has no valid tool chain."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        } else if (tcPath != tc->compilerCommand().toString().toUtf8()) {
            result << Task(Task::Warning, tr("CMake configuration has a path to a C++ compiler set, "
                                             "that does not match up with the compiler path "
                                             "configured in the tool chain of the kit."),
                           Utils::FileName(), -1, Core::Id(Constants::TASK_CATEGORY_BUILDSYSTEM));
        }
    }

    return result;
}

void CMakeConfigurationKitInformation::setup(Kit *k)
{
    if (k)
        k->setValue(CONFIGURATION_ID, defaultValue(k));
}

void CMakeConfigurationKitInformation::fix(Kit *k)
{
    Q_UNUSED(k);
}

KitInformation::ItemList CMakeConfigurationKitInformation::toUserOutput(const Kit *k) const
{
    const QStringList current = toStringList(k);
    return ItemList() << qMakePair(tr("CMake Configuration"), current.join(QLatin1String("<br>")));
}

KitConfigWidget *CMakeConfigurationKitInformation::createConfigWidget(Kit *k) const
{
    if (!k)
        return 0;
    return new Internal::CMakeConfigurationKitConfigWidget(k, this);
}

} // namespace CMakeProjectManager
