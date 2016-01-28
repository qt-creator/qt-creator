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
#include <vcsbase/vcsbaseconstants.h>

#include <utils/filesearch.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QSettings>

using namespace Utils;

namespace Git {
namespace Internal {

using namespace Core;

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
        m_process.start();
        if (!m_process.waitForStarted())
            return;
        QEventLoop eventLoop;
        connect(&m_process, static_cast<void(QProcess::*)(int)>(&QProcess::finished),
                this, [this, &eventLoop]() {
            read();
            eventLoop.quit();
        });
        eventLoop.exec();
        m_fi.setProgressValue(1);
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
