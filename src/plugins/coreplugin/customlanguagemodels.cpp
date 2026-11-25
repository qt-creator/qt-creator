// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customlanguagemodels.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "dialogs/ioptionspage.h"
#include "messagemanager.h"
#include "progressmanager/progressmanager.h"

#include <utils/aiassistance.h>
#include <utils/algorithm.h>
#include <utils/aspectlist.h>
#include <utils/aspects.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/shutdownguard.h>

#include <QFutureWatcher>
#include <QHeaderView>
#include <QLayout>
#include <QPromise>
#include <QPushButton>
#include <QScopeGuard>

#ifdef WITH_TESTS
#include <QSignalSpy>
#include <QTest>
#endif

#include <memory>

using namespace Utils;

namespace Core {
namespace {

class CustomLanguageModel : public AspectContainer
{
public:
    static CustomLanguageModel &fromBaseAspect(BaseAspect &aspect);
    static const CustomLanguageModel &fromBaseAspect(const std::shared_ptr<BaseAspect> &aspect);

    CustomLanguageModel();

    CommandLine commandLine();

    StringAspect name{this};
    FilePathAspect executable{this};
    StringAspect arguments{this};
};

class CustomLanguageModels : public AspectContainer
{
public:
    CustomLanguageModels();

    AspectList models{this};
};

CustomLanguageModels &customLanguageModels()
{
    static GuardedObject<CustomLanguageModels> models;
    return models;
}

class CustomLanguageModelsSettingsPage final : public Core::IOptionsPage
{
public:
    CustomLanguageModelsSettingsPage()
    {
        setId("AI.CustomModels");
        setDisplayName(Tr::tr("Custom Language Models"));
        setCategory(Constants::SETTINGS_CATEGORY_AI);
        setSettingsProvider([] { return &customLanguageModels(); });
    }
};

CommandLine CustomLanguageModel::commandLine()
{
    return CommandLine(executable.effectiveBinary(), arguments(), CommandLine::Raw);
}

CustomLanguageModel &CustomLanguageModel::fromBaseAspect(BaseAspect &aspect)
{
    return static_cast<CustomLanguageModel &>(aspect);
}

const CustomLanguageModel &CustomLanguageModel::fromBaseAspect(
    const std::shared_ptr<BaseAspect> &aspect)
{
    return fromBaseAspect(*aspect);
}

CustomLanguageModel::CustomLanguageModel()
{
    setSettingsKey("CustomLanguageModel");
    name.setSettingsKey("Name");
    name.setDisplayName(Tr::tr("Name"));
    name.setDisplayStyle(StringAspect::LineEditDisplay);
    name.setLabelText(Tr::tr("Name:"));
    name.setValue(Tr::tr("<New model>"));

    executable.setSettingsKey("Executable");
    executable.setDisplayName(Tr::tr("Executable"));
    executable.setLabelText(Tr::tr("Executable:"));
    executable.setExpectedKind(PathChooserKind::ExistingCommand);

    arguments.setSettingsKey("Arguments");
    arguments.setDisplayName(Tr::tr("Arguments"));
    arguments.setLabelText(Tr::tr("Arguments:"));
    arguments.setDisplayStyle(StringAspect::LineEditDisplay);

    using namespace Layouting;
    setLayouter([this] { return Form { name, br, executable, br, arguments }; });
}

CustomLanguageModels::CustomLanguageModels()
{
    setAutoApply(false);
    setSettingsGroup("CustomLanguageModels");
    models.setCreateItemFunction([] { return std::make_shared<CustomLanguageModel>(); });
    models.setSettingsKey("ModelList");
    models.setDisplayStyle(AspectList::DisplayStyle::ListViewWithDetails);
    models.listViewDataCallback = [](CustomLanguageModel *aspect, int role)->QVariant {
        if (role == Qt::DisplayRole)
            return aspect->name.volatileValue();
        return {};
    };

    setLayouter([this] {
        using namespace Layouting;
        return Column { models };
    });

    readSettings();
}

} // namespace

const QStringList availableLanguageModels()
{
    return Utils::transform(
        customLanguageModels().models.items(), [](const std::shared_ptr<BaseAspect> &aspect) {
            return CustomLanguageModel::fromBaseAspect(aspect).name();
        });
}

CommandLine commandLineForLanguageModel(const QString &model)
{
    for (const auto &l = customLanguageModels().models.items(); const auto &i : l) {
        if (auto &m = CustomLanguageModel::fromBaseAspect(*i); m.name() == model)
            return m.commandLine();
    }
    return {};
}

BaseAspect &customLanguageModelsContext()
{
    return customLanguageModels();
}

namespace Internal {

const char customLanguageModelProgressId[] = "Core.CustomLanguageModel.Query";

// Runs the command line of a model with the request as standard input, and
// takes its standard output as answer.
class CustomLanguageModelsAiProvider final : public AiProvider
{
public:
    CustomLanguageModelsAiProvider()
        : AiProvider("Core.CustomLanguageModels", Tr::tr("Custom Language Models"))
    {
        connect(&customLanguageModels(), &BaseAspect::changed,
                this, &CustomLanguageModelsAiProvider::updateTargets);
        updateTargets();
    }

    void execute(const AiRequest &request, const AiTargetSelection &selection,
                 AiResponse *response) final
    {
        const QString model = selection.targetId;
        const CommandLine cmdLine = commandLineForLanguageModel(model);
        if (cmdLine.isEmpty()) {
            response->reportFinished(
                ResultError(Tr::tr("The language model \"%1\" is not configured.").arg(model)));
            return;
        }

        const bool interactive = request.mode == AiRequest::Interactive;
        const auto answer = std::make_shared<QString>();
        const auto process = new Process(response);
        process->setCommand(cmdLine);
        if (!request.workingDirectory.isEmpty())
            process->setWorkingDirectory(request.workingDirectory);
        process->setWriteData(AiAssistance::toPlainText(request).toUtf8());
        process->setTextChannelMode(Channel::Output, TextChannelMode::MultiLine);
        connect(process, &Process::textOnStandardOutput,
                response, [response, answer, interactive](const QString &text) {
            answer->append(text);
            response->reportText(text);
            if (interactive)
                MessageManager::writeFlashing(text);
        });
        connect(process, &Process::done, response, [process, response, answer, interactive] {
            if (process->result() == ProcessResult::FinishedWithSuccess) {
                response->reportFinished(*answer);
                return;
            }
            const QString error = process->exitMessage(Process::FailureMessageFormat::WithStdErr);
            if (interactive)
                MessageManager::writeSilently(error);
            response->reportFinished(ResultError(error));
        });
        connect(response, &AiResponse::cancelRequested, process, [process] { process->kill(); });
        if (interactive) {
            MessageManager::writeDisrupting(Tr::tr("Querying %1...").arg(model));
            // Nothing else can stop an interactive request.
            showCancelableProgress(Tr::tr("Querying %1").arg(model), response);
        }
        process->start();
    }

private:
    static void showCancelableProgress(const QString &title, AiResponse *response)
    {
        const auto promise = std::make_shared<QPromise<void>>();
        promise->start();
        ProgressManager::addTask(promise->future(), title, customLanguageModelProgressId);
        const auto watcher = new QFutureWatcher<void>(response);
        connect(watcher, &QFutureWatcherBase::canceled, response, [response] {
            response->cancel();
        });
        watcher->setFuture(promise->future());
        connect(response, &AiResponse::finished, response, [promise] { promise->finish(); });
    }

    void updateTargets()
    {
        QList<AiTarget> targets;
        for (const QString &model : availableLanguageModels()) {
            AiTarget target;
            target.id = model;
            target.displayName = model;
            target.description = Tr::tr("Runs %1 and shows its answer in General Messages.")
                                     .arg(model);
            target.supportsHeadless = true;
            targets.append(target);
        }
        setTargets(targets);
    }
};

void setupCustomLanguageModels()
{
    (void) customLanguageModels();
    static const CustomLanguageModelsSettingsPage settingsPage;
    static GuardedObject<CustomLanguageModelsAiProvider> aiProvider;
}

#ifdef WITH_TESTS

class CustomLanguageModelsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCancelInteractiveFromProgress();
};

// An interactive request has no other way to stop a model that does not end.
void CustomLanguageModelsTest::testCancelInteractiveFromProgress()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("The model is a POSIX shell command.");

    const auto model = std::make_shared<CustomLanguageModel>();
    model->name.setValue("Never Ending Model");
    model->executable.setValue(FilePath::fromString("/bin/sh"));
    model->arguments.setValue("-c \"sleep 600\"");
    customLanguageModels().models.addItem(model);
    customLanguageModels().apply();
    const auto removeModel = qScopeGuard([model] {
        customLanguageModels().models.actualRemoveItem(model);
        customLanguageModels().apply();
    });
    QVERIFY(availableLanguageModels().contains("Never Ending Model"));

    QSignalSpy started(ProgressManager::instance(), &ProgressManager::taskStarted);
    AiRequest request;
    request.prompt = "Explain";
    AiTargetSelection selection;
    selection.providerId = "Core.CustomLanguageModels";
    selection.targetId = "Never Ending Model";
    QSignalSpy finished(AiAssistance::submit(request, selection), &AiResponse::finished);
    QTRY_VERIFY(started.contains({QVariant::fromValue(Id(customLanguageModelProgressId))}));
    QVERIFY(finished.isEmpty());

    ProgressManager::cancelTasks(customLanguageModelProgressId);
    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(!finished.first().first().value<Result<QString>>());
}

QObject *createCustomLanguageModelsTest()
{
    return new CustomLanguageModelsTest;
}

#endif // WITH_TESTS

} // namespace Internal
} // namespace Core

#ifdef WITH_TESTS
#include "customlanguagemodels.moc"
#endif
