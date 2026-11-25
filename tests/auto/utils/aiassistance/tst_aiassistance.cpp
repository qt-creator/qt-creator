// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/aiassistance.h>
#include <utils/qtcsettings_p.h>
#include <utils/theme/theme.h>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QListWidget>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace Utils;

const char providerId[] = "Test.Provider";

class TestProvider final : public AiProvider
{
public:
    TestProvider()
        : AiProvider(providerId, "Test")
    {}

    using AiProvider::setTargets;

    void execute(const AiRequest &request, const AiTargetSelection &selection,
                 AiResponse *response) final
    {
        requests.append(request);
        selections.append(selection);
        lastResponse = response;
        if (!autoAnswer)
            return;
        response->reportText("Hello ");
        response->reportText("World");
        response->reportFinished(QString("Hello World"));
    }

    QList<AiRequest> requests;
    QList<AiTargetSelection> selections;
    QPointer<AiResponse> lastResponse;
    bool autoAnswer = true;
};

static AiTarget chatTarget()
{
    AiTarget target;
    target.id = "chat";
    target.displayName = "Chat";
    target.supportsHeadless = true;
    AiTargetParameter model;
    model.id = "model";
    model.displayName = "Model";
    model.category = AiTargetParameter::Model;
    model.choices = {{"small", "Small", {}}, {"big", "Big", {}}};
    model.defaultValue = "small";
    AiTargetParameter effort;
    effort.id = "effort";
    effort.displayName = "Effort";
    effort.category = AiTargetParameter::Effort;
    effort.choices = {{"low", "Low", {}}, {"high", "High", {}}};
    effort.defaultValue = "high";
    target.parameters = {model, effort};
    return target;
}

static AiTarget paneTarget()
{
    AiTarget target;
    target.id = "pane";
    target.displayName = "Pane";
    return target;
}

static AiTargetSelection selectionFor(const QString &targetId,
                                      const QMap<QString, QString> &parameters = {})
{
    return {Id(providerId), targetId, parameters};
}

static QDialog *openDialog()
{
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible())
            return dialog;
    }
    return nullptr;
}

static QString currentTarget(QDialog *dialog)
{
    const QListWidgetItem *item = dialog->findChild<QListWidget *>()->currentItem();
    return item ? item->text() : QString();
}

static void selectTarget(QDialog *dialog, const QString &displayName)
{
    auto list = dialog->findChild<QListWidget *>();
    const QList<QListWidgetItem *> items = list->findItems(displayName, Qt::MatchExactly);
    QCOMPARE(items.size(), 1);
    list->setCurrentItem(items.first());
}

static QComboBox *parameterCombo(QDialog *dialog, int index)
{
    const QList<QComboBox *> combos = dialog->findChildren<QComboBox *>();
    return index < combos.size() ? combos.at(index) : nullptr;
}

class tst_AiAssistance : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Must run first, before anything created the built-in providers.
    void builtinProvidersNotify();
    void registration();
    void targetsChangedCallback();
    void hasTargets();
    void submitDeliversAfterReturn();
    void submitRejects_data();
    void submitRejects();
    void cancel();
    void cancelBeforeDelivery();
    void requestTask();
    void requestTaskCanceledByTree();
    void clipboard();
    void pickerRemembersSelection();
    void pickerCancel();

private:
    QTemporaryDir m_settingsDir;
};

void tst_AiAssistance::initTestCase()
{
    setCreatorTheme(new Theme("test"));
    QVERIFY(m_settingsDir.isValid());
    const QString fileName = m_settingsDir.filePath("settings.ini");
    Internal::SettingsSetup::setupSettings(new QtcSettings(fileName, QSettings::IniFormat),
                                           new QtcSettings(fileName, QSettings::IniFormat));
}

void tst_AiAssistance::cleanupTestCase()
{
    Internal::SettingsSetup::destroySettings();
    setCreatorTheme(nullptr);
}

void tst_AiAssistance::builtinProvidersNotify()
{
    QObject guard;
    int calls = 0;
    AiAssistance::onTargetsChanged(&guard, [&calls] {
        ++calls;
        AiAssistance::hasTargets(AiRequest::Interactive);
    });
    QVERIFY(AiAssistance::hasTargets(AiRequest::Interactive));
    QCOMPARE(calls, 2);
}

void tst_AiAssistance::registration()
{
    QVERIFY(!AiAssistance::provider(providerId));
    {
        TestProvider provider;
        QCOMPARE(AiAssistance::provider(providerId), &provider);
        const QList<AiProvider *> providers = AiAssistance::providers();
        QCOMPARE(providers.size(), 2);
        QCOMPARE(providers.first(), &provider);
        QCOMPARE(providers.last()->id(), AiAssistance::clipboardProviderId());
    }
    QVERIFY(!AiAssistance::provider(providerId));
    QCOMPARE(AiAssistance::providers().size(), 1);
}

void tst_AiAssistance::targetsChangedCallback()
{
    auto guard = std::make_unique<QObject>();
    int calls = 0;
    AiAssistance::onTargetsChanged(guard.get(), [&calls] { ++calls; });

    auto provider = std::make_unique<TestProvider>();
    QCOMPARE(calls, 1);
    provider->setTargets({chatTarget()});
    QCOMPARE(calls, 2);
    provider.reset();
    QCOMPARE(calls, 3);

    guard.reset();
    TestProvider other;
    other.setTargets({chatTarget()});
    QCOMPARE(calls, 3);
}

void tst_AiAssistance::hasTargets()
{
    QVERIFY(AiAssistance::hasTargets(AiRequest::Interactive));
    QVERIFY(!AiAssistance::hasTargets(AiRequest::Headless));

    TestProvider provider;
    provider.setTargets({paneTarget()});
    QVERIFY(!AiAssistance::hasTargets(AiRequest::Headless));
    provider.setTargets({paneTarget(), chatTarget()});
    QVERIFY(AiAssistance::hasTargets(AiRequest::Headless));
}

void tst_AiAssistance::submitDeliversAfterReturn()
{
    TestProvider provider;
    provider.setTargets({chatTarget()});

    AiRequest request;
    request.prompt = "Say hello";
    request.mode = AiRequest::Headless;
    AiResponse *response = AiAssistance::submit(request, selectionFor("chat", {{"model", "big"}}));
    QSignalSpy textSpy(response, &AiResponse::textReceived);
    QSignalSpy finishedSpy(response, &AiResponse::finished);
    QVERIFY(provider.requests.isEmpty());

    QTRY_COMPARE(finishedSpy.count(), 1);
    const auto result = finishedSpy.first().first().value<Result<QString>>();
    QVERIFY(result);
    QCOMPARE(*result, "Hello World");
    QCOMPARE(textSpy.count(), 2);
    QCOMPARE(textSpy.at(0).first().toString(), "Hello ");
    QCOMPARE(textSpy.at(1).first().toString(), "World");

    QCOMPARE(provider.requests.size(), 1);
    QCOMPARE(provider.requests.first().prompt, "Say hello");
    const QMap<QString, QString> expected{{"model", "big"}, {"effort", "high"}};
    QCOMPARE(provider.selections.first().parameters, expected);
}

void tst_AiAssistance::submitRejects_data()
{
    QTest::addColumn<AiTargetSelection>("selection");
    QTest::addColumn<int>("mode");

    QTest::newRow("unknown provider")
        << AiTargetSelection{Id("Test.Nobody"), "chat", {}} << int(AiRequest::Interactive);
    QTest::newRow("unknown target") << selectionFor("nothing") << int(AiRequest::Interactive);
    QTest::newRow("headless on interactive target")
        << selectionFor("pane") << int(AiRequest::Headless);
}

void tst_AiAssistance::submitRejects()
{
    QFETCH(AiTargetSelection, selection);
    QFETCH(int, mode);

    TestProvider provider;
    provider.setTargets({chatTarget(), paneTarget()});

    AiRequest request;
    request.prompt = "Do something";
    request.mode = AiRequest::Mode(mode);
    QSignalSpy finishedSpy(AiAssistance::submit(request, selection), &AiResponse::finished);
    QTRY_COMPARE(finishedSpy.count(), 1);
    const auto result = finishedSpy.first().first().value<Result<QString>>();
    QVERIFY(!result);
    QVERIFY(!result.error().isEmpty());
    QVERIFY(provider.requests.isEmpty());
}

void tst_AiAssistance::cancel()
{
    TestProvider provider;
    provider.autoAnswer = false;
    provider.setTargets({chatTarget()});

    AiRequest request;
    request.prompt = "Take your time";
    AiResponse *response = AiAssistance::submit(request, selectionFor("chat"));
    QSignalSpy cancelSpy(response, &AiResponse::cancelRequested);
    QSignalSpy finishedSpy(response, &AiResponse::finished);
    QTRY_VERIFY(provider.lastResponse);

    response->cancel();
    QCOMPARE(cancelSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 1);
    QVERIFY(!finishedSpy.first().first().value<Result<QString>>());

    response->reportFinished(QString("Too late"));
    response->cancel();
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(cancelSpy.count(), 1);

    QTRY_VERIFY(provider.lastResponse.isNull());
}

// A response canceled before the target got the request never reaches it.
void tst_AiAssistance::cancelBeforeDelivery()
{
    TestProvider provider;
    provider.setTargets({chatTarget()});

    AiRequest request;
    request.prompt = "Never mind";
    QPointer<AiResponse> response = AiAssistance::submit(request, selectionFor("chat"));
    QSignalSpy finishedSpy(response, &AiResponse::finished);
    response->cancel();
    QCOMPARE(finishedSpy.count(), 1);

    QTRY_VERIFY(response.isNull());
    QVERIFY(provider.requests.isEmpty());
}

// An AI request runs as a task, which succeeds with the answer.
void tst_AiAssistance::requestTask()
{
    TestProvider provider;
    provider.setTargets({chatTarget()});

    QString answer;
    const auto onSetup = [](AiRequestRunner &runner) {
        AiRequest request;
        request.prompt = "Say hello";
        request.mode = AiRequest::Headless;
        runner.setRequest(request);
        runner.setSelection(selectionFor("chat"));
    };
    const auto onDone = [&answer](const AiRequestRunner &runner) {
        answer = runner.result().value_or(QString());
    };
    QtTaskTree::QTaskTree tree({AiRequestTask(onSetup, onDone)});
    QSignalSpy doneSpy(&tree, &QtTaskTree::QTaskTree::done);
    tree.start();
    QTRY_COMPARE(doneSpy.count(), 1);
    QCOMPARE(doneSpy.first().first().value<QtTaskTree::DoneWith>(), QtTaskTree::DoneWith::Success);
    QCOMPARE(answer, "Hello World");
}

// Canceling the task tree cancels the request that runs in it.
void tst_AiAssistance::requestTaskCanceledByTree()
{
    TestProvider provider;
    provider.autoAnswer = false;
    provider.setTargets({chatTarget()});

    const auto onSetup = [](AiRequestRunner &runner) {
        AiRequest request;
        request.prompt = "Take your time";
        runner.setRequest(request);
        runner.setSelection(selectionFor("chat"));
    };
    QtTaskTree::QTaskTree tree({AiRequestTask(onSetup)});
    tree.start();
    QTRY_VERIFY(provider.lastResponse);
    QSignalSpy cancelSpy(provider.lastResponse.data(), &AiResponse::cancelRequested);

    tree.cancel();
    QCOMPARE(cancelSpy.count(), 1);
    QTRY_VERIFY(provider.lastResponse.isNull());
}

void tst_AiAssistance::clipboard()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath file = FilePath::fromString(dir.filePath("source.cpp"));
    QVERIFY(file.writeFileContents("line 1\nline 2\nline 3\nline 4\nline 5\n"));

    Text::Range range;
    range.begin.line = 2;
    range.end.line = 3;

    AiRequest request;
    request.prompt = "Explain this";
    request.context = {AiContextItem::fromFile(file, range),
                       AiContextItem::fromText("Snippet", "a ``` fence"),
                       AiContextItem::fromImage("Screenshot", QImage(4, 4, QImage::Format_RGB32))};

    const QString text = AiAssistance::toPlainText(request);
    QVERIFY(text.startsWith("Explain this\n"));
    QVERIFY(text.contains("\nline 2\nline 3\n"));
    QVERIFY(!text.contains("line 1"));
    QVERIFY(!text.contains("line 4"));
    QVERIFY(text.contains("````\na ``` fence\n````\n"));
    QVERIFY(text.contains("Screenshot"));

    QGuiApplication::clipboard()->clear();
    QSignalSpy finishedSpy(
        AiAssistance::submit(request, {AiAssistance::clipboardProviderId(), "Clipboard", {}}),
        &AiResponse::finished);
    QTRY_COMPARE(finishedSpy.count(), 1);
    QVERIFY(finishedSpy.first().first().value<Result<QString>>());
    QCOMPARE(QGuiApplication::clipboard()->text(), text);
}

void tst_AiAssistance::pickerRemembersSelection()
{
    TestProvider provider;
    provider.setTargets({chatTarget(), paneTarget()});

    AiRequest request;
    request.id = "Test.Request";
    request.prompt = "Pick me";

    QSignalSpy firstSpy(AiAssistance::submit(request), &AiResponse::finished);
    QDialog *dialog = nullptr;
    QTRY_VERIFY((dialog = openDialog()));
    selectTarget(dialog, "Chat");
    if (QTest::currentTestFailed())
        return;
    QComboBox *model = parameterCombo(dialog, 0);
    QVERIFY(model);
    model->setCurrentIndex(model->findText("Big"));
    dialog->accept();
    QTRY_COMPARE(firstSpy.count(), 1);
    QVERIFY(firstSpy.first().first().value<Result<QString>>());
    QCOMPARE(provider.selections.size(), 1);
    QCOMPARE(provider.selections.first().targetId, "chat");
    QCOMPARE(provider.selections.first().parameters.value("model"), "big");

    const std::optional<AiTargetSelection> remembered
        = AiAssistance::rememberedSelection("Test.Request");
    QVERIFY(remembered);
    QCOMPARE(*remembered, provider.selections.first());
    QCOMPARE(AiAssistance::rememberedSelection("Test.Other"), remembered);

    QSignalSpy secondSpy(AiAssistance::submit(request), &AiResponse::finished);
    QTRY_VERIFY((dialog = openDialog()));
    QCOMPARE(currentTarget(dialog), "Chat");
    QCOMPARE(parameterCombo(dialog, 0)->currentText(), "Big");
    selectTarget(dialog, "Pane");
    if (QTest::currentTestFailed())
        return;
    QVERIFY(!parameterCombo(dialog, 0));
    dialog->accept();
    QTRY_COMPARE(secondSpy.count(), 1);
    QCOMPARE(provider.selections.size(), 2);
    QCOMPARE(provider.selections.last().targetId, "pane");
    QCOMPARE(AiAssistance::rememberedSelection("Test.Request")->targetId, "pane");
}

void tst_AiAssistance::pickerCancel()
{
    TestProvider provider;
    provider.setTargets({chatTarget()});

    AiRequest request;
    request.prompt = "Never mind";
    AiResponse *response = AiAssistance::submit(request);
    QSignalSpy finishedSpy(response, &AiResponse::finished);
    QDialog *dialog = nullptr;
    QTRY_VERIFY((dialog = openDialog()));
    QPointer<QDialog> dialogGuard(dialog);
    dialog->reject();
    QTRY_COMPARE(finishedSpy.count(), 1);
    QVERIFY(!finishedSpy.first().first().value<Result<QString>>());
    QVERIFY(provider.requests.isEmpty());
    QTRY_VERIFY(dialogGuard.isNull());

    AiResponse *canceled = AiAssistance::submit(request);
    QSignalSpy cancelSpy(canceled, &AiResponse::finished);
    QTRY_VERIFY((dialog = openDialog()));
    dialogGuard = dialog;
    canceled->cancel();
    QCOMPARE(cancelSpy.count(), 1);
    QTRY_VERIFY(dialogGuard.isNull());
    QVERIFY(provider.requests.isEmpty());
}

QTEST_MAIN(tst_AiAssistance)

#include "tst_aiassistance.moc"
