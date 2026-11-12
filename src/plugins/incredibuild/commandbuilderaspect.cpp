// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandbuilderaspect.h"

#include "cmakecommandbuilder.h"
#include "incredibuildconstants.h"
#include "incredibuildtr.h"
#include "makecommandbuilder.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>

#include <utils/algorithm.h>
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>

#ifdef WITH_TESTS
#include <QTest>
#endif

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
                bs->setStepEnabled(false);
                m_buildStep->project()->saveSettings();
                return;
            }
        }
    }
}

void CommandBuilderAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto commandBuilder = new QComboBox;
    for (CommandBuilder *p : d->m_commandBuilders)
        commandBuilder->addItem(p->displayName());
    connect(commandBuilder, &QComboBox::currentIndexChanged, this, [this](int idx) {
        if (idx >= 0 && idx < int(sizeof(d->m_commandBuilders) / sizeof(d->m_commandBuilders[0])))
            d->m_activeCommandBuilder = d->m_commandBuilders[idx];
        updateGui();
    });
    setWheelScrollingWithoutFocusBlocked(commandBuilder);

    auto makePathChooser = new PathChooser;
    makePathChooser->setExpectedKind(PathChooserKind::ExistingCommand);
    makePathChooser->setBaseDirectory(PathChooser::homePath());
    makePathChooser->setHistoryCompleter("IncrediBuild.BuildConsole.MakeCommand.History");
    connect(makePathChooser, &PathChooser::rawPathChanged, this, [this, makePathChooser] {
        d->m_activeCommandBuilder->setCommand(makePathChooser->unexpandedFilePath());
        updateGui();
    });

    auto makeArgumentsLineEdit = new QLineEdit;
    connect(makeArgumentsLineEdit, &QLineEdit::textEdited, this, [this](const QString &arg) {
        d->m_activeCommandBuilder->setArguments(arg);
        updateGui();
    });

    auto label = new QLabel(Tr::tr("Command Helper:"));
    label->setToolTip(Tr::tr("Select a helper to establish the build command."));

    const auto showActiveCommandBuilder = [this, commandBuilder, makePathChooser,
                                           makeArgumentsLineEdit] {
        CommandBuilder *active = d->m_activeCommandBuilder;
        {
            const QSignalBlocker blocker(commandBuilder);
            commandBuilder->setCurrentText(active->displayName());
        }
        {
            const QSignalBlocker blocker(makePathChooser);
            if (makePathChooser->unexpandedFilePath() != active->command())
                makePathChooser->setFilePath(active->command());
            makePathChooser->setDefaultValue(active->defaultCommand());
        }
        makeArgumentsLineEdit->setPlaceholderText(active->defaultArguments());
        if (makeArgumentsLineEdit->text() != active->arguments())
            makeArgumentsLineEdit->setText(active->arguments());
    };
    connect(this, &CommandBuilderAspect::guiUpdateRequested, commandBuilder,
            showActiveCommandBuilder);

    // On first creation of the step, attempt to detect and migrate from preceding steps
    if (!d->m_loadedFromMap)
        d->tryToMigrate();

    parent.addRow({label, commandBuilder});
    parent.addRow({Tr::tr("Make command:"), makePathChooser});
    parent.addRow({Tr::tr("Make arguments:"), makeArgumentsLineEdit});

    // The migration above may have changed the helper for the other widgets, too.
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
    emit guiUpdateRequested(QPrivateSignal());
}

#ifdef WITH_TESTS
class CommandBuilderAspectTest final : public QObject
{
    Q_OBJECT

private slots:
    void testUpdatesEveryWidget()
    {
        // Only the make and CMake helpers need a build step, the custom command does not.
        CommandBuilderAspect commandBuilder(nullptr);
        BaseAspect &aspect = commandBuilder;
        aspect.fromMap({}); // Loaded, so nothing is migrated from other build steps.
        const std::unique_ptr<QWidget> first(Layouting::Form{aspect}.emerge());
        const std::unique_ptr<QWidget> second(Layouting::Form{aspect}.emerge());
        const auto argumentsEdit = [](const std::unique_ptr<QWidget> &widget) {
            return Utils::findOrDefault(widget->findChildren<QLineEdit *>(), [](QLineEdit *edit) {
                return edit->metaObject() == &QLineEdit::staticMetaObject;
            });
        };
        QLineEdit *firstEdit = argumentsEdit(first);
        QLineEdit *secondEdit = argumentsEdit(second);
        QVERIFY(firstEdit);
        QVERIFY(secondEdit);

        firstEdit->setText("--typed");
        emit firstEdit->textEdited("--typed");

        QCOMPARE(secondEdit->text(), "--typed");
        QVERIFY(commandBuilder.fullCommandFlag(true).endsWith("--typed"));
    }
};

QObject *createCommandBuilderAspectTest()
{
    return new CommandBuilderAspectTest;
}
#endif // WITH_TESTS

} // IncrediBuild::Internal

#ifdef WITH_TESTS
#include <commandbuilderaspect.moc>
#endif
