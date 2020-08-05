/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "commandbuilderaspect.h"

#include "cmakecommandbuilder.h"
#include "incredibuildconstants.h"
#include "makecommandbuilder.h"

#include <projectexplorer/abstractprocessstep.h>

#include <utils/environment.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

using namespace ProjectExplorer;
using namespace Utils;

namespace IncrediBuild {
namespace Internal {

class CommandBuilderAspectPrivate
{
public:
    CommandBuilderAspectPrivate(BuildStep *step)
        : m_buildStep{step},
          m_customCommandBuilder{step},
          m_makeCommandBuilder{step},
          m_cmakeCommandBuilder{step}
     {}

    void commandBuilderChanged();
    const QStringList &supportedCommandBuilders();
    void setCommandBuilder(const QString &commandBuilder);
    void tryToMigrate();

    BuildStep *m_buildStep;
    CommandBuilder m_customCommandBuilder;
    MakeCommandBuilder m_makeCommandBuilder;
    CMakeCommandBuilder m_cmakeCommandBuilder;

    CommandBuilder *m_commandBuilders[3] {
        &m_customCommandBuilder,
        &m_makeCommandBuilder,
        &m_cmakeCommandBuilder
    };

    CommandBuilder *m_activeCommandBuilder = m_commandBuilders[0];

    bool m_loadedFromMap = false;

    QPointer<QLabel> label;
    QPointer<QLineEdit> makeArgumentsLineEdit;
    QPointer<QComboBox> commandBuilder;
    QPointer<PathChooser> makePathChooser;
};

CommandBuilderAspect::CommandBuilderAspect(BuildStep *step)
    : d(new CommandBuilderAspectPrivate(step))
{
}

CommandBuilderAspect::~CommandBuilderAspect()
{
    delete d;
}

CommandBuilder *CommandBuilderAspect::commandBuilder() const
{
    return d->m_activeCommandBuilder;
}

const QStringList& CommandBuilderAspectPrivate::supportedCommandBuilders()
{
    static QStringList list;
    if (list.empty()) {
        for (CommandBuilder *p : m_commandBuilders)
            list.push_back(p->displayName());
    }

    return list;
}

void CommandBuilderAspectPrivate::setCommandBuilder(const QString &commandBuilder)
{
    for (CommandBuilder *p : m_commandBuilders) {
        if (p->displayName().compare(commandBuilder) == 0) {
            m_activeCommandBuilder = p;
            break;
        }
    }
}

void CommandBuilderAspectPrivate::commandBuilderChanged()
{
    setCommandBuilder(commandBuilder->currentText());

    QString defaultArgs;
    for (const QString &a : m_activeCommandBuilder->defaultArguments())
        defaultArgs += "\"" + a + "\" ";

    QString args;
    for (const QString &a : m_activeCommandBuilder->arguments())
        args += "\"" + a + "\" ";

    if (args == defaultArgs)
        makeArgumentsLineEdit->clear();
    makeArgumentsLineEdit->setText(args);

    const QString defaultCommand = m_activeCommandBuilder->defaultCommand();
    makePathChooser->lineEdit()->setPlaceholderText(defaultCommand);
    QString command = m_activeCommandBuilder->command();
    if (command == defaultCommand)
        command.clear();
    makePathChooser->setPath(command);
}

void CommandBuilderAspectPrivate::tryToMigrate()
{
    // This constructor is called when creating a fresh build step.
    // Attempt to detect build system from pre-existing steps.
    for (CommandBuilder *p : m_commandBuilders) {
        if (p->canMigrate(m_buildStep->stepList())) {
            m_activeCommandBuilder = p;
            break;
        }
    }
}

void CommandBuilderAspect::addToLayout(LayoutBuilder &builder)
{
    if (!d->commandBuilder) {
        d->commandBuilder = new QComboBox;
        d->commandBuilder->addItems(d->supportedCommandBuilders());
        d->commandBuilder->setCurrentText(d->m_activeCommandBuilder->displayName());
        connect(d->commandBuilder, &QComboBox::currentTextChanged, this,
            [this] { d->commandBuilderChanged(); });
    }

    if (!d->makePathChooser) {
        d->makePathChooser = new PathChooser;
        d->makeArgumentsLineEdit = new QLineEdit;
        const QString defaultCommand = d->m_activeCommandBuilder->defaultCommand();
        d->makePathChooser->lineEdit()->setPlaceholderText(defaultCommand);
        const QString command = d->m_activeCommandBuilder->command();
        if (command != defaultCommand)
            d->makePathChooser->setPath(command);

        d->makePathChooser->setExpectedKind(PathChooser::Kind::ExistingCommand);
        d->makePathChooser->setBaseDirectory(FilePath::fromString(PathChooser::homePath()));
        d->makePathChooser->setHistoryCompleter("IncrediBuild.BuildConsole.MakeCommand.History");
        connect(d->makePathChooser, &PathChooser::rawPathChanged, this, [this] {
            d->m_activeCommandBuilder->command(d->makePathChooser->rawPath());
        });


        QString defaultArgs;
        for (const QString &a : d->m_activeCommandBuilder->defaultArguments())
            defaultArgs += "\"" + a + "\" ";

        QString args;
        for (const QString &a : d->m_activeCommandBuilder->arguments())
            args += "\"" + a + "\" ";

        d->makeArgumentsLineEdit->setPlaceholderText(defaultArgs);
        if (args != defaultArgs)
            d->makeArgumentsLineEdit->setText(args);

        connect(d->makeArgumentsLineEdit, &QLineEdit::textEdited, this, [this] {
            d->m_activeCommandBuilder->arguments(d->makeArgumentsLineEdit->text());
        });

    }

    if (!d->label) {
        d->label = new QLabel(tr("Command Helper:"));
        d->label->setToolTip(tr("Select an helper to establish the build command."));
    }

    // On first creation of the step, attempt to detect and migrate from preceding steps
    if (!d->m_loadedFromMap)
        d->tryToMigrate();

    builder.startNewRow().addItems(d->label.data(), d->commandBuilder.data());
    builder.startNewRow().addItems(tr("Make command:"), d->makePathChooser.data());
    builder.startNewRow().addItems(tr("Make arguments:"), d->makeArgumentsLineEdit.data());
}

void CommandBuilderAspect::fromMap(const QVariantMap &map)
{
    d->m_loadedFromMap = true;

    // Command builder. Default to the first in list, which should be the "Custom Command"
    d->setCommandBuilder(map.value(settingsKey(), d->m_commandBuilders[0]->displayName()).toString());
    d->m_activeCommandBuilder->fromMap(map);
}

void CommandBuilderAspect::toMap(QVariantMap &map) const
{
    map[IncrediBuild::Constants::INCREDIBUILD_BUILDSTEP_TYPE]
            = QVariant(IncrediBuild::Constants::BUILDCONSOLE_BUILDSTEP_ID);
    map[settingsKey()] = QVariant(d->m_activeCommandBuilder->displayName());

    d->m_activeCommandBuilder->toMap(&map);
}

} // namespace Internal
} // namespace IncrediBuild
