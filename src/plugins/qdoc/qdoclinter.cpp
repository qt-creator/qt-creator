// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qdoclinter.h"

#include "qdoctr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/taskhub.h>

#include <texteditor/texteditorconstants.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>

#include <QRegularExpression>

using namespace Utils;

namespace QDoc::Internal {

const char TASK_CATEGORY[] = "QDoc.Diagnostics";

const QList<Rule> &ruleTable()
{
    static const QList<Rule> rules = {
        {"unknown-command", RuleLevel::Warning,
         Tr::tr("A command or macro QDoc does not know, usually a typo such as \\macOS")},
        {"deprecated-command", RuleLevel::Info,
         Tr::tr("A command QDoc still accepts but advises replacing, such as \\bold")},
        {"unterminated-block", RuleLevel::Error,
         Tr::tr("A missing '}', \\endcode, \\endlist or other terminator")},
        {"misplaced-command", RuleLevel::Warning,
         Tr::tr("\\li outside a list or table, a stray \\end..., \\target outside a cell")},
        {"invalid-argument", RuleLevel::Warning,
         Tr::tr("A command argument QDoc cannot use, or a required one that is missing")},
        {"list-style", RuleLevel::Warning, Tr::tr("An unrecognized \\list style hint")},
        {"sa-missing-comma", RuleLevel::Warning,
         Tr::tr("\\sa entries must be separated by commas")},
        {"sa-trailing-punctuation", RuleLevel::Hint,
         Tr::tr("A full stop closing a \\sa list, which QDoc drops and then reports as a "
                "missing comma")},
        {"sa-empty", RuleLevel::Hint, Tr::tr("\\sa naming no targets at all")},
        {"duplicate-target", RuleLevel::Warning,
         Tr::tr("The same \\target or \\keyword name twice, making one unreachable")},
        {"macro-arguments", RuleLevel::Warning,
         Tr::tr("A macro invoked with too few arguments, or one with no definition")},
        {"code-language", RuleLevel::Warning,
         Tr::tr("A \\code language not listed in codelanguages")},
        {"brief-punctuation", RuleLevel::Info,
         Tr::tr("A \\brief that does not end with a full stop, which is common in Qt")},
        {"missing-topic-command", RuleLevel::Info,
         Tr::tr("A .qdoc comment documenting nothing, with no \\class, \\page or similar")},
        {"no-such-parameter", RuleLevel::Warning,
         Tr::tr("\\a naming something absent from the signature")},
        {"redundant-sa-self", RuleLevel::Info,
         Tr::tr("\\sa pointing at the thing being documented")},
        {"expanded-text", RuleLevel::Off,
         Tr::tr("Problems inside text pulled in by a macro or \\include, which is not "
                "written in this file")},
        {"missing-image", RuleLevel::Warning, Tr::tr("An \\image not found in imagedirs")},
        {"missing-snippet", RuleLevel::Warning,
         Tr::tr("A \\snippet whose file or marker does not exist")},
        {"missing-include", RuleLevel::Warning,
         Tr::tr("An \\include target that cannot be found")},
        {"missing-alt-text", RuleLevel::Warning,
         Tr::tr("An \\image with no textual description")},
        {"quote-command", RuleLevel::Warning,
         Tr::tr("A \\printto or \\skipto that matched nothing")},
        {"config-syntax", RuleLevel::Warning,
         Tr::tr("A problem reading the .qdocconf: bad include(), missing =, unterminated "
                "string")},
        {"config-missing-path", RuleLevel::Warning,
         Tr::tr("A directory named by the .qdocconf that does not exist")},
        {"config-undefined-variable", RuleLevel::Info,
         Tr::tr("A $variable the .qdocconf expands but nothing defines, which the "
                "documentation build usually passes in")},
    };
    return rules;
}

static QStringList levelNames()
{
    return {Tr::tr("Off"), Tr::tr("Hint"), Tr::tr("Info"), Tr::tr("Warning"), Tr::tr("Error")};
}

static RuleLevel quieter(RuleLevel level)
{
    switch (level) {
    case RuleLevel::Error:
        return RuleLevel::Warning;
    case RuleLevel::Warning:
        return RuleLevel::Info;
    default:
        return RuleLevel::Hint;
    }
}

QDocSettings::QDocSettings()
{
    setSettingsGroup("QDoc");
    setAutoApply(false);

    diagnosticsEnabled.setSettingsKey("DiagnosticsEnabled");
    diagnosticsEnabled.setDefaultValue(true);
    diagnosticsEnabled.setLabelText(Tr::tr("Report markup problems"));

    suppressedByConfig.setSettingsKey("SuppressedByConfig");
    suppressedByConfig.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    suppressedByConfig.setLabelText(Tr::tr("Warnings the .qdocconf silences:"));
    suppressedByConfig.addOption(Tr::tr("Report one level quieter"));
    suppressedByConfig.addOption(Tr::tr("Report as usual"));
    suppressedByConfig.addOption(Tr::tr("Do not report"));
    suppressedByConfig.setDefaultValue(0);

    for (const Rule &rule : ruleTable()) {
        auto level = new SelectionAspect(this);
        level->setSettingsKey(keyFromString("Rule." + rule.id));
        level->setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
        level->setLabelText(rule.id);
        level->setToolTip(rule.title);
        for (const QString &name : levelNames())
            level->addOption(name);
        level->setDefaultValue(int(rule.level));
        m_ruleLevels.insert(rule.id, level);
    }

    // The title of each rule is its tool tip: 25 of them beside the combo boxes made
    // the page wider than the dialog.
    setLayouter([this] {
        using namespace Layouting;
        Form form;
        form.addItem(diagnosticsEnabled);
        form.addItem(br);
        form.addItem(suppressedByConfig);
        form.addItem(br);
        for (const Rule &rule : ruleTable()) {
            form.addItem(m_ruleLevels.value(rule.id));
            form.addItem(br);
        }
        return Column{form, st};
    });

    readSettings();
}

RuleLevel QDocSettings::levelOf(const QString &ruleId) const
{
    SelectionAspect *level = m_ruleLevels.value(ruleId);
    return level ? RuleLevel(level->value()) : RuleLevel::Warning;
}

void QDocSettings::setLevelOf(const QString &ruleId, RuleLevel level)
{
    if (SelectionAspect *aspect = m_ruleLevels.value(ruleId))
        aspect->setValue(int(level));
}

bool QDocSettings::isRuleEnabled(const QString &ruleId) const
{
    return levelOf(ruleId) != RuleLevel::Off;
}

std::optional<RuleLevel> QDocSettings::resolve(const QString &ruleId, bool suppressed) const
{
    RuleLevel level = levelOf(ruleId);
    if (level == RuleLevel::Off)
        return {};
    if (suppressed) {
        if (suppressedByConfig.value() == 2)
            return {};
        if (suppressedByConfig.value() == 0)
            level = quieter(level);
    }
    return level;
}

QDocSettings &settings()
{
    static QDocSettings theSettings;
    return theSettings;
}

// True when a spurious pattern silences the message the way Location::emitMessage()
// does: the match has to cover the whole message.
static bool isSuppressedWarning(const QString &message, const QStringList &patterns)
{
    if (message.isEmpty() || patterns.isEmpty())
        return false;
    QStringList alternatives;
    for (const QString &pattern : patterns) {
        if (!pattern.isEmpty())
            alternatives.append("(?:" + pattern + ')');
    }
    if (alternatives.isEmpty())
        return false;
    const QRegularExpression re(alternatives.join('|'));
    if (!re.isValid())
        return false;
    const QRegularExpressionMatch match = re.match(message);
    return match.hasMatch() && match.capturedStart() == 0
           && match.capturedLength() == message.size();
}

static ProjectExplorer::Task::TaskType taskType(RuleLevel level)
{
    if (level == RuleLevel::Error)
        return ProjectExplorer::Task::Error;
    if (level == RuleLevel::Warning)
        return ProjectExplorer::Task::Warning;
    return ProjectExplorer::Task::Unknown;
}

static QHash<FilePath, QList<ProjectExplorer::Task>> &publishedTasks()
{
    static QHash<FilePath, QList<ProjectExplorer::Task>> tasks;
    return tasks;
}

static void publish(const FilePath &owner, const QList<ProjectExplorer::Task> &tasks)
{
    QList<ProjectExplorer::Task> &previous = publishedTasks()[owner];
    for (const ProjectExplorer::Task &task : previous)
        ProjectExplorer::TaskHub::removeTask(task);
    previous = tasks;
    for (const ProjectExplorer::Task &task : tasks)
        ProjectExplorer::TaskHub::addTask(task);
}

void updateDiagnostics(const FilePath &file,
                       const QList<Problem> &problems,
                       const DocContext &context)
{
    if (!settings().diagnosticsEnabled()) {
        clearDiagnostics(file);
        publish(context.confFile, {});
        return;
    }

    const auto taskFor = [](const QString &message,
                            const QString &ruleId,
                            bool suppressed,
                            bool synthetic,
                            const FilePath &taskFile,
                            int line) -> std::optional<ProjectExplorer::Task> {
        // Text pulled in by a macro or \include has a rule of its own: it is not what
        // the author wrote in this file.
        const QString id = synthetic ? QString("expanded-text") : ruleId;
        const std::optional<RuleLevel> level = settings().resolve(id, suppressed);
        if (!level)
            return {};
        QStringList notes;
        if (synthetic)
            notes.append(Tr::tr("in text pulled in by a macro or \\include"));
        if (suppressed)
            notes.append(Tr::tr("silenced by spurious in the .qdocconf"));
        const QString text = notes.isEmpty()
                                 ? message
                                 : QString("%1 (%2)").arg(message, notes.join("; "));
        ProjectExplorer::Task task(taskType(*level),
                                   QString("%1 [%2]").arg(text, id),
                                   taskFile,
                                   line,
                                   TASK_CATEGORY);
        return task;
    };

    QList<ProjectExplorer::Task> tasks;
    for (const Problem &problem : problems) {
        const std::optional<ProjectExplorer::Task> task
            = taskFor(problem.message,
                      problem.rule,
                      isSuppressedWarning(problem.message, context.spurious),
                      problem.synthetic,
                      file,
                      problem.line);
        if (task)
            tasks.append(*task);
    }
    publish(file, tasks);

    // A problem in the .qdocconf belongs to the configuration file and the line that
    // holds it, not to whatever source file happened to trigger the parse.
    QList<ProjectExplorer::Task> configTasks;
    for (const ConfigProblem &problem : context.problems) {
        const std::optional<ProjectExplorer::Task> task
            = taskFor(problem.message,
                      ruleForMessage(problem.message),
                      isSuppressedWarning(problem.message, context.spurious),
                      false,
                      problem.file,
                      problem.line);
        if (task)
            configTasks.append(*task);
    }
    if (!context.confFile.isEmpty())
        publish(context.confFile, configTasks);
}

void clearDiagnostics(const FilePath &file)
{
    publish(file, {});
    publishedTasks().remove(file);
}

class QDocSettingsPage final : public Core::IOptionsPage
{
public:
    QDocSettingsPage()
    {
        setId("QDoc.Settings");
        setDisplayName(Tr::tr("QDoc"));
        setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &settings(); });
    }
};

void setupQDocLinter()
{
    ProjectExplorer::TaskHub::addCategory(
        {TASK_CATEGORY, Tr::tr("QDoc"), Tr::tr("Problems in QDoc markup."), true});
    static QDocSettingsPage theSettingsPage;
}

} // namespace QDoc::Internal

#ifdef WITH_TESTS

#include <QSignalSpy>
#include <QTest>

namespace QDoc::Internal {

// The rules and their levels are tested through the tasks they produce, since the
// mapping from a problem to a task in the Problems pane is the part that has no other
// coverage.
class QDocLinterTest final : public QObject
{
    Q_OBJECT

private:
    QList<ProjectExplorer::Task> m_added;
    QList<ProjectExplorer::Task> m_removed;
    QHash<QString, int> m_levels;
    bool m_enabled = true;
    int m_suppressed = 0;

    QList<ProjectExplorer::Task> ownTasks() const
    {
        QList<ProjectExplorer::Task> tasks;
        for (const ProjectExplorer::Task &task : m_added) {
            if (task.category() == Utils::Id(TASK_CATEGORY))
                tasks.append(task);
        }
        return tasks;
    }

    static Problem problem(const QString &message, const QString &rule, int line)
    {
        Problem result;
        result.message = message;
        result.rule = rule;
        result.line = line;
        return result;
    }

private slots:
    void initTestCase()
    {
        // The settings are the user's, so whatever the test changes is put back.
        m_enabled = settings().diagnosticsEnabled();
        m_suppressed = settings().suppressedByConfig();
        for (const Rule &rule : ruleTable())
            m_levels.insert(rule.id, int(settings().levelOf(rule.id)));

        connect(&ProjectExplorer::taskHub(),
                &ProjectExplorer::TaskHub::taskAdded,
                this,
                [this](const ProjectExplorer::Task &task) { m_added.append(task); });
        connect(&ProjectExplorer::taskHub(),
                &ProjectExplorer::TaskHub::taskRemoved,
                this,
                [this](const ProjectExplorer::Task &task) { m_removed.append(task); });
    }

    void cleanupTestCase()
    {
        settings().diagnosticsEnabled.setValue(m_enabled);
        settings().suppressedByConfig.setValue(m_suppressed);
        for (auto it = m_levels.cbegin(); it != m_levels.cend(); ++it)
            settings().setLevelOf(it.key(), RuleLevel(it.value()));
    }

    void init()
    {
        m_added.clear();
        m_removed.clear();
        settings().diagnosticsEnabled.setValue(true);
        settings().suppressedByConfig.setValue(0);
    }

    void cleanup() { clearDiagnostics(file()); }

    static Utils::FilePath file() { return Utils::FilePath::fromString("/tmp/qdoc-test.qdoc"); }

    // A problem becomes a task on its own line, with the severity its rule is set to
    // and the rule id in the description.
    void testDiagnostics()
    {
        settings().setLevelOf("unknown-command", RuleLevel::Warning);
        settings().setLevelOf("unterminated-block", RuleLevel::Error);
        settings().setLevelOf("brief-punctuation", RuleLevel::Info);

        updateDiagnostics(file(),
                          {problem("Unknown command '\\macOS'", "unknown-command", 7),
                           problem("Missing '\\endlist'", "unterminated-block", 11),
                           problem("'\\brief' statement does not end with a full stop.",
                                   "brief-punctuation",
                                   5)},
                          {});

        const QList<ProjectExplorer::Task> tasks = ownTasks();
        QCOMPARE(tasks.size(), 3);
        QCOMPARE(tasks.at(0).line(), 7);
        QCOMPARE(tasks.at(0).file(), file());
        QCOMPARE(tasks.at(0).type(), ProjectExplorer::Task::Warning);
        QVERIFY(tasks.at(0).description().contains("[unknown-command]"));
        QCOMPARE(tasks.at(1).type(), ProjectExplorer::Task::Error);
        // Info and hint have no task type of their own, and no mark in the editor.
        QCOMPARE(tasks.at(2).type(), ProjectExplorer::Task::Unknown);
    }

    // Rendering again must replace what the previous render reported, not add to it.
    void testRepeatedUpdate()
    {
        updateDiagnostics(file(), {problem("Unknown command '\\a'", "unknown-command", 1)}, {});
        QCOMPARE(ownTasks().size(), 1);
        const unsigned int first = ownTasks().first().id();

        m_added.clear();
        updateDiagnostics(file(), {problem("Unknown command '\\b'", "unknown-command", 2)}, {});
        QCOMPARE(ownTasks().size(), 1);
        QCOMPARE(m_removed.size(), 1);
        QCOMPARE(m_removed.first().id(), first);

        m_removed.clear();
        clearDiagnostics(file());
        QCOMPARE(m_removed.size(), 1);
    }

    void testRuleLevels()
    {
        settings().setLevelOf("unknown-command", RuleLevel::Off);
        updateDiagnostics(file(), {problem("Unknown command '\\a'", "unknown-command", 1)}, {});
        QCOMPARE(ownTasks().size(), 0);

        settings().setLevelOf("unknown-command", RuleLevel::Error);
        updateDiagnostics(file(), {problem("Unknown command '\\a'", "unknown-command", 1)}, {});
        QCOMPARE(ownTasks().size(), 1);
        QCOMPARE(ownTasks().first().type(), ProjectExplorer::Task::Error);

        // Nothing is reported at all while the checks are switched off.
        settings().diagnosticsEnabled.setValue(false);
        m_added.clear();
        updateDiagnostics(file(), {problem("Unknown command '\\a'", "unknown-command", 1)}, {});
        QCOMPARE(ownTasks().size(), 0);
    }

    // Text a macro or \include brought in is not what the author wrote here, so it has
    // a rule of its own, off by default.
    void testExpandedText()
    {
        settings().setLevelOf("unknown-command", RuleLevel::Warning);
        settings().setLevelOf("expanded-text", RuleLevel::Off);
        Problem synthetic = problem("Unknown command '\\a'", "unknown-command", 3);
        synthetic.synthetic = true;
        updateDiagnostics(file(), {synthetic}, {});
        QCOMPARE(ownTasks().size(), 0);

        settings().setLevelOf("expanded-text", RuleLevel::Warning);
        updateDiagnostics(file(), {synthetic}, {});
        QCOMPARE(ownTasks().size(), 1);
        QVERIFY(ownTasks().first().description().contains("[expanded-text]"));
        QVERIFY(ownTasks().first().description().contains("pulled in by a macro"));
    }

    // A warning the .qdocconf silences is worth seeing, one level quieter and labelled.
    void testSuppressedByConfig()
    {
        settings().setLevelOf("sa-missing-comma", RuleLevel::Warning);
        DocContext context;
        context.spurious = {"Missing comma in .*"};

        updateDiagnostics(file(),
                          {problem("Missing comma in '\\sa'", "sa-missing-comma", 4)},
                          context);
        QCOMPARE(ownTasks().size(), 1);
        QCOMPARE(ownTasks().first().type(), ProjectExplorer::Task::Unknown);
        QVERIFY(ownTasks().first().description().contains("silenced by spurious"));

        settings().suppressedByConfig.setValue(2); // Do not report
        m_added.clear();
        updateDiagnostics(file(),
                          {problem("Missing comma in '\\sa'", "sa-missing-comma", 4)},
                          context);
        QCOMPARE(ownTasks().size(), 0);

        settings().suppressedByConfig.setValue(1); // Report as usual
        m_added.clear();
        updateDiagnostics(file(),
                          {problem("Missing comma in '\\sa'", "sa-missing-comma", 4)},
                          context);
        QCOMPARE(ownTasks().size(), 1);
        QCOMPARE(ownTasks().first().type(), ProjectExplorer::Task::Warning);
    }

    // A problem in the configuration belongs to the configuration file and the line
    // that holds it, not to the document that happened to trigger the parse.
    void testConfigProblems()
    {
        settings().setLevelOf("config-missing-path", RuleLevel::Warning);
        const Utils::FilePath conf = Utils::FilePath::fromString("/tmp/qdoc-test.qdocconf");
        DocContext context;
        context.confFile = conf;
        context.problems = {{"Cannot find file or directory: images", conf, 12}};

        updateDiagnostics(file(), {}, context);
        const QList<ProjectExplorer::Task> tasks = ownTasks();
        QCOMPARE(tasks.size(), 1);
        QCOMPARE(tasks.first().file(), conf);
        QCOMPARE(tasks.first().line(), 12);

        clearDiagnostics(conf);
    }
};

QObject *createQDocLinterTest()
{
    return new QDocLinterTest;
}

} // namespace QDoc::Internal

#include "qdoclinter.moc"

#endif // WITH_TESTS
