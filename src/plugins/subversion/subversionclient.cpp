/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "subversionclient.h"
#include "subversionsettings.h"
#include "subversionconstants.h"

#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

using namespace Utils;
using namespace VcsBase;

namespace Subversion {
namespace Internal {

// Collect all parameters required for a diff to be able to associate them
// with a diff editor and re-run the diff with parameters.
struct SubversionDiffParameters
{
    QString workingDir;
    QStringList extraOptions;
    QStringList files;
};

// Parameter widget controlling whitespace diff mode, associated with a parameter
class SubversionDiffParameterWidget : public VcsBaseEditorParameterWidget
{
    Q_OBJECT
public:
    explicit SubversionDiffParameterWidget(SubversionClient *client,
                                           const SubversionDiffParameters &p,
                                           QWidget *parent = 0);
    QStringList arguments() const;
    void executeCommand();

private:
    SubversionClient *m_client;
    const SubversionDiffParameters m_params;
};

SubversionDiffParameterWidget::SubversionDiffParameterWidget(SubversionClient *client,
                                                             const SubversionDiffParameters &p,
                                                             QWidget *parent)
    : VcsBaseEditorParameterWidget(parent), m_client(client), m_params(p)
{
    mapSetting(addToggleButton(QLatin1String("w"), tr("Ignore Whitespace")),
               client->settings()->boolPointer(SubversionSettings::diffIgnoreWhiteSpaceKey));
}

QStringList SubversionDiffParameterWidget::arguments() const
{
    QStringList args;
    // Subversion wants" -x -<ext-args>", default being -u
    const QStringList formatArguments = VcsBaseEditorParameterWidget::arguments();
    if (!formatArguments.isEmpty()) {
        args << QLatin1String("-x")
             << (QLatin1String("-u") + formatArguments.join(QString()));
    }
    return args;
}

void SubversionDiffParameterWidget::executeCommand()
{
    m_client->diff(m_params.workingDir, m_params.files, m_params.extraOptions);
}

SubversionClient::SubversionClient(SubversionSettings *settings) :
    VcsBaseClient(settings)
{
}

SubversionSettings *SubversionClient::settings() const
{
    return dynamic_cast<SubversionSettings *>(VcsBaseClient::settings());
}

VcsCommand *SubversionClient::createCommitCmd(const QString &repositoryRoot,
                                              const QStringList &files,
                                              const QString &commitMessageFile,
                                              const QStringList &extraOptions) const
{
    const QStringList svnExtraOptions =
            QStringList(extraOptions)
            << SubversionClient::addAuthenticationOptions(*settings())
            << QLatin1String(Constants::NON_INTERACTIVE_OPTION)
            << QLatin1String("--encoding") << QLatin1String("utf8")
            << QLatin1String("--file") << commitMessageFile;

    VcsCommand *cmd = createCommand(repositoryRoot);
    cmd->addFlags(VcsBasePlugin::ShowStdOutInLogWindow);
    QStringList args(vcsCommandString(CommitCommand));
    cmd->addJob(args << svnExtraOptions << files);
    return cmd;
}

void SubversionClient::commit(const QString &repositoryRoot,
                              const QStringList &files,
                              const QString &commitMessageFile,
                              const QStringList &extraOptions)
{
    if (Subversion::Constants::debug)
        qDebug() << Q_FUNC_INFO << commitMessageFile << files;

    VcsCommand *cmd = createCommitCmd(repositoryRoot, files, commitMessageFile, extraOptions);
    cmd->execute();
}

Core::Id SubversionClient::vcsEditorKind(VcsCommandTag cmd) const
{
    switch (cmd) {
    case DiffCommand:
        return "Subversion Diff Editor"; // TODO: create subversionconstants.h
    default:
        return Core::Id();
    }
}

// Add authorization options to the command line arguments.
QStringList SubversionClient::addAuthenticationOptions(const SubversionSettings &settings)
{
    if (!settings.hasAuthentication())
        return QStringList();

    const QString userName = settings.stringValue(SubversionSettings::userKey);
    const QString password = settings.stringValue(SubversionSettings::passwordKey);

    if (userName.isEmpty())
        return QStringList();

    QStringList rc;
    rc.push_back(QLatin1String("--username"));
    rc.push_back(userName);
    if (!password.isEmpty()) {
        rc.push_back(QLatin1String("--password"));
        rc.push_back(password);
    }
    return rc;
}

QString SubversionClient::synchronousTopic(const QString &repository)
{
    QStringList args;
    args << QLatin1String("info");

    QByteArray stdOut;
    if (!vcsFullySynchronousExec(repository, args, &stdOut))
        return QString();

    const QString revisionString = QLatin1String("Revision: ");
    // stdOut is ASCII only (at least in those areas we care about).
    QString output = SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(stdOut));
    foreach (const QString &line, output.split(QLatin1Char('\n'))) {
        if (line.startsWith(revisionString))
            return QString::fromLatin1("r") + line.mid(revisionString.count());
    }
    return QString();
}

void SubversionClient::diff(const QString &workingDir, const QStringList &files,
                           const QStringList &extraOptions)
{
    QStringList args;
    args << addAuthenticationOptions(*settings());
    args.append(QLatin1String("--internal-diff"));
    args << extraOptions;
    VcsBaseClient::diff(workingDir, files, args);
}

QString SubversionClient::findTopLevelForFile(const QFileInfo &file) const
{
    Q_UNUSED(file)
    return QString();
}

QStringList SubversionClient::revisionSpec(const QString &revision) const
{
    Q_UNUSED(revision)
    return QStringList();
}

VcsBaseClient::StatusItem SubversionClient::parseStatusLine(const QString &line) const
{
    Q_UNUSED(line)
    return VcsBaseClient::StatusItem();
}

VcsBaseEditorParameterWidget *SubversionClient::createDiffEditor(
        const QString &workingDir, const QStringList &files, const QStringList &extraOptions)
{
    Q_UNUSED(extraOptions)
    SubversionDiffParameters p;
    p.workingDir = workingDir;
    p.files = files;
    p.extraOptions = extraOptions;
    return new SubversionDiffParameterWidget(this, p);
}

} // namespace Internal
} // namespace Subversion

#include "subversionclient.moc"
