// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimtoolchainfactory.h"

#include "nimconstants.h"
#include "nimtoolchain.h"
#include "nimtr.h"

#include <projectexplorer/devicesupport/devicemanager.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimToolChainFactory::NimToolChainFactory()
{
    setDisplayName(Tr::tr("Nim"));
    setSupportedToolChainType(Constants::C_NIMTOOLCHAIN_TYPEID);
    setSupportedLanguages({Constants::C_NIMLANGUAGE_ID});
    setToolchainConstructor([] { return new NimToolChain; });
    setUserCreatable(true);
}

Toolchains NimToolChainFactory::autoDetect(const ToolchainDetector &detector) const
{
    Toolchains result;

    IDevice::ConstPtr dev =
        detector.device ? detector.device : DeviceManager::defaultDesktopDevice();

    const FilePath compilerPath = dev->searchExecutableInPath("nim");
    if (compilerPath.isEmpty())
        return result;

    result = Utils::filtered(detector.alreadyKnown, [compilerPath](ToolChain *tc) {
        return tc->typeId() == Constants::C_NIMTOOLCHAIN_TYPEID
                && tc->compilerCommand() == compilerPath;
    });

    if (!result.empty())
        return result;

    auto tc = new NimToolChain;
    tc->setDetection(ToolChain::AutoDetection);
    tc->setCompilerCommand(compilerPath);
    result.append(tc);
    return result;
}

Toolchains NimToolChainFactory::detectForImport(const ToolChainDescription &tcd) const
{
    Toolchains result;
    if (tcd.language == Constants::C_NIMLANGUAGE_ID) {
        auto tc = new NimToolChain;
        tc->setDetection(ToolChain::ManualDetection); // FIXME: sure?
        tc->setCompilerCommand(tcd.compilerPath);
        result.append(tc);
    }
    return result;
}

NimToolChainConfigWidget::NimToolChainConfigWidget(NimToolChain *tc)
    : ToolChainConfigWidget(tc)
    , m_compilerCommand(new PathChooser)
    , m_compilerVersion(new QLineEdit)
{
    // Create ui
    const auto gnuVersionArgs = QStringList("--version");
    m_compilerCommand->setExpectedKind(PathChooser::ExistingCommand);
    m_compilerCommand->setCommandVersionArguments(gnuVersionArgs);
    m_mainLayout->addRow(Tr::tr("&Compiler path:"), m_compilerCommand);
    m_compilerVersion->setReadOnly(true);
    m_mainLayout->addRow(Tr::tr("&Compiler version:"), m_compilerVersion);

    // Fill
    fillUI();

    // Connect
    connect(m_compilerCommand, &PathChooser::textChanged, this, [this] {
        const FilePath path = m_compilerCommand->rawFilePath();
        auto tc = static_cast<NimToolChain *>(toolChain());
        QTC_ASSERT(tc, return);
        tc->setCompilerCommand(path);
        fillUI();
    });
}

void NimToolChainConfigWidget::applyImpl()
{
    auto tc = static_cast<NimToolChain *>(toolChain());
    Q_ASSERT(tc);
    if (tc->isAutoDetected())
        return;
    tc->setCompilerCommand(m_compilerCommand->filePath());
}

void NimToolChainConfigWidget::discardImpl()
{
    fillUI();
}

bool NimToolChainConfigWidget::isDirtyImpl() const
{
    auto tc = static_cast<NimToolChain *>(toolChain());
    Q_ASSERT(tc);
    return tc->compilerCommand() != m_compilerCommand->filePath();
}

void NimToolChainConfigWidget::makeReadOnlyImpl()
{
    m_compilerCommand->setReadOnly(true);
}

void NimToolChainConfigWidget::fillUI()
{
    auto tc = static_cast<NimToolChain *>(toolChain());
    Q_ASSERT(tc);
    m_compilerCommand->setFilePath(tc->compilerCommand());
    m_compilerVersion->setText(tc->compilerVersion());
}

}
