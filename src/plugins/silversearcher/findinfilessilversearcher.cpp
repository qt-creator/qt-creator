/****************************************************************************
**
** Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "findinfilessilversearcher.h"

#include <aggregation/aggregate.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <texteditor/findinfiles.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include "silversearcheroutputparser.h"

#include <QProcess>
#include <QHBoxLayout>
#include <QLabel>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace {
const QLatin1String silverSearcherName("Silver Searcher");

using FutureInterfaceType = QFutureInterface<FileSearchResultList>;

const QString metacharacters = "+()^$.{}[]|\\";

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
    QProcess silverSearcherProcess;
    silverSearcherProcess.start("ag", {"--version"});
    if (silverSearcherProcess.waitForFinished()) {
        if (silverSearcherProcess.readAll().contains("ag version"))
            return true;
    }

    return false;
}

void runSilverSeacher(FutureInterfaceType &fi, FileFindParameters parameters)
{
    ProgressTimer progress(fi, 5);
    const QString directory = parameters.additionalParameters.toString();
    QStringList arguments = {"--parallel", "--ackmate"};

    if (parameters.flags & FindCaseSensitively)
        arguments << "-s";
    else
        arguments << "-i";

    if (parameters.flags & FindWholeWords)
        arguments << "-w";

    if (!(parameters.flags & FindRegularExpression))
        arguments << "-Q";

    for (const QString &filter : parameters.exclusionFilters)
        arguments << "--ignore" << filter;

    FileName path = FileName::fromUserInput(FileUtils::normalizePathName(directory));
    arguments << parameters.text << path.toString();

    QString nameFiltersAsRegex;
    for (const QString &filter : parameters.nameFilters)
        nameFiltersAsRegex += QString("(%1)|").arg(convertWildcardToRegex(filter));
    nameFiltersAsRegex.remove(nameFiltersAsRegex.length() - 1, 1);

    arguments << "-G" << nameFiltersAsRegex;

    QProcess process;
    process.start("ag", arguments);
    if (process.waitForFinished()) {
        typedef QList<FileSearchResult> FileSearchResultList;
        SilverSearcher::SilverSearcherOutputParser parser(process.readAll());
        FileSearchResultList items = parser.parse();
        if (!items.isEmpty())
            fi.reportResult(items);
    } else {
        fi.reportCanceled();
    }
}

} // namespace

namespace SilverSearcher {

FindInFilesSilverSearcher::FindInFilesSilverSearcher()
    : m_widget(0),
      m_path("ag"),
      m_toolName("SilverSearcher")
{
    m_widget = new QWidget;
    FindInFiles *findInFiles = FindInFiles::instance();
    QTC_ASSERT(findInFiles, return);
    findInFiles->addSearchEngine(this);

    setEnabled(isSilverSearcherAvailable());
    if (!isEnabled()) {
        auto layout = new QHBoxLayout(m_widget);
        layout->setMargin(0);
        QLabel *label = new QLabel(tr("SilverSearcher is not available on system"));
        label->setStyleSheet("QLabel { color : red; }");
        layout->addWidget(label);
    }
}

FindInFilesSilverSearcher::~FindInFilesSilverSearcher()
{
}

QVariant FindInFilesSilverSearcher::parameters() const
{
    return QVariant();
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

void FindInFilesSilverSearcher::writeSettings(QSettings * /*settings*/) const
{
}

QFuture<FileSearchResultList> FindInFilesSilverSearcher::executeSearch(
        const FileFindParameters &parameters, BaseFileFind * /*baseFileFind*/)
{
    return Utils::runAsync(runSilverSeacher, parameters);
}

IEditor *FindInFilesSilverSearcher::openEditor(const SearchResultItem & /*item*/,
                                               const FileFindParameters & /*parameters*/)
{
    return 0;
}

void FindInFilesSilverSearcher::readSettings(QSettings * /*settings*/)
{
}

} // namespace SilverSearcher
