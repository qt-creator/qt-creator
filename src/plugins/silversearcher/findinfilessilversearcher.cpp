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

static void runSilverSeacher(QPromise<SearchResultItems> &promise,
                             const FileFindParameters &parameters)
{
    const auto setupProcess = [parameters](Process &process) {
        const FilePath directory
            = FilePath::fromUserInput(parameters.additionalParameters.toString());
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
        process.setCommand({"ag", arguments});
    };

    FilePath lastFilePath;
    const auto outputParser = [&lastFilePath](const QFuture<void> &future, const QString &input,
                                              const std::optional<QRegularExpression> &regExp) {
        return SilverSearcher::parse(future, input, regExp, &lastFilePath);
    };

    TextEditor::searchInProcessOutput(promise, parameters, setupProcess, outputParser);
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
