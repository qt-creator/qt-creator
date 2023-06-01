// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findinfilessilversearcher.h"
#include "silversearcherparser.h"
#include "silversearchertr.h"

#include <texteditor/findinfiles.h>
#include <utils/async.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>

using namespace Core;
using namespace SilverSearcher;
using namespace TextEditor;
using namespace Utils;

namespace {
const QLatin1String s_metaCharacters = QLatin1String("+()^$.{}[]|\\");
const QLatin1String s_searchOptionsString = QLatin1String("SearchOptionsString");

class SilverSearcherSearchOptions
{
public:
    QString searchOptions;
};

static QString convertWildcardToRegex(const QString &wildcard)
{
    QString regex;
    const int wildcardSize = wildcard.size();
    regex.append('^');
    for (int i = 0; i < wildcardSize; ++i) {
        const QChar ch = wildcard[i];
        if (ch == '*')
            regex.append(".*");
        else if (ch == '?')
            regex.append('.');
        else if (s_metaCharacters.indexOf(ch) != -1)
            regex.append('\\' + ch);
        else
            regex.append(ch);
    }
    regex.append('$');
    return regex;
}

static bool isSilverSearcherAvailable()
{
    Process silverSearcherProcess;
    silverSearcherProcess.setCommand({"ag", {"--version"}});
    silverSearcherProcess.start();
    return silverSearcherProcess.waitForFinished(1000)
        && silverSearcherProcess.cleanedStdOut().contains("ag version");
}

static std::optional<QRegularExpression> regExpFromParameters(const FileFindParameters &parameters)
{
    if (!(parameters.flags & FindRegularExpression))
        return {};

    const QRegularExpression::PatternOptions patternOptions
        = (parameters.flags & FindCaseSensitively)
              ? QRegularExpression::NoPatternOption
              : QRegularExpression::CaseInsensitiveOption;
    QRegularExpression regExp;
    regExp.setPattern(parameters.text);
    regExp.setPatternOptions(patternOptions);
    return regExp;
}

static void runSilverSeacher(QPromise<SearchResultItems> &promise,
                             const FileFindParameters &parameters)
{
    const FilePath directory = FilePath::fromUserInput(parameters.additionalParameters.toString());
    QStringList arguments = {"--parallel", "--ackmate"};

    if (parameters.flags & FindCaseSensitively)
        arguments << "-s";
    else
        arguments << "-i";

    if (parameters.flags & FindWholeWords)
        arguments << "-w";

    if (!(parameters.flags & FindRegularExpression))
        arguments << "-Q";

    for (const QString &filter : std::as_const(parameters.exclusionFilters))
        arguments << "--ignore" << filter;

    QString nameFiltersAsRegExp;
    for (const QString &filter : std::as_const(parameters.nameFilters))
        nameFiltersAsRegExp += QString("(%1)|").arg(convertWildcardToRegex(filter));
    nameFiltersAsRegExp.remove(nameFiltersAsRegExp.length() - 1, 1);

    arguments << "-G" << nameFiltersAsRegExp;

    const SilverSearcherSearchOptions params = parameters.searchEngineParameters
                                                   .value<SilverSearcherSearchOptions>();
    if (!params.searchOptions.isEmpty())
        arguments << params.searchOptions.split(' ');

    arguments << "--" << parameters.text << directory.normalizedPathName().toString();

    QEventLoop loop;

    Process process;
    process.setCommand({"ag", arguments});
    ParserState parserState;
    const std::optional<QRegularExpression> regExp = regExpFromParameters(parameters);
    QStringList outputBuffer;
    // The states transition exactly in this order:
    enum State { BelowLimit, AboveLimit, Paused, Resumed };
    State state = BelowLimit;
    process.setStdOutCallback([&process, &loop, &promise, &state, &outputBuffer, &parserState,
                               regExp](const QString &output) {
        if (promise.isCanceled()) {
            process.close();
            loop.quit();
            return;
        }
        // The SearchResultWidget is going to pause the search anyway, so start buffering
        // the output.
        if (state == AboveLimit || state == Paused) {
            outputBuffer.append(output);
        } else {
            SilverSearcher::parse(promise, output, &parserState, regExp);
            if (state == BelowLimit && parserState.m_reportedResultsCount > 200000)
                state = AboveLimit;
        }
    });
    QObject::connect(&process, &Process::done, &loop, [&loop, &promise, &state] {
        if (state == BelowLimit || state == Resumed || promise.isCanceled())
            loop.quit();
    });

    process.start();
    if (process.state() == QProcess::NotRunning)
        return;

    QFutureWatcher<void> watcher;
    QFuture<void> future(promise.future());
    QObject::connect(&watcher, &QFutureWatcherBase::canceled, &loop, [&process, &loop] {
        process.close();
        loop.quit();
    });
    QObject::connect(&watcher, &QFutureWatcherBase::paused, &loop, [&state] { state = Paused; });
    QObject::connect(&watcher, &QFutureWatcherBase::resumed, &loop,
                     [&process, &loop, &promise, &state, &outputBuffer, &parserState, regExp] {
        state = Resumed;
        for (const QString &output : outputBuffer) {
            if (promise.isCanceled()) {
                process.close();
                loop.quit();
            }
            SilverSearcher::parse(promise, output, &parserState, regExp);
        }
        outputBuffer.clear();
    });
    watcher.setFuture(future);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

} // namespace

Q_DECLARE_METATYPE(SilverSearcherSearchOptions)

namespace SilverSearcher {

FindInFilesSilverSearcher::FindInFilesSilverSearcher(QObject *parent)
    : SearchEngine(parent)
    , m_path("ag")
    , m_toolName("SilverSearcher")
{
    m_widget = new QWidget;
    auto layout = new QHBoxLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    m_searchOptionsLineEdit = new QLineEdit;
    m_searchOptionsLineEdit->setPlaceholderText(Tr::tr("Search Options (optional)"));
    layout->addWidget(m_searchOptionsLineEdit);

    FindInFiles *findInFiles = FindInFiles::instance();
    QTC_ASSERT(findInFiles, return);
    findInFiles->addSearchEngine(this);

    // TODO: Make disabled by default and run isSilverSearcherAvailable asynchronously
    setEnabled(isSilverSearcherAvailable());
    if (!isEnabled()) {
        QLabel *label = new QLabel(Tr::tr("Silver Searcher is not available on the system."));
        label->setStyleSheet("QLabel { color : red; }");
        layout->addWidget(label);
    }
}

QVariant FindInFilesSilverSearcher::parameters() const
{
    SilverSearcherSearchOptions silverSearcherSearchOptions;
    silverSearcherSearchOptions.searchOptions = m_searchOptionsLineEdit->text();
    return QVariant::fromValue(silverSearcherSearchOptions);
}

QString FindInFilesSilverSearcher::title() const
{
    return "Silver Searcher";
}

QString FindInFilesSilverSearcher::toolTip() const
{
    return {};
}

QWidget *FindInFilesSilverSearcher::widget() const
{
    return m_widget;
}

void FindInFilesSilverSearcher::writeSettings(QSettings *settings) const
{
    settings->setValue(s_searchOptionsString, m_searchOptionsLineEdit->text());
}

QFuture<SearchResultItems> FindInFilesSilverSearcher::executeSearch(
        const FileFindParameters &parameters, BaseFileFind * /*baseFileFind*/)
{
    return Utils::asyncRun(runSilverSeacher, parameters);
}

IEditor *FindInFilesSilverSearcher::openEditor(const SearchResultItem & /*item*/,
                                               const FileFindParameters & /*parameters*/)
{
    return nullptr;
}

void FindInFilesSilverSearcher::readSettings(QSettings *settings)
{
    m_searchOptionsLineEdit->setText(settings->value(s_searchOptionsString).toString());
}

} // namespace SilverSearcher
