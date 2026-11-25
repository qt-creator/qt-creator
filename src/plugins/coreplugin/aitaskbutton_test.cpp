// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aitaskbutton_test.h"

#include "aitaskbutton.h"
#include "progressmanager/progressmanager.h"

#include <utils/aiassistance.h>

#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include <memory>

using namespace QtTaskTree;
using namespace Utils;

namespace Core::Internal {

const char providerId[] = "Test.AiTaskButtonProvider";
const char progressId[] = "Test.AiTaskButtonProgress";

class SilentAiProvider final : public AiProvider
{
public:
    SilentAiProvider()
        : AiProvider(providerId, "Silent")
    {
        AiTarget target;
        target.id = "silent";
        target.displayName = "Silent";
        target.supportsHeadless = true;
        setTargets({target});
    }

    // Never answers, so that the request runs until it is canceled.
    void execute(const AiRequest &, const AiTargetSelection &, AiResponse *response) final
    {
        ++requests;
        connect(response, &AiResponse::cancelRequested, this, [this] { ++cancelRequests; });
    }

    int requests = 0;
    int cancelRequests = 0;
};

static AiTaskButton::RecipeFactory silentRequestRecipe()
{
    return [] {
        const auto onSetup = [](AiRequestRunner &runner) {
            AiRequest request;
            request.prompt = "Take your time";
            request.mode = AiRequest::Headless;
            runner.setRequest(request);
            runner.setSelection({providerId, "silent", {}});
        };
        return Group{AiRequestTask(onSetup)};
    };
}

static QWidget *spinner(const AiTaskButton &button)
{
    return button.findChild<QWidget *>("aiTaskButtonSpinner");
}

class AiTaskButtonTest final : public QObject
{
    Q_OBJECT

private slots:
    void testVisibilityFollowsTargets();
    void testRunAndCancel();
    void testCancelFromProgress();
};

// The button is shown only while a target can run its kind of request, and a
// button without a parent never becomes a window.
void AiTaskButtonTest::testVisibilityFollowsTargets()
{
    if (AiAssistance::hasTargets(AiRequest::Headless))
        QSKIP("A provider of this session already runs headless requests.");

    QWidget parent;
    AiTaskButton button(&parent);
    button.setRequestMode(AiRequest::Headless);
    AiTaskButton orphan;
    orphan.setRequestMode(AiRequest::Headless);
    QVERIFY(button.isHidden());

    auto provider = std::make_unique<SilentAiProvider>();
    QVERIFY(!button.isHidden());
    QVERIFY(!orphan.isVisible());

    provider.reset();
    QVERIFY(button.isHidden());
}

// A click runs the recipe with a spinner and progress, and a second click
// cancels it, and with it the request.
void AiTaskButtonTest::testRunAndCancel()
{
    SilentAiProvider provider;
    QWidget parent;
    AiTaskButton button(&parent);
    button.setRequestMode(AiRequest::Headless);
    button.setIdleToolTip("Do it");
    button.setProgress("Doing It", progressId);
    button.setRecipe(silentRequestRecipe());
    QVERIFY(spinner(button));
    QVERIFY(!spinner(button)->isVisibleTo(&button));
    QCOMPARE(button.toolTip(), "Do it");

    QSignalSpy started(ProgressManager::instance(), &ProgressManager::taskStarted);
    QSignalSpy ended(ProgressManager::instance(), &ProgressManager::allTasksFinished);
    button.click();
    QVERIFY(button.isRunning());
    QVERIFY(spinner(button)->isVisibleTo(&button));
    QVERIFY(button.icon().isNull());
    QCOMPARE(button.toolTip(), "Cancel");
    QVERIFY(started.contains({QVariant::fromValue(Id(progressId))}));
    QTRY_COMPARE(provider.requests, 1);

    button.click();
    QVERIFY(!button.isRunning());
    QCOMPARE(provider.cancelRequests, 1);
    QVERIFY(!spinner(button)->isVisibleTo(&button));
    QVERIFY(!button.icon().isNull());
    QCOMPARE(button.toolTip(), "Do it");
    QTRY_VERIFY(ended.contains({QVariant::fromValue(Id(progressId))}));
}

// Canceling the progress in the status bar cancels the task tree, too.
void AiTaskButtonTest::testCancelFromProgress()
{
    SilentAiProvider provider;
    QWidget parent;
    AiTaskButton button(&parent);
    button.setRequestMode(AiRequest::Headless);
    button.setProgress("Doing It", progressId);
    button.setRecipe(silentRequestRecipe());

    button.start();
    QTRY_COMPARE(provider.requests, 1);
    ProgressManager::cancelTasks(progressId);
    QTRY_VERIFY(!button.isRunning());
    QCOMPARE(provider.cancelRequests, 1);
}

QObject *createAiTaskButtonTest()
{
    return new AiTaskButtonTest;
}

} // namespace Core::Internal

#include "aitaskbutton_test.moc"
