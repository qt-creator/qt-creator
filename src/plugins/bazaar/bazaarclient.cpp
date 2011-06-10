/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "bazaarclient.h"
#include "constants.h"

#include <vcsbase/vcsbaseclientsettings.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>

#include <utils/qtcassert.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QtDebug>

namespace Bazaar {
namespace Internal {

BazaarClient::BazaarClient(BazaarSettings *settings) :
    VCSBase::VCSBaseClient(settings)
{
}

BazaarSettings *BazaarClient::settings() const
{
    return dynamic_cast<BazaarSettings *>(VCSBase::VCSBaseClient::settings());
}

bool BazaarClient::synchronousSetUserId()
{
    QStringList args;
    args << QLatin1String("whoami")
         << QString("%1 <%2>").arg(settings()->userName()).arg(settings()->email());
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
    const QRegExp branchLocationRx("bound_location\\s*=\\s*(.+)$");
    const QRegExp isBranchBoundRx("bound\\s*=\\s*(.+)$");
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

QString BazaarClient::findTopLevelForFile(const QFileInfo &file) const
{
    const QString repositoryCheckFile =
            QLatin1String(Constants::BAZAARREPO) + QLatin1String("/branch-format");
    return file.isDir() ?
                VCSBase::VCSBasePlugin::findRepositoryForDirectory(file.absoluteFilePath(),
                                                                   repositoryCheckFile) :
                VCSBase::VCSBasePlugin::findRepositoryForDirectory(file.absolutePath(),
                                                                   repositoryCheckFile);
}

QString BazaarClient::vcsEditorKind(VCSCommand cmd) const
{
    switch(cmd) {
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

QStringList BazaarClient::cloneArguments(const QString &srcLocation,
                                         const QString &dstLocation,
                                         const QStringList &extraOptions) const
{
    QStringList args(extraOptions);
    args << srcLocation;
    if (!dstLocation.isEmpty())
        args << dstLocation;
    return args;
}

QStringList BazaarClient::pullArguments(const QString &srcLocation,
                                        const QStringList &extraOptions) const
{
    QStringList args(extraOptions);
    if (!srcLocation.isEmpty())
        args << srcLocation;
    return args;
}

QStringList BazaarClient::pushArguments(const QString &dstLocation,
                                        const QStringList &extraOptions) const
{
    QStringList args(extraOptions);
    if (!dstLocation.isEmpty())
        args << dstLocation;
    return args;
}

QStringList BazaarClient::commitArguments(const QStringList &files,
                                          const QString &commitMessageFile,
                                          const QStringList &extraOptions) const
{
    QStringList args(extraOptions);
    args << QLatin1String("-F") << commitMessageFile;
    args << files;
    return args;
}

QStringList BazaarClient::importArguments(const QStringList &files) const
{
    QStringList args;
    if (!files.isEmpty())
        args.append(files);
    return args;
}

QStringList BazaarClient::updateArguments(const QString &revision) const
{
    QStringList args;
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    return args;
}

QStringList BazaarClient::revertArguments(const QString &file,
                                          const QString &revision) const
{
    QStringList args;
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    if (!file.isEmpty())
        args << file;
    return args;
}

QStringList BazaarClient::revertAllArguments(const QString &revision) const
{
    QStringList args;
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    return args;
}

QStringList BazaarClient::annotateArguments(const QString &file,
                                            const QString &revision,
                                            int lineNumber) const
{
    Q_UNUSED(lineNumber);
    QStringList args(QLatin1String("--long"));
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    return args << file;
}

QStringList BazaarClient::diffArguments(const QStringList &files,
                                        const QStringList &extraOptions) const
{
    QStringList args(extraOptions);
    if (!files.isEmpty())
        args.append(files);
    return args;
}

QStringList BazaarClient::logArguments(const QStringList &files,
                                       const QStringList &extraOptions) const
{
    return diffArguments(files, extraOptions);
}

QStringList BazaarClient::statusArguments(const QString &file) const
{
    QStringList args;
    args.append(QLatin1String("--short"));
    if (!file.isEmpty())
        args.append(file);
    return args;
}

QStringList BazaarClient::viewArguments(const QString &revision) const
{
    QStringList args(QLatin1String("log"));
    args << QLatin1String("-p") << QLatin1String("-v")
         << QLatin1String("-r") << revision;
    return args;
}

QPair<QString, QString> BazaarClient::parseStatusLine(const QString &line) const
{
    QPair<QString, QString> status;
    if (!line.isEmpty()) {
        const QChar flagVersion = line[0];
        if (flagVersion == QLatin1Char('+'))
            status.first = QLatin1String("Versioned");
        else if (flagVersion == QLatin1Char('-'))
            status.first = QLatin1String("Unversioned");
        else if (flagVersion == QLatin1Char('R'))
            status.first = QLatin1String("Renamed");
        else if (flagVersion == QLatin1Char('?'))
            status.first = QLatin1String("Unknown");
        else if (flagVersion == QLatin1Char('X'))
            status.first = QLatin1String("Nonexistent");
        else if (flagVersion == QLatin1Char('C'))
            status.first = QLatin1String("Conflict");
        else if (flagVersion == QLatin1Char('P'))
            status.first = QLatin1String("PendingMerge");

        const int lineLength = line.length();
        if (lineLength >= 2) {
            const QChar flagContents = line[1];
            if (flagContents == QLatin1Char('N'))
                status.first = QLatin1String("Created");
            else if (flagContents == QLatin1Char('D'))
                status.first = QLatin1String("Deleted");
            else if (flagContents == QLatin1Char('K'))
                status.first = QLatin1String("KindChanged");
            else if (flagContents == QLatin1Char('M'))
                status.first = QLatin1String("Modified");
        }
        if (lineLength >= 3) {
            const QChar flagExec = line[2];
            if (flagExec == QLatin1Char('*'))
                status.first = QLatin1String("ExecuteBitChanged");
        }
        // The status string should be similar to "xxx file_with_changes"
        // so just should take the file name part and store it
        status.second = line.mid(4);
    }
    return status;
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
class BazaarDiffParameterWidget : public VCSBase::VCSBaseEditorParameterWidget
{
    Q_OBJECT
public:
    BazaarDiffParameterWidget(BazaarClient *client,
                              const BazaarCommandParameters &p, QWidget *parent = 0) :
        VCSBase::VCSBaseEditorParameterWidget(parent), m_client(client), m_params(p)
    {
        mapSetting(addToggleButton(QLatin1String("-w"), tr("Ignore whitespace")),
                   &client->settings()->diffIgnoreWhiteSpace);
        mapSetting(addToggleButton(QLatin1String("-B"), tr("Ignore blank lines")),
                   &client->settings()->diffIgnoreBlankLines);
    }

    QStringList arguments() const
    {
        QStringList args;
        // Bazaar wants "--diff-options=-w -B.."
        const QStringList formatArguments = VCSBaseEditorParameterWidget::arguments();
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

VCSBase::VCSBaseEditorParameterWidget *BazaarClient::createDiffEditor(
    const QString &workingDir, const QStringList &files, const QStringList &extraOptions)
{
    const BazaarCommandParameters parameters(workingDir, files, extraOptions);
    return new BazaarDiffParameterWidget(this, parameters);
}

} //namespace Internal
} // namespace Bazaar

#include "bazaarclient.moc"
