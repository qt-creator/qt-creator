// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitgrep.h"

#include "gitclient.h"
#include "gittr.h"

#include <coreplugin/vcsmanager.h>

#include <texteditor/findinfiles.h>

#include <vcsbase/vcsbaseconstants.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/fancylineedit.h>
#include <utils/filesearch.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QFuture>
#include <QHBoxLayout>
#include <QRegularExpressionValidator>
#include <QSettings>

using namespace Core;
using namespace Utils;
using namespace VcsBase;

namespace Git::Internal {

const char GitGrepRef[] = "GitGrepRef";

class GitGrepParameters
{
public:
    QString ref;
    bool recurseSubmodules = false;
    QString id() const { return recurseSubmodules ? ref + ".Rec" : ref; }
};

static QStringView nextLine(QStringView *remainingInput)
{
    const int newLinePos = remainingInput->indexOf('\n');
    if (newLinePos < 0) {
        QStringView ret = *remainingInput;
        *remainingInput = QStringView();
        return ret;
    }
    QStringView ret = remainingInput->left(newLinePos);
    *remainingInput = remainingInput->mid(newLinePos + 1);
    return ret;
}

class GitGrepRunner
{
public:
    GitGrepRunner(const TextEditor::FileFindParameters &parameters)
        : m_parameters(parameters)
    {
        m_directory = FilePath::fromString(parameters.additionalParameters.toString());
        m_vcsBinary = GitClient::instance()->vcsBinary();
        m_environment = GitClient::instance()->processEnvironment();
        if (m_parameters.flags & FindRegularExpression) {
            const QRegularExpression::PatternOptions patternOptions
                = m_parameters.flags & FindCaseSensitively
                ? QRegularExpression::NoPatternOption
                : QRegularExpression::CaseInsensitiveOption;
            m_regexp.setPattern(m_parameters.text);
            m_regexp.setPatternOptions(patternOptions);
        }
    }

    struct Match
    {
        Match() = default;
        Match(int start, int length) :
            matchStart(start), matchLength(length) {}

        int matchStart = 0;
        int matchLength = 0;
        QStringList regexpCapturedTexts;
    };

    void processLine(QStringView line, SearchResultItems *resultList) const
    {
        if (line.isEmpty())
            return;
        static const QLatin1String boldRed("\x1b[1;31m");
        static const QLatin1String resetColor("\x1b[m");
        SearchResultItem result;
        const int lineSeparator = line.indexOf(QChar::Null);
        QStringView filePath = line.left(lineSeparator);
        if (!m_ref.isEmpty() && filePath.startsWith(m_ref))
            filePath = filePath.mid(m_ref.length());
        result.setFilePath(m_directory.pathAppended(filePath.toString()));
        const int textSeparator = line.indexOf(QChar::Null, lineSeparator + 1);
        const int lineNumber = line.mid(lineSeparator + 1, textSeparator - lineSeparator - 1).toInt();
        QString text = line.mid(textSeparator + 1).toString();
        QList<Match> matches;
        while (true) {
            const int matchStart = text.indexOf(boldRed);
            if (matchStart == -1)
                break;
            const int matchTextStart = matchStart + boldRed.size();
            const int matchEnd = text.indexOf(resetColor, matchTextStart);
            QTC_ASSERT(matchEnd != -1, break);
            const int matchLength = matchEnd - matchTextStart;
            Match match(matchStart, matchLength);
            const QString matchText = text.mid(matchTextStart, matchLength);
            if (m_parameters.flags & FindRegularExpression)
                match.regexpCapturedTexts = m_regexp.match(matchText).capturedTexts();
            matches.append(match);
            text = text.left(matchStart) + matchText + text.mid(matchEnd + resetColor.size());
        }
        result.setDisplayText(text);

        for (const auto &match : std::as_const(matches)) {
            result.setMainRange(lineNumber, match.matchStart, match.matchLength);
            result.setUserData(match.regexpCapturedTexts);
            result.setUseTextEditorFont(true);
            resultList->append(result);
        }
    }

    int read(QPromise<SearchResultItems> &promise, const QString &input)
    {
        SearchResultItems items;
        QStringView remainingInput(input);
        while (true) {
            if (promise.isCanceled())
                return 0;

            if (remainingInput.isEmpty())
                break;

            const QStringView line = nextLine(&remainingInput);
            if (line.isEmpty())
                continue;

            processLine(line, &items);
        }
        if (!items.isEmpty())
            promise.addResult(items);
        return items.count();
    }

    void operator()(QPromise<SearchResultItems> &promise)
    {
        QStringList arguments = {
            "-c", "color.grep.match=bold red",
            "-c", "color.grep=always",
            "-c", "color.grep.filename=",
            "-c", "color.grep.lineNumber=",
            "grep", "-zn", "--no-full-name"
        };
        if (!(m_parameters.flags & FindCaseSensitively))
            arguments << "-i";
        if (m_parameters.flags & FindWholeWords)
            arguments << "-w";
        if (m_parameters.flags & FindRegularExpression)
            arguments << "-P";
        else
            arguments << "-F";
        arguments << "-e" << m_parameters.text;
        GitGrepParameters params = m_parameters.searchEngineParameters.value<GitGrepParameters>();
        if (params.recurseSubmodules)
            arguments << "--recurse-submodules";
        if (!params.ref.isEmpty()) {
            arguments << params.ref;
            m_ref = params.ref + ':';
        }
        const QStringList filterArgs =
                m_parameters.nameFilters.isEmpty() ? QStringList("*") // needed for exclusion filters
                                                   : m_parameters.nameFilters;
        const QStringList exclusionArgs =
                Utils::transform(m_parameters.exclusionFilters, [](const QString &filter) {
                    return QString(":!" + filter);
                });
        arguments << "--" << filterArgs << exclusionArgs;

        QEventLoop loop;

        Process process;
        process.setEnvironment(m_environment);
        process.setCommand({m_vcsBinary, arguments});
        process.setWorkingDirectory(m_directory);
        QStringList outputBuffer;
        // The states transition exactly in this order:
        enum State { BelowLimit, AboveLimit, Paused, Resumed };
        State state = BelowLimit;
        int reportedResultsCount = 0;
        process.setStdOutCallback([this, &process, &loop, &promise, &state, &reportedResultsCount,
                                   &outputBuffer](const QString &output) {
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
                reportedResultsCount += read(promise, output);
                if (state == BelowLimit && reportedResultsCount > 200000)
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
                         [this, &process, &loop, &promise, &state, &outputBuffer] {
            state = Resumed;
            for (const QString &output : outputBuffer) {
                if (promise.isCanceled()) {
                    process.close();
                    loop.quit();
                }
                read(promise, output);
            }
            outputBuffer.clear();
            if (process.state() == QProcess::NotRunning)
                loop.quit();
        });
        watcher.setFuture(future);
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }

private:
    FilePath m_vcsBinary;
    FilePath m_directory;
    QString m_ref;
    TextEditor::FileFindParameters m_parameters;
    Environment m_environment;
    QRegularExpression m_regexp;
};

static bool isGitDirectory(const FilePath &path)
{
    static IVersionControl *gitVc = VcsManager::versionControl(VcsBase::Constants::VCS_ID_GIT);
    QTC_ASSERT(gitVc, return false);
    return gitVc == VcsManager::findVersionControlForDirectory(path);
}

GitGrep::GitGrep(GitClient *client)
    : m_client(client)
{
    m_widget = new QWidget;
    auto layout = new QHBoxLayout(m_widget);
    layout->setContentsMargins(0, 0, 0, 0);
    m_treeLineEdit = new FancyLineEdit;
    m_treeLineEdit->setPlaceholderText(Tr::tr("Tree (optional)"));
    m_treeLineEdit->setToolTip(Tr::tr("Can be HEAD, tag, local or remote branch, or a commit hash.\n"
                                  "Leave empty to search through the file system."));
    const QRegularExpression refExpression("[\\S]*");
    m_treeLineEdit->setValidator(new QRegularExpressionValidator(refExpression, this));
    layout->addWidget(m_treeLineEdit);
    // asynchronously check git version, add "recurse submodules" option if available
    Utils::onResultReady(client->gitVersion(),
                         this,
                         [this, pLayout = QPointer<QHBoxLayout>(layout)](unsigned version) {
                             if (version >= 0x021300 && pLayout) {
                                 m_recurseSubmodules = new QCheckBox(Tr::tr("Recurse submodules"));
                                 pLayout->addWidget(m_recurseSubmodules);
                             }
                         });
    TextEditor::FindInFiles *findInFiles = TextEditor::FindInFiles::instance();
    QTC_ASSERT(findInFiles, return);
    connect(findInFiles, &TextEditor::FindInFiles::pathChanged,
            m_widget, [this](const FilePath &path) {
        setEnabled(isGitDirectory(path));
    });
    connect(this, &SearchEngine::enabledChanged, m_widget, &QWidget::setEnabled);
    findInFiles->addSearchEngine(this);
}

GitGrep::~GitGrep()
{
    delete m_widget;
}

QString GitGrep::title() const
{
    return Tr::tr("Git Grep");
}

QString GitGrep::toolTip() const
{
    const QString ref = m_treeLineEdit->text();
    if (!ref.isEmpty())
        return Tr::tr("Ref: %1\n%2").arg(ref);
    return QLatin1String("%1");
}

QWidget *GitGrep::widget() const
{
    return m_widget;
}

QVariant GitGrep::parameters() const
{
    GitGrepParameters params;
    params.ref = m_treeLineEdit->text();
    if (m_recurseSubmodules)
        params.recurseSubmodules = m_recurseSubmodules->isChecked();
    return QVariant::fromValue(params);
}

void GitGrep::readSettings(QSettings *settings)
{
    m_treeLineEdit->setText(settings->value(GitGrepRef).toString());
}

void GitGrep::writeSettings(QSettings *settings) const
{
    settings->setValue(GitGrepRef, m_treeLineEdit->text());
}

QFuture<SearchResultItems> GitGrep::executeSearch(const TextEditor::FileFindParameters &parameters,
        TextEditor::BaseFileFind * /*baseFileFind*/)
{
    return Utils::asyncRun(GitGrepRunner(parameters));
}

IEditor *GitGrep::openEditor(const SearchResultItem &item,
                             const TextEditor::FileFindParameters &parameters)
{
    const GitGrepParameters params = parameters.searchEngineParameters.value<GitGrepParameters>();
    const QStringList &itemPath = item.path();
    if (params.ref.isEmpty() || itemPath.isEmpty())
        return nullptr;
    const FilePath path = FilePath::fromUserInput(itemPath.first());
    const FilePath topLevel = FilePath::fromString(parameters.additionalParameters.toString());
    IEditor *editor = m_client->openShowEditor(topLevel, params.ref, path,
                                               GitClient::ShowEditor::OnlyIfDifferent);
    if (editor)
        editor->gotoLine(item.mainRange().begin.line, item.mainRange().begin.column);
    return editor;
}

} // Git::Internal

Q_DECLARE_METATYPE(Git::Internal::GitGrepParameters)
