// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "spotlightlocatorfilter.h"

#include "../coreplugintr.h"
#include "../messagemanager.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/fancylineedit.h>
#include <utils/link.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QFormLayout>
#include <QJsonObject>
#include <QRegularExpression>

using namespace Utils;

namespace Core::Internal {

static QString defaultCommand()
{
    if (HostOsInfo::isMacHost())
        return "mdfind";
    if (HostOsInfo::isWindowsHost())
        return "es.exe";
    return "locate";
}

/*
    For the tools es [1] and locate [2], interpret space as AND operator.

    Currently doesn't support fine picking a file with a space in the path by escaped space.

    [1]: https://www.voidtools.com/support/everything/command_line_interface/
    [2]: https://www.gnu.org/software/findutils/manual/html_node/find_html/Invoking-locate.html
 */

static QString defaultArguments(Qt::CaseSensitivity sens = Qt::CaseInsensitive)
{
    if (HostOsInfo::isMacHost())
        return QString("\"kMDItemFSName = '*%{Query:EscapedWithWildcards}*'%1\"")
            .arg(sens == Qt::CaseInsensitive ? QString("c") : "");
    if (HostOsInfo::isWindowsHost())
        return QString("%1 -n 10000 %{Query:Escaped}")
            .arg(sens == Qt::CaseInsensitive ? QString() : "-i ");
    return QString("%1 -A -l 10000 %{Query:Escaped}")
        .arg(sens == Qt::CaseInsensitive ? QString() : "-i ");
}

const bool kSortResultsDefault = true;

const char kCommandKey[] = "command";
const char kArgumentsKey[] = "arguments";
const char kCaseSensitiveKey[] = "caseSensitive";
const char kSortResultsKey[] = "sortResults";

static QString escaped(const QString &query)
{
    QString quoted = query;
    quoted.replace('\\', "\\\\").replace('\'', "\\\'").replace('\"', "\\\"");
    return quoted;
}

static MacroExpander *createMacroExpander(const QString &query)
{
    MacroExpander *expander = new MacroExpander;
    expander->registerVariable("Query",
                               Tr::tr("Locator query string."),
                               [query] { return query; });
    expander->registerVariable("Query:Escaped",
                               Tr::tr("Locator query string with quotes escaped with backslash."),
                               [query] { return escaped(query); });
    expander->registerVariable("Query:EscapedWithWildcards",
                               Tr::tr("Locator query string with quotes escaped with backslash and "
                                      "spaces replaced with \"*\" wildcards."),
                               [query] {
                                   QString quoted = escaped(query);
                                   quoted.replace(' ', '*');
                                   return quoted;
                               });
    expander->registerVariable("Query:Regex",
                               Tr::tr("Locator query string as regular expression."),
                               [query] {
                                   QString regex = query;
                                   regex = regex.replace('*', ".*");
                                   regex = regex.replace(' ', ".*");
                                   return regex;
                               });
    return expander;
}

SpotlightLocatorFilter::SpotlightLocatorFilter()
{
    setId("SpotlightFileNamesLocatorFilter");
    setDefaultShortcutString("md");
    setDisplayName(Tr::tr("File Name Index"));
    setDescription(Tr::tr(
        "Locates files from a global file system index (Spotlight, Locate, Everything). Append "
        "\"+<number>\" or \":<number>\" to jump to the given line number. Append another "
        "\"+<number>\" or \":<number>\" to jump to the column number as well."));
    m_command = defaultCommand();
    m_arguments = defaultArguments();
    m_caseSensitiveArguments = defaultArguments(Qt::CaseSensitive);
}

static void matches(QPromise<void> &promise,
                    const LocatorStorage &storage,
                    const CommandLine &command,
                    bool sortResults)
{
    // If search string contains spaces, treat them as wildcard '*' and search in full path
    const QString wildcardInput = QDir::fromNativeSeparators(storage.input()).replace(' ', '*');
    const Link inputLink = Link::fromString(wildcardInput, true);
    const QString newInput = inputLink.targetFilePath.toString();
    const QRegularExpression regExp = ILocatorFilter::createRegExp(newInput);
    if (!regExp.isValid())
        return;

    const bool hasPathSeparator = newInput.contains('/') || newInput.contains('*');
    LocatorFileCache::MatchedEntries entries = {};
    QEventLoop loop;
    Process process;
    process.setCommand(command);
    process.setEnvironment(Environment::systemEnvironment()); // TODO: Is it needed?

    QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                     [&, entriesPtr = &entries] {
        QString output = process.readAllStandardOutput();
        output.replace("\r\n", "\n");
        const QStringList items = output.split('\n');
        const FilePaths filePaths = Utils::transform(items, &FilePath::fromUserInput);
        LocatorFileCache::processFilePaths(promise.future(), filePaths, hasPathSeparator, regExp,
                                           inputLink, entriesPtr);
        if (promise.isCanceled())
            loop.exit();
    });
    QObject::connect(&process, &Process::done, &process, [&] {
        if (process.result() != ProcessResult::FinishedWithSuccess) {
            MessageManager::writeFlashing(Tr::tr("Locator: Error occurred when running \"%1\".")
                                              .arg(command.executable().toUserOutput()));
        }
        loop.exit();
    });
    QFutureWatcher<void> watcher;
    watcher.setFuture(promise.future());
    QObject::connect(&watcher, &QFutureWatcherBase::canceled, &watcher, [&loop] { loop.exit(); });
    if (promise.isCanceled())
        return;
    process.start();
    loop.exec();

    if (sortResults) {
        for (auto &entry : entries) {
            if (promise.isCanceled())
                return;
            if (entry.size() < 1000)
                Utils::sort(entry, LocatorFilterEntry::compareLexigraphically);
        }
    }
    if (promise.isCanceled())
        return;
    storage.reportOutput(std::accumulate(std::begin(entries), std::end(entries),
                                         LocatorFilterEntries()));
}

LocatorMatcherTasks SpotlightLocatorFilter::matchers()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage,
                          command = m_command,
                          insensArgs = m_arguments,
                          sensArgs = m_caseSensitiveArguments,
                          sortResults = m_sortResults](Async<void> &async) {
        const Link link = Link::fromString(storage->input(), true);
        const FilePath input = link.targetFilePath;
        if (input.isEmpty())
            return SetupResult::StopWithDone;

        // only pass the file name part to allow searches like "somepath/*foo"
        const std::unique_ptr<MacroExpander> expander(createMacroExpander(input.fileName()));
        const QString args = caseSensitivity(input.toString()) == Qt::CaseInsensitive
                           ? insensArgs : sensArgs;
        const CommandLine cmd(FilePath::fromString(command), expander->expand(args),
                              CommandLine::Raw);
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(matches, *storage, cmd, sortResults);
        return SetupResult::Continue;
    };

    return {{AsyncTask<void>(onSetup), storage}};
}

bool SpotlightLocatorFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    QWidget configWidget;
    QFormLayout *layout = new QFormLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    configWidget.setLayout(layout);
    PathChooser *commandEdit = new PathChooser;
    commandEdit->setExpectedKind(PathChooser::ExistingCommand);
    commandEdit->lineEdit()->setText(m_command);
    FancyLineEdit *argumentsEdit = new FancyLineEdit;
    argumentsEdit->setText(m_arguments);
    FancyLineEdit *caseSensitiveArgumentsEdit = new FancyLineEdit;
    caseSensitiveArgumentsEdit->setText(m_caseSensitiveArguments);
    auto sortResults = new QCheckBox(Tr::tr("Sort results"));
    sortResults->setChecked(m_sortResults);
    layout->addRow(Tr::tr("Executable:"), commandEdit);
    layout->addRow(Tr::tr("Arguments:"), argumentsEdit);
    layout->addRow(Tr::tr("Case sensitive:"), caseSensitiveArgumentsEdit);
    layout->addRow({}, sortResults);
    std::unique_ptr<MacroExpander> expander(createMacroExpander(""));
    auto chooser = new VariableChooser(&configWidget);
    chooser->addMacroExpanderProvider([expander = expander.get()] { return expander; });
    chooser->addSupportedWidget(argumentsEdit);
    chooser->addSupportedWidget(caseSensitiveArgumentsEdit);
    const bool accepted = ILocatorFilter::openConfigDialog(parent, &configWidget);
    if (accepted) {
        m_command = commandEdit->rawFilePath().toString();
        m_arguments = argumentsEdit->text();
        m_caseSensitiveArguments = caseSensitiveArgumentsEdit->text();
        m_sortResults = sortResults->isChecked();
    }
    return accepted;
}

void SpotlightLocatorFilter::saveState(QJsonObject &obj) const
{
    if (m_command != defaultCommand())
        obj.insert(kCommandKey, m_command);
    if (m_arguments != defaultArguments())
        obj.insert(kArgumentsKey, m_arguments);
    if (m_caseSensitiveArguments != defaultArguments(Qt::CaseSensitive))
        obj.insert(kCaseSensitiveKey, m_caseSensitiveArguments);
    if (m_sortResults != kSortResultsDefault)
        obj.insert(kSortResultsKey, m_sortResults);
}

void SpotlightLocatorFilter::restoreState(const QJsonObject &obj)
{
    m_command = obj.value(kCommandKey).toString(defaultCommand());
    m_arguments = obj.value(kArgumentsKey).toString(defaultArguments());
    m_caseSensitiveArguments = obj.value(kCaseSensitiveKey).toString(defaultArguments(Qt::CaseSensitive));
    m_sortResults = obj.value(kSortResultsKey).toBool(kSortResultsDefault);
}

} // namespace Core::Internal
