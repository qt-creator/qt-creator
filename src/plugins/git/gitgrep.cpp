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

#include <coreplugin/vcsmanager.h>
#include <texteditor/findinfiles.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseconstants.h>

#include <utils/filesearch.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/synchronousprocess.h>

#include <QCheckBox>
#include <QFuture>
#include <QFutureWatcher>
#include <QScopedPointer>
#include <QSettings>
#include <QTextStream>

using namespace Utils;

namespace Git {
namespace Internal {

using namespace Core;
using VcsBase::VcsCommand;

namespace {

const char EnableGitGrep[] = "EnableGitGrep";

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

    void processLine(const QString &line, FileSearchResultList *resultList) const
    {
        if (line.isEmpty())
            return;
        static const QLatin1String boldRed("\x1b[1;31m");
        static const QLatin1String resetColor("\x1b[m");
        FileSearchResult single;
        const int lineSeparator = line.indexOf(QChar::Null);
        single.fileName = m_directory + QLatin1Char('/')
                + line.left(lineSeparator);
        const int textSeparator = line.indexOf(QChar::Null, lineSeparator + 1);
        single.lineNumber = line.mid(lineSeparator + 1, textSeparator - lineSeparator - 1).toInt();
        QString text = line.mid(textSeparator + 1);
        QVector<QPair<int, int>> matches;
        for (;;) {
            const int matchStart = text.indexOf(boldRed);
            if (matchStart == -1)
                break;
            const int matchTextStart = matchStart + boldRed.size();
            const int matchEnd = text.indexOf(resetColor, matchTextStart);
            QTC_ASSERT(matchEnd != -1, break);
            const int matchLength = matchEnd - matchTextStart;
            matches.append(qMakePair(matchStart, matchLength));
            text = text.left(matchStart) + text.mid(matchTextStart, matchLength)
                    + text.mid(matchEnd + resetColor.size());
        }
        single.matchingLine = text;
        foreach (auto match, matches) {
            single.matchStart = match.first;
            single.matchLength = match.second;
            resultList->append(single);
        }
    }

    void read(const QString &text)
    {
        FileSearchResultList resultList;
        QString t = text;
        QTextStream stream(&t);
        while (!stream.atEnd() && !m_fi.isCanceled())
            processLine(stream.readLine(), &resultList);
        if (!resultList.isEmpty())
            m_fi.reportResult(resultList);
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
        GitClient *client = GitPlugin::instance()->client();
        QScopedPointer<VcsCommand> command(client->createCommand(m_directory));
        command->addFlags(VcsCommand::SilentOutput);
        command->setProgressiveOutput(true);
        QFutureWatcher<FileSearchResultList> watcher;
        watcher.setFuture(m_fi.future());
        connect(&watcher, &QFutureWatcher<FileSearchResultList>::canceled,
                command.data(), &VcsCommand::cancel);
        connect(command.data(), &VcsCommand::stdOutText, this, &GitGrepRunner::read);
        SynchronousProcessResponse resp = command->runCommand(client->vcsBinary(), arguments, 0);
        switch (resp.result) {
        case SynchronousProcessResponse::TerminatedAbnormally:
        case SynchronousProcessResponse::StartFailed:
        case SynchronousProcessResponse::Hang:
            m_fi.reportCanceled();
            break;
        case SynchronousProcessResponse::Finished:
        case SynchronousProcessResponse::FinishedError:
            // When no results are found, git-grep exits with non-zero status.
            // Do not consider this as an error.
            break;
        }
    }

    static void run(QFutureInterface<FileSearchResultList> &fi,
                    TextEditor::FileFindParameters parameters)
    {
        GitGrepRunner runner(fi, parameters);
        runner.exec();
    }

private:
    FutureInterfaceType m_fi;
    QString m_directory;
    const TextEditor::FileFindParameters &m_parameters;
};

} // namespace

static bool validateDirectory(const QString &path)
{
    static IVersionControl *gitVc = VcsManager::versionControl(VcsBase::Constants::VCS_ID_GIT);
    QTC_ASSERT(gitVc, return false);
    return gitVc == VcsManager::findVersionControlForDirectory(path, 0);
}

GitGrep::GitGrep()
{
    m_widget = new QCheckBox(tr("&Use Git Grep"));
    m_widget->setToolTip(tr("Use Git Grep for searching. This includes only files "
                            "that are managed by source control."));
    TextEditor::FindInFiles *findInFiles = TextEditor::FindInFiles::instance();
    QTC_ASSERT(findInFiles, return);
    QObject::connect(findInFiles, &TextEditor::FindInFiles::pathChanged,
                     m_widget.data(), [this](const QString &path) {
        m_widget->setEnabled(validateDirectory(path));
    });
    findInFiles->setFindExtension(this);
}

GitGrep::~GitGrep()
{
    delete m_widget.data();
}

QString GitGrep::title() const
{
    return tr("Git Grep");
}

QWidget *GitGrep::widget() const
{
    return m_widget.data();
}

bool GitGrep::isEnabled() const
{
    return m_widget->isEnabled() && m_widget->isChecked();
}

bool GitGrep::isEnabled(const TextEditor::FileFindParameters &parameters) const
{
    return parameters.extensionParameters.toBool();
}

QVariant GitGrep::parameters() const
{
    return isEnabled();
}

void GitGrep::readSettings(QSettings *settings)
{
    m_widget->setChecked(settings->value(QLatin1String(EnableGitGrep), false).toBool());
}

void GitGrep::writeSettings(QSettings *settings) const
{
    settings->setValue(QLatin1String(EnableGitGrep), m_widget->isChecked());
}

QFuture<FileSearchResultList> GitGrep::executeSearch(
        const TextEditor::FileFindParameters &parameters)
{
    return Utils::runAsync<FileSearchResultList>(GitGrepRunner::run, parameters);
}

} // Internal
} // Git
