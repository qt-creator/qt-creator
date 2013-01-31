/**************************************************************************
**
** Copyright (c) 2013 Hugues Delorme
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "bazaarclient.h"
#include "constants.h"

#include <vcsbase/vcsbaseclientsettings.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>

#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

namespace Bazaar {
namespace Internal {

BazaarClient::BazaarClient(BazaarSettings *settings) :
    VcsBase::VcsBaseClient(settings)
{
}

BazaarSettings *BazaarClient::settings() const
{
    return dynamic_cast<BazaarSettings *>(VcsBase::VcsBaseClient::settings());
}

bool BazaarClient::synchronousSetUserId()
{
    QStringList args;
    args << QLatin1String("whoami")
         << (settings()->stringValue(BazaarSettings::userNameKey) + QLatin1String(" <")
             + settings()->stringValue(BazaarSettings::userEmailKey) + QLatin1Char('>'));
    QByteArray stdOut;
    return vcsFullySynchronousExec(QDir::currentPath(), args, &stdOut);
}

BranchInfo BazaarClient::synchronousBranchQuery(const QString &repositoryRoot) const
{
    QFile branchConfFile(repositoryRoot + QDir::separator() +
                         QLatin1String(Constants::BAZAARREPO) +
                         QLatin1String("/branch/branch.conf"));
    if (!branchConfFile.open(QIODevice::ReadOnly))
        return BranchInfo(QString(), false);

    QTextStream ts(&branchConfFile);
    QString branchLocation;
    QString isBranchBound;
    QRegExp branchLocationRx(QLatin1String("bound_location\\s*=\\s*(.+)$"));
    QRegExp isBranchBoundRx(QLatin1String("bound\\s*=\\s*(.+)$"));
    while (!ts.atEnd() && (branchLocation.isEmpty() || isBranchBound.isEmpty())) {
        const QString line = ts.readLine();
        if (branchLocationRx.indexIn(line) != -1)
            branchLocation = branchLocationRx.cap(1);
        else if (isBranchBoundRx.indexIn(line) != -1)
            isBranchBound = isBranchBoundRx.cap(1);
    }
    if (isBranchBound.simplified().toLower() == QLatin1String("true"))
        return BranchInfo(branchLocation, true);
    return BranchInfo(repositoryRoot, false);
}

void BazaarClient::commit(const QString &repositoryRoot, const QStringList &files,
                          const QString &commitMessageFile, const QStringList &extraOptions)
{
    VcsBaseClient::commit(repositoryRoot, files, commitMessageFile,
                          QStringList(extraOptions) << QLatin1String("-F") << commitMessageFile);
}

void BazaarClient::annotate(const QString &workingDir, const QString &file,
                            const QString revision, int lineNumber,
                            const QStringList &extraOptions)
{
    VcsBaseClient::annotate(workingDir, file, revision, lineNumber,
                            QStringList(extraOptions) << QLatin1String("--long"));
}

QString BazaarClient::findTopLevelForFile(const QFileInfo &file) const
{
    const QString repositoryCheckFile =
            QLatin1String(Constants::BAZAARREPO) + QLatin1String("/branch-format");
    return file.isDir() ?
                VcsBase::VcsBasePlugin::findRepositoryForDirectory(file.absoluteFilePath(),
                                                                   repositoryCheckFile) :
                VcsBase::VcsBasePlugin::findRepositoryForDirectory(file.absolutePath(),
                                                                   repositoryCheckFile);
}

void BazaarClient::view(const QString &source, const QString &id, const QStringList &extraOptions)
{
    QStringList args(QLatin1String("log"));
    args << QLatin1String("-p") << QLatin1String("-v") << extraOptions;
    VcsBaseClient::view(source, id, args);
}

QString BazaarClient::vcsEditorKind(VcsCommand cmd) const
{
    switch (cmd) {
    case AnnotateCommand:
        return QLatin1String(Constants::ANNOTATELOG);
    case DiffCommand:
        return QLatin1String(Constants::DIFFLOG);
    case LogCommand:
        return QLatin1String(Constants::FILELOG);
    default:
        return QString();
    }
}

QString BazaarClient::vcsCommandString(VcsCommand cmd) const
{
    switch (cmd) {
    case CloneCommand:
        return QLatin1String("branch");
    default:
        return VcsBaseClient::vcsCommandString(cmd);
    }
}

QStringList BazaarClient::revisionSpec(const QString &revision) const
{
    QStringList args;
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    return args;
}

BazaarClient::StatusItem BazaarClient::parseStatusLine(const QString &line) const
{
    StatusItem item;
    if (!line.isEmpty()) {
        const QChar flagVersion = line[0];
        if (flagVersion == QLatin1Char('+'))
            item.flags = QLatin1String("Versioned");
        else if (flagVersion == QLatin1Char('-'))
            item.flags = QLatin1String("Unversioned");
        else if (flagVersion == QLatin1Char('R'))
            item.flags = QLatin1String("Renamed");
        else if (flagVersion == QLatin1Char('?'))
            item.flags = QLatin1String("Unknown");
        else if (flagVersion == QLatin1Char('X'))
            item.flags = QLatin1String("Nonexistent");
        else if (flagVersion == QLatin1Char('C'))
            item.flags = QLatin1String("Conflict");
        else if (flagVersion == QLatin1Char('P'))
            item.flags = QLatin1String("PendingMerge");

        const int lineLength = line.length();
        if (lineLength >= 2) {
            const QChar flagContents = line[1];
            if (flagContents == QLatin1Char('N'))
                item.flags = QLatin1String("Created");
            else if (flagContents == QLatin1Char('D'))
                item.flags = QLatin1String("Deleted");
            else if (flagContents == QLatin1Char('K'))
                item.flags = QLatin1String("KindChanged");
            else if (flagContents == QLatin1Char('M'))
                item.flags = QLatin1String("Modified");
        }
        if (lineLength >= 3) {
            const QChar flagExec = line[2];
            if (flagExec == QLatin1Char('*'))
                item.flags = QLatin1String("ExecuteBitChanged");
        }
        // The status string should be similar to "xxx file_with_changes"
        // so just should take the file name part and store it
        item.file = line.mid(4);
    }
    return item;
}

// Collect all parameters required for a diff or log to be able to associate
// them with an editor and re-run the command with parameters.
struct BazaarCommandParameters
{
    BazaarCommandParameters(const QString &workDir,
                            const QStringList &inFiles,
                            const QStringList &options) :
        workingDir(workDir), files(inFiles), extraOptions(options)
    {
    }

    QString workingDir;
    QStringList files;
    QStringList extraOptions;
};

// Parameter widget controlling whitespace diff mode, associated with a parameter
class BazaarDiffParameterWidget : public VcsBase::VcsBaseEditorParameterWidget
{
    Q_OBJECT
public:
    BazaarDiffParameterWidget(BazaarClient *client,
                              const BazaarCommandParameters &p, QWidget *parent = 0) :
        VcsBase::VcsBaseEditorParameterWidget(parent), m_client(client), m_params(p)
    {
        mapSetting(addToggleButton(QLatin1String("-w"), tr("Ignore whitespace")),
                   client->settings()->boolPointer(BazaarSettings::diffIgnoreWhiteSpaceKey));
        mapSetting(addToggleButton(QLatin1String("-B"), tr("Ignore blank lines")),
                   client->settings()->boolPointer(BazaarSettings::diffIgnoreBlankLinesKey));
    }

    QStringList arguments() const
    {
        QStringList args;
        // Bazaar wants "--diff-options=-w -B.."
        const QStringList formatArguments = VcsBaseEditorParameterWidget::arguments();
        if (!formatArguments.isEmpty()) {
            const QString a = QLatin1String("--diff-options=")
                    + formatArguments.join(QString(QLatin1Char(' ')));
            args.append(a);
        }
        return args;
    }

    void executeCommand()
    {
        m_client->diff(m_params.workingDir, m_params.files, m_params.extraOptions);
    }

private:
    BazaarClient *m_client;
    const BazaarCommandParameters m_params;
};

VcsBase::VcsBaseEditorParameterWidget *BazaarClient::createDiffEditor(
        const QString &workingDir, const QStringList &files, const QStringList &extraOptions)
{
    const BazaarCommandParameters parameters(workingDir, files, extraOptions);
    return new BazaarDiffParameterWidget(this, parameters);
}

class BazaarLogParameterWidget : public VcsBase::VcsBaseEditorParameterWidget
{
    Q_OBJECT
public:
    BazaarLogParameterWidget(BazaarClient *client,
                             const BazaarCommandParameters &p, QWidget *parent = 0) :
        VcsBase::VcsBaseEditorParameterWidget(parent), m_client(client), m_params(p)
    {
        mapSetting(addToggleButton(QLatin1String("--verbose"), tr("Verbose"),
                                   tr("Show files changed in each revision")),
                   m_client->settings()->boolPointer(BazaarSettings::logVerboseKey));
        mapSetting(addToggleButton(QLatin1String("--forward"), tr("Forward"),
                                   tr("Show from oldest to newest")),
                   m_client->settings()->boolPointer(BazaarSettings::logForwardKey));
        mapSetting(addToggleButton(QLatin1String("--include-merges"), tr("Include merges"),
                                   tr("Show merged revisions")),
                   m_client->settings()->boolPointer(BazaarSettings::logIncludeMergesKey));

        QList<ComboBoxItem> logChoices;
        logChoices << ComboBoxItem(tr("Detailed"), QLatin1String("long"))
                   << ComboBoxItem(tr("Moderately short"), QLatin1String("short"))
                   << ComboBoxItem(tr("One line"), QLatin1String("line"))
                   << ComboBoxItem(tr("GNU ChangeLog"), QLatin1String("gnu-changelog"));
        mapSetting(addComboBox(QLatin1String("--log-format"), logChoices),
                   m_client->settings()->stringPointer(BazaarSettings::logFormatKey));
    }

    void executeCommand()
    {
        m_client->log(m_params.workingDir, m_params.files, m_params.extraOptions);
    }

private:
    BazaarClient *m_client;
    const BazaarCommandParameters m_params;
};

VcsBase::VcsBaseEditorParameterWidget *BazaarClient::createLogEditor(
        const QString &workingDir, const QStringList &files, const QStringList &extraOptions)
{
    const BazaarCommandParameters parameters(workingDir, files, extraOptions);
    return new BazaarLogParameterWidget(this, parameters);
}

} // namespace Internal
} // namespace Bazaar

#include "bazaarclient.moc"
