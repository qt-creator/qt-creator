// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findinfilessilversearcher.h"

#include <aggregation/aggregate.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <texteditor/findinfiles.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>

#include "silversearcheroutputparser.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace {
const QLatin1String silverSearcherName("Silver Searcher");

using FutureInterfaceType = QFutureInterface<FileSearchResultList>;

const QString metacharacters = "+()^$.{}[]|\\";

const QString SearchOptionsString = "SearchOptionsString";

class SilverSearcherSearchOptions
{
public:
    QString searchOptions;
};

QString convertWildcardToRegex(const QString &wildcard)
{
    QString regex;
    const int wildcardSize = wildcard.size();
    regex.append('^');
    for (int i = 0; i < wildcardSize; ++i) {
        const QChar ch = wildcard[i];
        if (ch == '*') {
            regex.append(".*");
        } else if (ch == '?') {
            regex.append('.');
        } else if (metacharacters.indexOf(ch) != -1) {
            regex.append('\\');
            regex.append(ch);
        } else {
            regex.append(ch);
        }
    }
    regex.append('$');

    return regex;
}

bool isSilverSearcherAvailable()
{
    QtcProcess silverSearcherProcess;
    silverSearcherProcess.setCommand({"ag", {"--version"}});
    silverSearcherProcess.start();
    if (silverSearcherProcess.waitForFinished(1000)) {
        if (silverSearcherProcess.cleanedStdOut().contains("ag version"))
            return true;
    }

    return false;
}

void runSilverSeacher(FutureInterfaceType &fi, FileFindParameters parameters)
{
    ProgressTimer progress(fi, 5);
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

    QString nameFiltersAsRegex;
    for (const QString &filter : std::as_const(parameters.nameFilters))
        nameFiltersAsRegex += QString("(%1)|").arg(convertWildcardToRegex(filter));
    nameFiltersAsRegex.remove(nameFiltersAsRegex.length() - 1, 1);

    arguments << "-G" << nameFiltersAsRegex;

    SilverSearcherSearchOptions params = parameters.searchEngineParameters
                                             .value<SilverSearcherSearchOptions>();
    if (!params.searchOptions.isEmpty())
        arguments << params.searchOptions.split(' ');

    arguments << "--" << parameters.text << directory.normalizedPathName().toString();

    QtcProcess process;
    process.setCommand({"ag", arguments});
    process.start();
    if (process.waitForFinished()) {
        QRegularExpression regexp;
        if (parameters.flags & FindRegularExpression) {
            const QRegularExpression::PatternOptions patternOptions
                = (parameters.flags & FindCaseSensitively)
                      ? QRegularExpression::NoPatternOption
                      : QRegularExpression::CaseInsensitiveOption;
            regexp.setPattern(parameters.text);
            regexp.setPatternOptions(patternOptions);
        }
        SilverSearcher::SilverSearcherOutputParser parser(process.cleanedStdOut(), regexp);
        FileSearchResultList items = parser.parse();
        if (!items.isEmpty())
            fi.reportResult(items);
    } else {
        fi.reportCanceled();
    }
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
    m_searchOptionsLineEdit->setPlaceholderText(tr("Search Options (optional)"));
    layout->addWidget(m_searchOptionsLineEdit);

    FindInFiles *findInFiles = FindInFiles::instance();
    QTC_ASSERT(findInFiles, return);
    findInFiles->addSearchEngine(this);

    setEnabled(isSilverSearcherAvailable());
    if (!isEnabled()) {
        QLabel *label = new QLabel(tr("Silver Searcher is not available on the system."));
        label->setStyleSheet("QLabel { color : red; }");
        layout->addWidget(label);
    }
}

FindInFilesSilverSearcher::~FindInFilesSilverSearcher()
{
}

QVariant FindInFilesSilverSearcher::parameters() const
{
    SilverSearcherSearchOptions silverSearcherSearchOptions;
    silverSearcherSearchOptions.searchOptions = m_searchOptionsLineEdit->text();
    return QVariant::fromValue(silverSearcherSearchOptions);
}

QString FindInFilesSilverSearcher::title() const
{
    return silverSearcherName;
}

QString FindInFilesSilverSearcher::toolTip() const
{
    return QString();
}

QWidget *FindInFilesSilverSearcher::widget() const
{
    return m_widget;
}

void FindInFilesSilverSearcher::writeSettings(QSettings *settings) const
{
    settings->setValue(SearchOptionsString, m_searchOptionsLineEdit->text());
}

QFuture<FileSearchResultList> FindInFilesSilverSearcher::executeSearch(
        const FileFindParameters &parameters, BaseFileFind * /*baseFileFind*/)
{
    return Utils::runAsync(runSilverSeacher, parameters);
}

IEditor *FindInFilesSilverSearcher::openEditor(const SearchResultItem & /*item*/,
                                               const FileFindParameters & /*parameters*/)
{
    return nullptr;
}

void FindInFilesSilverSearcher::readSettings(QSettings *settings)
{
    m_searchOptionsLineEdit->setText(settings->value(SearchOptionsString).toString());
}

} // namespace SilverSearcher
