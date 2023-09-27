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
#include <QHBoxLayout>
#include <QRegularExpressionValidator>

using namespace Core;
using namespace TextEditor;
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

struct Match
{
    Match() = default;
    Match(int start, int length) :
        matchStart(start), matchLength(length) {}

    int matchStart = 0;
    int matchLength = 0;
    QStringList regexpCapturedTexts;
};

static void processLine(QStringView line, SearchResultItems *resultList,
                        const std::optional<QRegularExpression> &regExp, const QString &ref,
                        const FilePath &directory)
{
    if (line.isEmpty())
        return;
    static const QLatin1String boldRed("\x1b[1;31m");
    static const QLatin1String resetColor("\x1b[m");
    SearchResultItem result;
    const int lineSeparator = line.indexOf(QChar::Null);
    QStringView filePath = line.left(lineSeparator);
    if (!ref.isEmpty() && filePath.startsWith(ref))
        filePath = filePath.mid(ref.length());
    result.setFilePath(directory.pathAppended(filePath.toString()));
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
        if (regExp)
            match.regexpCapturedTexts = regExp->match(matchText).capturedTexts();
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

static SearchResultItems parse(const QFuture<void> &future, const QString &input,
                               const std::optional<QRegularExpression> &regExp, const QString &ref,
                               const FilePath &directory)
{
    SearchResultItems items;
    QStringView remainingInput(input);
    while (true) {
        if (future.isCanceled())
            return {};

        if (remainingInput.isEmpty())
            break;

        const QStringView line = nextLine(&remainingInput);
        if (line.isEmpty())
            continue;

        processLine(line, &items, regExp, ref, directory);
    }
    return items;
}

static void runGitGrep(QPromise<SearchResultItems> &promise, const FileFindParameters &parameters,
                       const GitGrepParameters &gitParameters)
{
    const auto setupProcess = [&parameters, gitParameters](Process &process) {
        const FilePath vcsBinary = gitClient().vcsBinary();
        const Environment environment = gitClient().processEnvironment();

        QStringList arguments = {
            "-c", "color.grep.match=bold red",
            "-c", "color.grep=always",
            "-c", "color.grep.filename=",
            "-c", "color.grep.lineNumber=",
            "grep", "-zn", "--no-full-name"
        };
        if (!(parameters.flags & FindCaseSensitively))
            arguments << "-i";
        if (parameters.flags & FindWholeWords)
            arguments << "-w";
        if (parameters.flags & FindRegularExpression)
            arguments << "-P";
        else
            arguments << "-F";
        arguments << "-e" << parameters.text;
        if (gitParameters.recurseSubmodules)
            arguments << "--recurse-submodules";
        if (!gitParameters.ref.isEmpty()) {
            arguments << gitParameters.ref;
        }
        const QStringList filterArgs =
            parameters.nameFilters.isEmpty() ? QStringList("*") // needed for exclusion filters
                                               : parameters.nameFilters;
        const QStringList exclusionArgs =
            Utils::transform(parameters.exclusionFilters, [](const QString &filter) {
                return QString(":!" + filter);
            });
        arguments << "--" << filterArgs << exclusionArgs;

        process.setEnvironment(environment);
        process.setCommand({vcsBinary, arguments});
        process.setWorkingDirectory(parameters.searchDir);
    };

    const QString ref = gitParameters.ref.isEmpty() ? QString() : gitParameters.ref + ':';
    const auto outputParser = [&ref, &parameters](const QFuture<void> &future, const QString &input,
                                                 const std::optional<QRegularExpression> &regExp) {
        return parse(future, input, regExp, ref, parameters.searchDir);
    };

    TextEditor::searchInProcessOutput(promise, parameters, setupProcess, outputParser);
}

static bool isGitDirectory(const FilePath &path)
{
    static IVersionControl *gitVc = VcsManager::versionControl(VcsBase::Constants::VCS_ID_GIT);
    QTC_ASSERT(gitVc, return false);
    return gitVc == VcsManager::findVersionControlForDirectory(path);
}

GitGrep::GitGrep()
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
    Utils::onResultReady(gitClient().gitVersion(), this,
                         [this, pLayout = QPointer<QHBoxLayout>(layout)](unsigned version) {
        if (version >= 0x021300 && pLayout) {
            m_recurseSubmodules = new QCheckBox(Tr::tr("Recurse submodules"));
            pLayout->addWidget(m_recurseSubmodules);
        }
    });
    FindInFiles *findInFiles = FindInFiles::instance();
    QTC_ASSERT(findInFiles, return);
    connect(findInFiles, &FindInFiles::searchDirChanged, m_widget, [this](const FilePath &path) {
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

GitGrepParameters GitGrep::gitParameters() const
{
    return {m_treeLineEdit->text(), m_recurseSubmodules && m_recurseSubmodules->isChecked()};
}

void GitGrep::readSettings(QtcSettings *settings)
{
    m_treeLineEdit->setText(settings->value(GitGrepRef).toString());
}

void GitGrep::writeSettings(QtcSettings *settings) const
{
    settings->setValue(GitGrepRef, m_treeLineEdit->text());
}

SearchExecutor GitGrep::searchExecutor() const
{
    return [gitParameters = gitParameters()](const FileFindParameters &parameters) {
        return Utils::asyncRun(runGitGrep, parameters, gitParameters);
    };
}

EditorOpener GitGrep::editorOpener() const
{
    return [params = gitParameters()](const SearchResultItem &item,
                                      const FileFindParameters &parameters) -> IEditor * {
        const QStringList &itemPath = item.path();
        if (params.ref.isEmpty() || itemPath.isEmpty())
            return nullptr;
        const FilePath path = FilePath::fromUserInput(itemPath.first());
        IEditor *editor = gitClient().openShowEditor(
            parameters.searchDir, params.ref, path, GitClient::ShowEditor::OnlyIfDifferent);
        if (editor)
            editor->gotoLine(item.mainRange().begin.line, item.mainRange().begin.column);
        return editor;
    };
}

} // Git::Internal
