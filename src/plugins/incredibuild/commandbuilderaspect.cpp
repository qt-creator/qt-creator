// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandbuilderaspect.h"

#include "cmakecommandbuilder.h"
#include "incredibuildconstants.h"
#include "incredibuildtr.h"
#include "makecommandbuilder.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>

#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>

using namespace ProjectExplorer;
using namespace Utils;

namespace IncrediBuild::Internal {

class CommandBuilderAspectPrivate
{
public:
    CommandBuilderAspectPrivate(BuildStep *step)
        : m_buildStep{step},
          m_customCommandBuilder{step},
          m_makeCommandBuilder{step},
          m_cmakeCommandBuilder{step}
     {}

    void tryToMigrate();
    void setActiveCommandBuilder(const QString &commandBuilderId);

    BuildStep *m_buildStep;
    CommandBuilder m_customCommandBuilder;
    MakeCommandBuilder m_makeCommandBuilder;
    CMakeCommandBuilder m_cmakeCommandBuilder;

    CommandBuilder *m_commandBuilders[3] {
        &m_customCommandBuilder,
        &m_makeCommandBuilder,
        &m_cmakeCommandBuilder
    };

    // Default to "Custom Command", but try to upgrade in tryToMigrate() later.
    CommandBuilder *m_activeCommandBuilder = m_commandBuilders[0];

    bool m_loadedFromMap = false;

    QPointer<QLabel> label;
    QPointer<QComboBox> commandBuilder;
    QPointer<PathChooser> makePathChooser;
    QPointer<QLineEdit> makeArgumentsLineEdit;
};

CommandBuilderAspect::CommandBuilderAspect(BuildStep *step)
    : BaseAspect(step)
    , d(new CommandBuilderAspectPrivate(step))
{
}

CommandBuilderAspect::~CommandBuilderAspect()
{
    delete d;
}

QString CommandBuilderAspect::fullCommandFlag(bool keepJobNum) const
{
    QString argsLine = d->m_activeCommandBuilder->arguments();

    if (!keepJobNum)
        argsLine = d->m_activeCommandBuilder->setMultiProcessArg(argsLine);

    QString fullCommand("\"%1\" %2");
    fullCommand = fullCommand.arg(d->m_activeCommandBuilder->effectiveCommand().toUserOutput(), argsLine);

    return fullCommand;
}

void CommandBuilderAspectPrivate::setActiveCommandBuilder(const QString &commandBuilderId)
{
    for (CommandBuilder *p : m_commandBuilders) {
        if (p->id() == commandBuilderId) {
            m_activeCommandBuilder = p;
            break;
        }
    }
}

void CommandBuilderAspectPrivate::tryToMigrate()
{
    // This function is called when creating a fresh build step.
    // Attempt to detect build system from pre-existing steps.
    for (CommandBuilder *p : m_commandBuilders) {
        const QList<Utils::Id> migratableSteps = p->migratableSteps();
        for (Utils::Id stepId : migratableSteps) {
            if (BuildStep *bs = m_buildStep->stepList()->firstStepWithId(stepId)) {
                m_activeCommandBuilder = p;
                bs->setEnabled(false);
                m_buildStep->project()->saveSettings();
                return;
            }
        }
    }
}

void CommandBuilderAspect::addToLayout(Layouting::LayoutItem &parent)
{
    if (!d->commandBuilder) {
        d->commandBuilder = new QComboBox;
        for (CommandBuilder *p : d->m_commandBuilders)
            d->commandBuilder->addItem(p->displayName());
        connect(d->commandBuilder, &QComboBox::currentIndexChanged, this, [this](int idx) {
            if (idx >= 0 && idx < int(sizeof(d->m_commandBuilders) / sizeof(d->m_commandBuilders[0])))
                d->m_activeCommandBuilder = d->m_commandBuilders[idx];
            updateGui();
        });
    }

    if (!d->makePathChooser) {
        d->makePathChooser = new PathChooser;
        d->makePathChooser->setExpectedKind(PathChooser::Kind::ExistingCommand);
        d->makePathChooser->setBaseDirectory(PathChooser::homePath());
        d->makePathChooser->setHistoryCompleter("IncrediBuild.BuildConsole.MakeCommand.History");
        connect(d->makePathChooser, &PathChooser::rawPathChanged, this, [this] {
            d->m_activeCommandBuilder->setCommand(d->makePathChooser->rawFilePath());
            updateGui();
        });
    }

   if (!d->makeArgumentsLineEdit) {
        d->makeArgumentsLineEdit = new QLineEdit;
        connect(d->makeArgumentsLineEdit, &QLineEdit::textEdited, this, [this](const QString &arg) {
            d->m_activeCommandBuilder->setArguments(arg);
            updateGui();
        });
    }

    if (!d->label) {
        d->label = new QLabel(Tr::tr("Command Helper:"));
        d->label->setToolTip(Tr::tr("Select a helper to establish the build command."));
    }

    // On first creation of the step, attempt to detect and migrate from preceding steps
    if (!d->m_loadedFromMap)
        d->tryToMigrate();

    parent.addRow({d->label.data(), d->commandBuilder.data()});
    parent.addRow({Tr::tr("Make command:"), d->makePathChooser.data()});
    parent.addRow({Tr::tr("Make arguments:"), d->makeArgumentsLineEdit.data()});

    updateGui();
}

void CommandBuilderAspect::fromMap(const Store &map)
{
    d->m_loadedFromMap = true;

    d->setActiveCommandBuilder(map.value(settingsKey()).toString());
    d->m_customCommandBuilder.fromMap(map);
    d->m_makeCommandBuilder.fromMap(map);
    d->m_cmakeCommandBuilder.fromMap(map);

    updateGui();
}

void CommandBuilderAspect::toMap(Store &map) const
{
    map[IncrediBuild::Constants::INCREDIBUILD_BUILDSTEP_TYPE]
            = QVariant(IncrediBuild::Constants::BUILDCONSOLE_BUILDSTEP_ID);
    map[settingsKey()] = QVariant(d->m_activeCommandBuilder->id());

    d->m_customCommandBuilder.toMap(&map);
    d->m_makeCommandBuilder.toMap(&map);
    d->m_cmakeCommandBuilder.toMap(&map);
}

void CommandBuilderAspect::updateGui()
{
    if (!d->commandBuilder)
        return;

    d->commandBuilder->setCurrentText(d->m_activeCommandBuilder->displayName());

    const FilePath defaultCommand = d->m_activeCommandBuilder->defaultCommand();
    d->makePathChooser->setFilePath(d->m_activeCommandBuilder->command());
    d->makePathChooser->setDefaultValue(defaultCommand.toUserOutput());

    const QString defaultArgs = d->m_activeCommandBuilder->defaultArguments();
    d->makeArgumentsLineEdit->setPlaceholderText(defaultArgs);
    d->makeArgumentsLineEdit->setText(d->m_activeCommandBuilder->arguments());
}

} // IncrediBuild::Internal
