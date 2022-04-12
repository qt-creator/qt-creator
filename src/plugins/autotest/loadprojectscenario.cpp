/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "loadprojectscenario.h"

#include <cppeditor/cpptoolstestcase.h>
#include <cppeditor/projectinfo.h>

#include <qtsupport/qtkitinformation.h>

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
    const QString projectFilePath = m_tmpDir->path() + "/plain/plain.pro";

    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    // This code must trigger a call to PluginManager::finishScenario() at some later point.
    const CppEditor::ProjectInfo::ConstPtr projectInfo = projectManager.open(projectFilePath,
                                                                            true, m_kit);
    return projectInfo.get();
}

} // namespace Internal
} // namespace Autotest
