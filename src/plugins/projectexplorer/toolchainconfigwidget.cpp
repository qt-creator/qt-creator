// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchainconfigwidget.h"

#include "toolchain.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "toolchainmanager.h"

#include <utils/detailswidget.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QPainter>

using namespace Utils;

namespace ProjectExplorer {

ToolchainConfigWidget::ToolchainConfigWidget(const ToolchainBundle &bundle)
    : m_bundle(bundle)
{
    auto centralWidget = new Utils::DetailsWidget;
    centralWidget->setState(Utils::DetailsWidget::NoSummary);

    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    setFocusPolicy(Qt::NoFocus);

    setWidget(centralWidget);

    auto detailsBox = new QWidget();

    m_mainLayout = new QFormLayout(detailsBox);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    centralWidget->setWidget(detailsBox);
    m_mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow); // for the Macs...

    m_nameLineEdit = new QLineEdit;
    m_nameLineEdit->setText(bundle.displayName());

    m_mainLayout->addRow(Tr::tr("Name:"), m_nameLineEdit);

    if (bundle.type() != Constants::MSVC_TOOLCHAIN_TYPEID)
        setupCompilerPathChoosers();

    connect(m_nameLineEdit, &QLineEdit::textChanged, this, &ToolchainConfigWidget::dirty);
}

void ToolchainConfigWidget::apply()
{
    m_bundle.setDisplayName(m_nameLineEdit->text());
    if (!bundle().isAutoDetected()) {
        for (const auto &[tc, pathChooser] : std::as_const(m_commands))
            bundle().setCompilerCommand(tc->language(), pathChooser->filePath());
    }
    applyImpl();
}

void ToolchainConfigWidget::discard()
{
    m_nameLineEdit->setText(m_bundle.displayName());
    for (const auto &[tc, pathChooser] : std::as_const(m_commands))
        pathChooser->setFilePath(bundle().compilerCommand(tc->language()));
    discardImpl();
}

bool ToolchainConfigWidget::isDirty() const
{
    for (const auto &[tc, pathChooser] : std::as_const(m_commands)) {
        if (pathChooser->filePath() != bundle().compilerCommand(tc->language()))
            return true;
    }
    return m_nameLineEdit->text() != m_bundle.displayName() || isDirtyImpl();
}

void ToolchainConfigWidget::makeReadOnly()
{
    m_nameLineEdit->setEnabled(false);
    for (const auto &commands : std::as_const(m_commands))
        commands.second->setReadOnly(true);
    makeReadOnlyImpl();
}

void ToolchainConfigWidget::addErrorLabel()
{
    if (!m_errorLabel) {
        m_errorLabel = new QLabel;
        m_errorLabel->setVisible(false);
    }
    m_mainLayout->addRow(m_errorLabel);
}

void ToolchainConfigWidget::setErrorMessage(const QString &m)
{
    QTC_ASSERT(m_errorLabel, return);
    if (m.isEmpty()) {
        clearErrorMessage();
    } else {
        m_errorLabel->setText(m);
        m_errorLabel->setStyleSheet(QLatin1String("background-color: \"red\""));
        m_errorLabel->setVisible(true);
    }
}

void ToolchainConfigWidget::clearErrorMessage()
{
    QTC_ASSERT(m_errorLabel, return);
    m_errorLabel->clear();
    m_errorLabel->setStyleSheet(QString());
    m_errorLabel->setVisible(false);
}

QStringList ToolchainConfigWidget::splitString(const QString &s)
{
    ProcessArgs::SplitError splitError;
    const OsType osType = HostOsInfo::hostOs();
    QStringList res = ProcessArgs::splitArgs(s, osType, false, &splitError);
    if (splitError != ProcessArgs::SplitOk) {
        res = ProcessArgs::splitArgs(s + '\\', osType, false, &splitError);
        if (splitError != ProcessArgs::SplitOk) {
            res = ProcessArgs::splitArgs(s + '"', osType, false, &splitError);
            if (splitError != ProcessArgs::SplitOk)
                res = ProcessArgs::splitArgs(s + '\'', osType, false, &splitError);
        }
    }
    return res;
}

ToolchainConfigWidget::ToolchainChooser ToolchainConfigWidget::compilerPathChooser(Utils::Id language)
{
    for (const ToolchainChooser &chooser : std::as_const(m_commands)) {
        if (chooser.first->language() == language)
            return chooser;
    }
    return {};
}

void ToolchainConfigWidget::setupCompilerPathChoosers()
{
    const QString nameLabelString = int(bundle().toolchains().size()) == 1
                                        ? Tr::tr("&Compiler path")
                                        : QString();
    bundle().forEach<Toolchain>([&](const Toolchain &tc) {
        const QString name = !nameLabelString.isEmpty()
                ? nameLabelString
                : Tr::tr("%1 compiler path").arg(
                                       ToolchainManager::displayNameOfLanguageId(tc.language()));
        const auto commandChooser = new PathChooser(this);
        commandChooser->setExpectedKind(PathChooser::ExistingCommand);
        commandChooser->setHistoryCompleter("PE.ToolChainCommand.History");
        commandChooser->setAllowPathFromDevice(true);
        commandChooser->setFilePath(tc.compilerCommand());
        m_commands << std::make_pair(&tc, commandChooser);
        if (tc.language() == Constants::CXX_LANGUAGE_ID
            && bundle().factory()->supportedLanguages().contains(Constants::C_LANGUAGE_ID)) {
            m_deriveCxxCompilerCheckBox = new QCheckBox(Tr::tr("Derive from C compiler"));
            m_deriveCxxCompilerCheckBox->setChecked(true);
            const auto commandLayout = new QHBoxLayout;
            commandLayout->addWidget(commandChooser);
            commandLayout->addWidget(m_deriveCxxCompilerCheckBox);
            m_mainLayout->addRow(name, commandLayout);
            if (!tc.compilerCommand().isExecutableFile())
                deriveCxxCompilerCommand();
        } else {
            m_mainLayout->addRow(name, commandChooser);
        }
        connect(commandChooser, &PathChooser::rawPathChanged, this, [this, &tc] {
            emit compilerCommandChanged(tc.language());
            if (tc.language() == Constants::C_LANGUAGE_ID)
                deriveCxxCompilerCommand();
        });
        connect(commandChooser, &PathChooser::rawPathChanged, this, &ToolchainConfigWidget::dirty);
    });
}

FilePath ToolchainConfigWidget::compilerCommand(Utils::Id language)
{
    if (const PathChooser * const chooser = compilerPathChooser(language).second)
        return chooser->filePath();
    return {};
}

bool ToolchainConfigWidget::hasAnyCompiler() const
{
    for (const auto &cmd : std::as_const(m_commands)) {
        if (cmd.second->filePath().isExecutableFile())
            return true;
    }
    return false;
}

void ToolchainConfigWidget::setCommandVersionArguments(const QStringList &args)
{
    for (const auto &[_,pathChooser] : std::as_const(m_commands))
        pathChooser->setCommandVersionArguments(args);
}

void ToolchainConfigWidget::deriveCxxCompilerCommand()
{
    if (!m_deriveCxxCompilerCheckBox || !m_deriveCxxCompilerCheckBox->isChecked())
        return;

    using namespace Constants;
    const ToolchainChooser cChooser = compilerPathChooser(C_LANGUAGE_ID);
    const ToolchainChooser cxxChooser = compilerPathChooser(CXX_LANGUAGE_ID);
    QTC_ASSERT(cChooser.first && cChooser.second && cxxChooser.second, return);
    if (cChooser.second->filePath().isExecutableFile()) {
        if (const FilePath cxxCmd = bundle().factory()->correspondingCompilerCommand(
                cChooser.second->filePath(), CXX_LANGUAGE_ID);
            cxxCmd.isExecutableFile())
            cxxChooser.second->setFilePath(cxxCmd);
    }
}

} // namespace ProjectExplorer
