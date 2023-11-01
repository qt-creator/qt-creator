// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "loadprojectscenario.h"

#include <cppeditor/cpptoolstestcase.h>
#include <cppeditor/projectinfo.h>

#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>

#include <QDebug>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Autotest {
namespace Internal {

LoadProjectScenario::LoadProjectScenario(TestTreeModel *model, QObject *parent)
    : QObject(parent),
      m_model(model)
{
}

LoadProjectScenario::~LoadProjectScenario()
{
    delete m_tmpDir;
}

bool LoadProjectScenario::operator()()
{
    return init() && loadProject();
}

bool LoadProjectScenario::init()
{
    const QList<Kit *> allKits = KitManager::kits();
    if (allKits.count() == 0) {
        qWarning() << "This scenario requires at least one kit to be present";
        return false;
    }

    m_kit = findOr(allKits, nullptr, [](Kit *k) {
            return k->isValid() && QtSupport::QtKitAspect::qtVersion(k) != nullptr;
    });
    if (!m_kit) {
        qWarning() << "The scenario requires at least one valid kit with a valid Qt";
        return false;
    }

    if (!QtSupport::QtKitAspect::qtVersion(m_kit)) {
        qWarning() << "Could not figure out which Qt version is used for default kit.";
        return false;
    }

    const ToolChain * const toolchain = ToolChainKitAspect::cxxToolChain(m_kit);
    if (!toolchain) {
        qWarning() << "This test requires that there is a kit with a toolchain.";
        return false;
    }

    m_tmpDir = new CppEditor::Tests::TemporaryCopiedDir(":/unit_test");
    return true;
}

bool LoadProjectScenario::loadProject()
{
    const FilePath projectFilePath = m_tmpDir->filePath() / "/plain/plain.pro";

    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    // This code must trigger a call to PluginManager::finishScenario() at some later point.
    const CppEditor::ProjectInfo::ConstPtr projectInfo = projectManager.open(projectFilePath,
                                                                            true, m_kit);
    return projectInfo.get();
}

} // namespace Internal
} // namespace Autotest
