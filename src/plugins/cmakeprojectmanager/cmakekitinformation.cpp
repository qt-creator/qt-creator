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
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>

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
    if (it == known.constEnd())
        it = std::find_if(known.constBegin(), known.constEnd(),
                          [](const QString &s) { return s == QLatin1String("CodeBlocks - Unix Makefiles"); });
    if (it == known.constEnd())
        it = std::find_if(known.constBegin(), known.constEnd(),
                          [](const QString &s) { return s == QLatin1String("CodeBlocks - NMake Makefiles"); });
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

} // namespace CMakeProjectManager
