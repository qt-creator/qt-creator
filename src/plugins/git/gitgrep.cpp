/****************************************************************************
**
** Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
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

#include "gitgrep.h"
#include "gitclient.h"
#include "gitplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/vcsmanager.h>
#include <vcsbase/vcsbaseconstants.h>

#include <utils/filesearch.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>

#include <QDir>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QLabel>
#include <QSettings>

using namespace Utils;

namespace Git {
namespace Internal {

using namespace Core;

QString GitGrep::id() const
{
    return QLatin1String("Git Grep");
}

QString GitGrep::displayName() const
{
    return tr("Git Grep");
}

namespace {

class GitGrepRunner : public QObject
{
    using FutureInterfaceType = QFutureInterface<FileSearchResultList>;

public:
    GitGrepRunner(FutureInterfaceType &fi,
                  const TextEditor::FileFindParameters &parameters) :
        m_fi(fi),
        m_parameters(parameters)
    {
        m_directory = parameters.additionalParameters.toString();
    }

    void processLine(const QByteArray &line, FileSearchResultList *resultList) const
    {
        if (line.isEmpty())
            return;
        static const char boldRed[] = "\x1b[1;31m";
        static const char resetColor[] = "\x1b[m";
        FileSearchResult single;
        const int lineSeparator = line.indexOf('\0');
        single.fileName = m_directory + QLatin1Char('/')
                + QString::fromLocal8Bit(line.left(lineSeparator));
        const int textSeparator = line.indexOf('\0', lineSeparator + 1);
        single.lineNumber = line.mid(lineSeparator + 1, textSeparator - lineSeparator - 1).toInt();
        QByteArray text = line.mid(textSeparator + 1);
        QVector<QPair<int, int>> matches;
        for (;;) {
            const int matchStart = text.indexOf(boldRed);
            if (matchStart == -1)
                break;
            const int matchTextStart = matchStart + int(sizeof(boldRed)) - 1;
            const int matchEnd = text.indexOf(resetColor, matchTextStart);
            QTC_ASSERT(matchEnd != -1, break);
            const int matchLength = matchEnd - matchTextStart;
            matches.append(qMakePair(matchStart, matchLength));
            text = text.left(matchStart) + text.mid(matchTextStart, matchLength)
                    + text.mid(matchEnd + int(sizeof(resetColor)) - 1);
        }
        single.matchingLine = QString::fromLocal8Bit(text);
        foreach (auto match, matches) {
            single.matchStart = match.first;
            single.matchLength = match.second;
            resultList->append(single);
        }
    }

    void read()
    {
        FileSearchResultList resultList;
        while (m_process.canReadLine() && !m_fi.isCanceled())
            processLine(m_process.readLine().trimmed(), &resultList);
        if (!resultList.isEmpty())
            m_fi.reportResult(resultList);
    }

    void finish()
    {
        read();
        m_fi.setProgressValue(1);
        m_fi.reportFinished();
    }

    void exec()
    {
        m_fi.setProgressRange(0, 1);
        m_fi.setProgressValue(0);

        QStringList arguments;
        arguments << QLatin1String("-c") << QLatin1String("color.grep.match=bold red")
                  << QLatin1String("grep") << QLatin1String("-zn")
                  << QLatin1String("--color=always");
        if (!(m_parameters.flags & FindCaseSensitively))
            arguments << QLatin1String("-i");
        if (m_parameters.flags & FindWholeWords)
            arguments << QLatin1String("-w");
        if (m_parameters.flags & FindRegularExpression)
            arguments << QLatin1String("-P");
        else
            arguments << QLatin1String("-F");
        arguments << m_parameters.text;
        arguments << QLatin1String("--") << m_parameters.nameFilters;
        QString args;
        m_process.addArgs(&args, arguments);
        m_process.setWorkingDirectory(m_directory);
        m_process.setCommand(GitPlugin::instance()->client()->vcsBinary().toString(), args);
        QFutureWatcher<FileSearchResultList> watcher;
        watcher.setFuture(m_fi.future());
        connect(&watcher, &QFutureWatcher<FileSearchResultList>::canceled,
                &m_process, &QtcProcess::kill);
        connect(&m_process, &QProcess::readyRead,
                this, &GitGrepRunner::read);
        connect(&m_process, static_cast<void(QProcess::*)(int)>(&QProcess::finished),
                this, &GitGrepRunner::finish);
        QEventLoop eventLoop;
        connect(&watcher, &QFutureWatcher<FileSearchResultList>::finished,
                &eventLoop, &QEventLoop::quit);
        m_process.start();
        if (!m_process.waitForStarted())
            return;
        m_fi.reportStarted();
        eventLoop.exec();
    }

    static void run(QFutureInterface<FileSearchResultList> &fi,
                    TextEditor::FileFindParameters parameters)
    {
        GitGrepRunner runner(fi, parameters);
        runner.exec();
    }

private:
    QtcProcess m_process;
    FutureInterfaceType m_fi;
    QString m_directory;
    const TextEditor::FileFindParameters &m_parameters;
};

} // namespace

QFuture<FileSearchResultList> GitGrep::executeSearch(
        const TextEditor::FileFindParameters &parameters)
{
    return Utils::runAsync<FileSearchResultList>(GitGrepRunner::run, parameters);
}

FileIterator *GitGrep::files(const QStringList &, const QVariant &) const
{
    QTC_ASSERT(false, return 0);
}

QVariant GitGrep::additionalParameters() const
{
    return qVariantFromValue(path().toString());
}

QString GitGrep::label() const
{
    const QChar slash = QLatin1Char('/');
    const QStringList &nonEmptyComponents = path().toFileInfo().absoluteFilePath()
            .split(slash, QString::SkipEmptyParts);
    return tr("Git Grep \"%1\":").arg(nonEmptyComponents.isEmpty() ? QString(slash)
                                                                   : nonEmptyComponents.last());
}

QString GitGrep::toolTip() const
{
    //: %3 is filled by BaseFileFind::runNewSearch
    return tr("Path: %1\nFilter: %2\n%3")
            .arg(path().toUserOutput(), fileNameFilters().join(QLatin1Char(',')));
}

bool GitGrep::validateDirectory(FancyLineEdit *edit, QString *errorMessage) const
{
    static IVersionControl *gitVc =
            VcsManager::versionControl(VcsBase::Constants::VCS_ID_GIT);
    QTC_ASSERT(gitVc, return false);
    if (!m_directory->defaultValidationFunction()(edit, errorMessage))
        return false;
    const QString path = m_directory->path();
    IVersionControl *vc = VcsManager::findVersionControlForDirectory(path, 0);
    if (vc == gitVc)
        return true;
    if (errorMessage)
        *errorMessage = tr("The path \"%1\" is not managed by Git").arg(path);
    return false;
}

QWidget *GitGrep::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);

        QLabel *dirLabel = new QLabel(tr("Director&y:"));
        gridLayout->addWidget(dirLabel, 0, 0, Qt::AlignRight);
        m_directory = new PathChooser;
        m_directory->setExpectedKind(PathChooser::ExistingDirectory);
        m_directory->setHistoryCompleter(QLatin1String("Git.Grep.History"), true);
        m_directory->setPromptDialogTitle(tr("Directory to search"));
        m_directory->setValidationFunction([this](FancyLineEdit *edit, QString *errorMessage) {
            return validateDirectory(edit, errorMessage);
        });
        connect(m_directory.data(), &PathChooser::validChanged,
                this, &GitGrep::enabledChanged);
        dirLabel->setBuddy(m_directory);
        gridLayout->addWidget(m_directory, 0, 1, 1, 2);

        QLabel * const filePatternLabel = new QLabel(tr("Fi&le pattern:"));
        filePatternLabel->setMinimumWidth(80);
        filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        filePatternLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QWidget *patternWidget = createPatternWidget();
        filePatternLabel->setBuddy(patternWidget);
        gridLayout->addWidget(filePatternLabel, 1, 0);
        gridLayout->addWidget(patternWidget, 1, 1, 1, 2);
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    return m_configWidget;
}

FileName GitGrep::path() const
{
    return m_directory->fileName();
}

void GitGrep::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("GitGrep"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void GitGrep::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("GitGrep"));
    readCommonSettings(settings, QString());
    settings->endGroup();
}

bool GitGrep::isValid() const
{
    return m_directory->isValid();
}

void GitGrep::setDirectory(const FileName &directory)
{
    m_directory->setFileName(directory);
}

} // Internal
} // Git
