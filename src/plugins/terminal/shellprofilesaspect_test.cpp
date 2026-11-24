// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "shellprofilesaspect_test.h"

#include "shellprofilesaspect.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>

#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>

namespace Terminal::Internal {

static ShellProfile profile(const QString &id, bool detected)
{
    ShellProfile result;
    result.id = id;
    result.name = id;
    result.executable = Utils::FilePath::fromString("/bin/" + id);
    result.isDetected = detected;
    return result;
}

static QStringList ids(const QList<ShellProfile> &profiles)
{
    return Utils::transform(profiles, &ShellProfile::id);
}

static const QList<ShellProfile> detected{profile("a", true),
                                          profile("b", true),
                                          profile("c", true)};

class ShellProfilesAspectTest final : public QObject
{
    Q_OBJECT

private slots:
    void testOrder()
    {
        ShellProfileSettings settings;
        settings.manualProfiles = {profile("m2", false), profile("m1", false)};
        settings.detectedOrder = {"c", "a"};

        QCOMPARE(ids(orderedShellProfiles(settings, detected)),
                 QStringList({"c", "a", "b", "m2", "m1"}));
    }

    void testHiddenProfilesAreNotVisible()
    {
        ShellProfileSettings settings;
        settings.manualProfiles = {profile("m", false)};
        settings.hiddenProfiles = {"b", "m"};

        // Only detected profiles can be hidden; a manual one is removed instead.
        QCOMPARE(ids(visibleShellProfiles(settings, detected)), QStringList({"a", "c", "m"}));
    }

    void testDefault_data()
    {
        QTest::addColumn<QString>("defaultProfile");
        QTest::addColumn<QStringList>("hidden");
        QTest::addColumn<QString>("expected");

        QTest::newRow("chosen") << "m" << QStringList() << "m";
        QTest::newRow("none chosen") << "" << QStringList() << "a";
        QTest::newRow("chosen is gone") << "gone" << QStringList() << "a";
        QTest::newRow("chosen is hidden") << "b" << QStringList({"a", "b"}) << "c";
    }

    void testDefault()
    {
        QFETCH(QString, defaultProfile);
        QFETCH(QStringList, hidden);
        QFETCH(QString, expected);

        ShellProfileSettings settings;
        settings.manualProfiles = {profile("m", false)};
        settings.defaultProfile = defaultProfile;
        settings.hiddenProfiles = hidden;

        QCOMPARE(defaultShellProfile(settings, detected.first(), [] { return detected; }).id,
                 expected);
    }

    void testDefaultWithNothingVisible()
    {
        ShellProfileSettings settings;
        settings.hiddenProfiles = {"a", "b", "c"};

        QCOMPARE(defaultShellProfile(settings, detected.first(), [] { return detected; }).id,
                 QString("a"));
    }

    void testSettingsRoundTrip()
    {
        ShellProfile manual = profile("m", false);
        manual.arguments = "-l -i";
        manual.environmentChanges = Utils::EnvironmentChanges(
            Utils::EnvironmentItem::fromStringList({"FOO=bar", "BAZ"}));
        manual.workingDirectory = Utils::FilePath::fromString("/tmp/work");
        manual.environmentChanges.setFile(Utils::FilePath::fromString("/tmp/env.sh"), true);
        manual.iconPath = Utils::FilePath::fromString("/tmp/icon.ico");

        ShellProfileSettings settings;
        settings.manualProfiles = {manual, profile("n", false)};
        settings.hiddenProfiles = {"b"};
        settings.detectedOrder = {"c", "a", "b"};
        settings.defaultProfile = "n";

        ShellProfilesAspect aspect(nullptr);
        aspect.setSettingsKey("ShellProfiles");
        aspect.setValue(settings);
        Utils::Store map;
        aspect.toMap(map);

        ShellProfilesAspect restored(nullptr);
        restored.setSettingsKey("ShellProfiles");
        restored.fromMap(map);

        QCOMPARE(restored.value(), settings);
    }

    void testEditorAppliesAndCancels()
    {
        Utils::AspectContainer container;
        container.setAutoApply(false);
        ShellProfilesAspect aspect(&container);

        QWidget widget;
        Layouting::Column{aspect}.attachTo(&widget);

        const QList<QPushButton *> buttons = widget.findChildren<QPushButton *>();
        QPushButton *add = Utils::findOrDefault(buttons, [](QPushButton *button) {
            return button->text() == "Add";
        });
        QVERIFY(add);
        auto tree = widget.findChild<QTreeWidget *>();
        QVERIFY(tree);
        QTreeWidgetItem *manualRoot = tree->topLevelItem(1);
        QVERIFY(manualRoot);

        add->click();
        QVERIFY(container.isDirty());
        QCOMPARE(aspect.volatileValue().manualProfiles.size(), 1);
        QCOMPARE(aspect.value().manualProfiles.size(), 0);

        container.cancel();
        QCOMPARE(aspect.volatileValue().manualProfiles.size(), 0);
        QCOMPARE(tree->topLevelItem(1)->childCount(), 0);

        add->click();
        container.apply();
        QCOMPARE(aspect.value().manualProfiles.size(), 1);
        QCOMPARE(tree->topLevelItem(1)->childCount(), 1);
        QCOMPARE(tree->currentItem()->data(0, Qt::UserRole).toString(),
                 aspect.value().manualProfiles.first().id);
    }

    void testEditorShowsWorkingDirectory()
    {
        ShellProfile manual = profile("m", false);
        manual.workingDirectory = Utils::FilePath::fromString("/tmp/work");
        manual.environmentChanges.setFile(Utils::FilePath::fromString("/tmp/env.sh"), true);
        manual.iconPath = Utils::FilePath::fromString("/tmp/icon.ico");
        ShellProfileSettings settings;
        settings.manualProfiles = {manual};

        Utils::AspectContainer container;
        container.setAutoApply(false);
        ShellProfilesAspect aspect(&container);
        aspect.setValue(settings);

        QWidget widget;
        Layouting::Column{aspect}.attachTo(&widget);

        auto tree = widget.findChild<QTreeWidget *>();
        QVERIFY(tree);
        QCOMPARE(tree->columnCount(), 3);
        QTreeWidgetItem *manualRoot = tree->topLevelItem(1);
        QVERIFY(manualRoot);
        QCOMPARE(manualRoot->childCount(), 1);
        QCOMPARE(manualRoot->child(0)->text(2), manual.workingDirectory.toUserOutput());
    }

    void testWithDefaultShell_data()
    {
        QTest::addColumn<QString>("callerDirectory");
        QTest::addColumn<QString>("callerShell");
        QTest::addColumn<QString>("defaultDirectory");
        QTest::addColumn<QString>("profileDirectory");
        QTest::addColumn<QString>("expectedShell");
        QTest::addColumn<QString>("expectedDirectory");

        const QString profileShell = "/bin/zsh -l";
        const QString remote = "docker://image/project";

        QTest::newRow("no directory") << "" << "" << "" << "" << profileShell << "";
        QTest::newRow("local project")
            << "" << "" << "/tmp/project" << "" << profileShell << "/tmp/project";
        QTest::newRow("remote project") << "" << "" << remote << "" << "" << remote;
        QTest::newRow("remote project, profile directory")
            << "" << "" << remote << "/tmp/profile" << "" << remote;
        QTest::newRow("profile directory before project")
            << "" << "" << "/tmp/project" << "/tmp/profile" << profileShell << "/tmp/profile";
        QTest::newRow("caller directory before profile")
            << "/tmp/caller" << "" << "/tmp/project" << "/tmp/profile" << profileShell
            << "/tmp/caller";
        QTest::newRow("remote caller directory")
            << remote << "" << "/tmp/project" << "" << "" << remote;
        QTest::newRow("caller shell")
            << "" << "/bin/bash" << "/tmp/project" << "/tmp/profile" << "/bin/bash"
            << "/tmp/project";
    }

    void testWithDefaultShell()
    {
        QFETCH(QString, callerDirectory);
        QFETCH(QString, callerShell);
        QFETCH(QString, defaultDirectory);
        QFETCH(QString, profileDirectory);
        QFETCH(QString, expectedShell);
        QFETCH(QString, expectedDirectory);

        ShellProfile defaultProfile = profile("zsh", true);
        defaultProfile.arguments = "-l";
        defaultProfile.workingDirectory = Utils::FilePath::fromString(profileDirectory);

        Utils::Terminal::OpenTerminalParameters parameters;
        if (!callerDirectory.isEmpty())
            parameters.workingDirectory = Utils::FilePath::fromString(callerDirectory);
        if (!callerShell.isEmpty())
            parameters.shellCommand = Utils::CommandLine{Utils::FilePath::fromString(callerShell)};

        const Utils::Terminal::OpenTerminalParameters result
            = withDefaultShell(parameters,
                               Utils::FilePath::fromString(defaultDirectory),
                               defaultProfile);

        QCOMPARE(result.shellCommand ? result.shellCommand->toUserOutput() : QString(),
                 expectedShell);
        QCOMPARE(result.workingDirectory ? result.workingDirectory->toUserOutput() : QString(),
                 expectedDirectory);
    }

    void testWithDefaultShellAppliesEnvironment()
    {
        ShellProfile defaultProfile = profile("zsh", true);
        defaultProfile.environmentChanges = Utils::EnvironmentChanges(
            Utils::EnvironmentItem::fromStringList({"FOO=bar"}));

        Utils::Environment callerEnvironment;
        callerEnvironment.set("BAZ", "qux");
        Utils::Terminal::OpenTerminalParameters parameters;
        parameters.environment = callerEnvironment;

        const Utils::Terminal::OpenTerminalParameters result
            = withDefaultShell(parameters, {}, defaultProfile);

        QVERIFY(result.environment);
        const Utils::Environment environment
            = result.environment->appliedToEnvironment(Utils::Environment());
        QCOMPARE(environment.value("FOO"), QString("bar"));
        QCOMPARE(environment.value("BAZ"), QString("qux"));
    }

    void testRemovedMigratedShellStaysRemoved()
    {
        Utils::QtcSettings &settings = Utils::userSettings();
        const Utils::Key group("ShellProfilesMigrationTest");
        settings.remove(group);
        settings.setValue(group + "/ShellPath",
                          Utils::FilePath::fromString("/bin/sh").toSettings());

        {
            Utils::AspectContainer container;
            container.setSettingsGroup(Utils::stringFromKey(group));
            container.setAutoApply(false);
            ShellProfilesAspect aspect(&container);
            aspect.setSettingsKey("ShellProfiles");
            container.readSettings();
            migrateLegacyShellSetting(aspect, group);

            QCOMPARE(aspect.value().manualProfiles.size(), 1);
            QCOMPARE(aspect.value().manualProfiles.first().executable,
                     Utils::FilePath::fromString("/bin/sh"));
            QCOMPARE(aspect.value().defaultProfile, aspect.value().manualProfiles.first().id);

            // What removing the profile on the settings page and applying does.
            aspect.setValue({});
            container.writeSettings();
        }

        Utils::AspectContainer container;
        container.setSettingsGroup(Utils::stringFromKey(group));
        ShellProfilesAspect aspect(&container);
        aspect.setSettingsKey("ShellProfiles");
        container.readSettings();
        migrateLegacyShellSetting(aspect, group);
        const bool migratedAgain = !aspect.value().manualProfiles.isEmpty();

        settings.remove(group);
        QVERIFY(!migratedAgain);
    }

    void testSystemShellIsDetectedOnce()
    {
        const Utils::FilePath system = systemShellProfile().executable;
        const QList<ShellProfile> detected = detectedShellProfiles();
        QCOMPARE(Utils::count(detected,
                              [&system](const ShellProfile &p) { return p.executable == system; }),
                 1);
    }

    void testDefaultDoesNotDetectUnlessNeeded_data()
    {
        QTest::addColumn<QString>("defaultProfile");
        QTest::addColumn<QStringList>("hidden");
        QTest::addColumn<QStringList>("order");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<bool>("detects");

        QTest::newRow("none chosen") << "" << QStringList() << QStringList() << "a" << false;
        QTest::newRow("system chosen") << "a" << QStringList() << QStringList() << "a" << false;
        QTest::newRow("manual chosen") << "m" << QStringList() << QStringList() << "m" << false;
        QTest::newRow("system first in order")
            << "" << QStringList() << QStringList({"a", "c"}) << "a" << false;
        QTest::newRow("other detected chosen")
            << "b" << QStringList() << QStringList() << "b" << true;
        QTest::newRow("other detected first")
            << "" << QStringList() << QStringList({"c", "a"}) << "c" << true;
        QTest::newRow("system hidden") << "" << QStringList({"a"}) << QStringList() << "b" << true;
    }

    void testDefaultDoesNotDetectUnlessNeeded()
    {
        QFETCH(QString, defaultProfile);
        QFETCH(QStringList, hidden);
        QFETCH(QStringList, order);
        QFETCH(QString, expected);
        QFETCH(bool, detects);

        ShellProfileSettings settings;
        settings.manualProfiles = {profile("m", false)};
        settings.defaultProfile = defaultProfile;
        settings.hiddenProfiles = hidden;
        settings.detectedOrder = order;

        int detections = 0;
        const ShellProfile result = defaultShellProfile(settings, detected.first(), [&detections] {
            ++detections;
            return detected;
        });

        QCOMPARE(result.id, expected);
        QCOMPARE(detections > 0, detects);
    }

    void testProfileExpandsMacros()
    {
        const QString home = Utils::qtcEnvironmentVariable("HOME");
        if (home.isEmpty())
            QSKIP("The test expands %{Env:HOME}, which is not set.");

        ShellProfile defaultProfile = profile("zsh", false);
        defaultProfile.executable = Utils::FilePath::fromString("%{Env:HOME}/bin/zsh");
        defaultProfile.arguments = "--rcs %{Env:HOME}/rc";
        defaultProfile.workingDirectory = Utils::FilePath::fromString("%{Env:HOME}/work");

        const Utils::Terminal::OpenTerminalParameters result
            = withDefaultShell({}, {}, defaultProfile);

        QVERIFY(result.shellCommand);
        QCOMPARE(result.shellCommand->executable(),
                 Utils::FilePath::fromString(home + "/bin/zsh"));
        QCOMPARE(result.shellCommand->arguments(), QString("--rcs " + home + "/rc"));
        QVERIFY(result.workingDirectory);
        QCOMPARE(*result.workingDirectory, Utils::FilePath::fromString(home + "/work"));

        // The editor shows what the user entered.
        QCOMPARE(defaultProfile.unexpandedCommandLine().executable(), defaultProfile.executable);
    }

    void testProfileAppliesEnvironmentFile()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const Utils::FilePath file = Utils::FilePath::fromString(directory.filePath("env.txt"));
        QVERIFY(file.writeFileContents("FROM_FILE=yes\n"));

        ShellProfile defaultProfile = profile("zsh", false);
        defaultProfile.environmentChanges = Utils::EnvironmentChanges(
            Utils::EnvironmentItem::fromStringList({"FROM_USER=yes"}));
        defaultProfile.environmentChanges.setFile(file, false);

        const Utils::Terminal::OpenTerminalParameters result
            = withDefaultShell({}, {}, defaultProfile);

        QVERIFY(result.environment);
        const Utils::Environment environment
            = result.environment->appliedToEnvironment(Utils::Environment());
        QCOMPARE(environment.value("FROM_FILE"), QString("yes"));
        QCOMPARE(environment.value("FROM_USER"), QString("yes"));
    }
};

QObject *createShellProfilesAspectTest()
{
    return new ShellProfilesAspectTest;
}

} // namespace Terminal::Internal

#include "shellprofilesaspect_test.moc"
