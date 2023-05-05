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
#include <QTextStream>

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

class GitGrepRunner
{
    using PromiseType = QPromise<SearchResultItems>;

public:
    GitGrepRunner(const TextEditor::FileFindParameters &parameters)
        : m_parameters(parameters)
    {
        m_directory = FilePath::fromString(parameters.additionalParameters.toString());
        m_vcsBinary = GitClient::instance()->vcsBinary();
        m_environment = GitClient::instance()->processEnvironment();
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

    void processLine(const QString &line, SearchResultItems *resultList) const
    {
        if (line.isEmpty())
            return;
        static const QLatin1String boldRed("\x1b[1;31m");
        static const QLatin1String resetColor("\x1b[m");
        SearchResultItem result;
        const int lineSeparator = line.indexOf(QChar::Null);
        QString filePath = line.left(lineSeparator);
        if (!m_ref.isEmpty() && filePath.startsWith(m_ref))
            filePath.remove(0, m_ref.length());
        result.setFilePath(m_directory.pathAppended(filePath));
        const int textSeparator = line.indexOf(QChar::Null, lineSeparator + 1);
        const int lineNumber = line.mid(lineSeparator + 1, textSeparator - lineSeparator - 1).toInt();
        QString text = line.mid(textSeparator + 1);
        QRegularExpression regexp;
        QList<Match> matches;
        if (m_parameters.flags & FindRegularExpression) {
            const QRegularExpression::PatternOptions patternOptions =
                    (m_parameters.flags & FindCaseSensitively)
                    ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;
            regexp.setPattern(m_parameters.text);
            regexp.setPatternOptions(patternOptions);
        }
        for (;;) {
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
                match.regexpCapturedTexts = regexp.match(matchText).capturedTexts();
            matches.append(match);
            text = text.left(matchStart) + matchText + text.mid(matchEnd + resetColor.size());
        }
        result.setDisplayText(text);

        for (const auto &match : std::as_const(matches)) {
            result.setMainRange(lineNumber, match.matchStart, match.matchLength);
            result.setUserData(match.regexpCapturedTexts);
            resultList->append(result);
        }
    }

    void read(PromiseType &fi, const QString &text)
    {
        SearchResultItems resultList;
        QString t = text;
        QTextStream stream(&t);
        while (!stream.atEnd() && !fi.isCanceled())
            processLine(stream.readLine(), &resultList);
        if (!resultList.isEmpty() && !fi.isCanceled())
            fi.addResult(resultList);
    }

    void operator()(PromiseType &promise)
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

        Process process;
        process.setEnvironment(m_environment);
        process.setCommand({m_vcsBinary, arguments});
        process.setWorkingDirectory(m_directory);
        process.setStdOutCallback([this, &promise](const QString &text) { read(promise, text); });
        process.start();
        process.waitForFinished();

        switch (process.result()) {
        case ProcessResult::TerminatedAbnormally:
        case ProcessResult::StartFailed:
        case ProcessResult::Hang:
            promise.future().cancel();
            break;
        case ProcessResult::FinishedWithSuccess:
        case ProcessResult::FinishedWithError:
            // When no results are found, git-grep exits with non-zero status.
            // Do not consider this as an error.
            break;
        }
    }

private:
    FilePath m_vcsBinary;
    FilePath m_directory;
    QString m_ref;
    TextEditor::FileFindParameters m_parameters;
    Environment m_environment;
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
