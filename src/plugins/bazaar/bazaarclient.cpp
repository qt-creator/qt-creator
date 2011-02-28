/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "bazaarclient.h"
#include "constants.h"

#include <vcsbase/vcsbaseplugin.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QtDebug>

namespace {

void addBoolArgument(const QVariant &optionValue,
                     const QString &optionName,
                     QStringList *arguments)
{
    if (arguments == 0)
        return;
    Q_ASSERT(optionValue.canConvert(QVariant::Bool));
    if (optionValue.toBool())
        arguments->append(optionName);
}

void addRevisionArgument(const QVariant &optionValue, QStringList *arguments)
{
    if (arguments == 0)
        return;
    Q_ASSERT(optionValue.canConvert(QVariant::String));
    const QString revision = optionValue.toString();
    if (!revision.isEmpty())
        (*arguments) << QLatin1String("-r") << revision;
}

} // Anonymous namespace

namespace Bazaar {
namespace Internal {

BazaarClient::BazaarClient(const VCSBase::VCSBaseClientSettings &settings) :
    VCSBase::VCSBaseClient(settings)
{
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
                                         const ExtraCommandOptions &extraOptions) const
{
    QStringList args;
    // Fetch extra options
    foreach (int iOption, extraOptions.keys()) {
        const QVariant iOptValue = extraOptions[iOption];
        switch (iOption) {
        case UseExistingDirCloneOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--use-existing-dir"), &args);
            break;
        case StackedCloneOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--stacked"), &args);
            break;
        case StandAloneCloneOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--standalone"), &args);
            break;
        case BindCloneOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--bind"), &args);
            break;
        case SwitchCloneOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--switch"), &args);
            break;
        case HardLinkCloneOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--hardlink"), &args);
            break;
        case NoTreeCloneOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--no-tree"), &args);
            break;
        case RevisionCloneOptionId :
            ::addRevisionArgument(iOptValue, &args);
            break;
        default :
            Q_ASSERT(false); // Invalid option !
        }
    } // end foreach ()
    // Add arguments for common options
    args << srcLocation;
    if (!dstLocation.isEmpty())
        args << dstLocation;
    return args;
}

QStringList BazaarClient::pullArguments(const QString &srcLocation,
                                        const ExtraCommandOptions &extraOptions) const
{
    // Fetch extra options
    QStringList args(commonPullOrPushArguments(extraOptions));
    foreach (int iOption, extraOptions.keys()) {
        const QVariant iOptValue = extraOptions[iOption];
        switch (iOption) {
        case RememberPullOrPushOptionId : break;
        case OverwritePullOrPushOptionId : break;
        case RevisionPullOrPushOptionId : break;
        case LocalPullOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--local"), &args);
            break;
        default :
            Q_ASSERT(false); // Invalid option !
        }
    } // end foreach ()
    // Add arguments for common options
    if (!srcLocation.isEmpty())
        args << srcLocation;
    return args;
}

QStringList BazaarClient::pushArguments(const QString &dstLocation,
                                        const ExtraCommandOptions &extraOptions) const
{
    // Fetch extra options
    QStringList args(commonPullOrPushArguments(extraOptions));
    foreach (int iOption, extraOptions.keys()) {
        const QVariant iOptValue = extraOptions[iOption];
        switch (iOption) {
        case RememberPullOrPushOptionId : break;
        case OverwritePullOrPushOptionId : break;
        case RevisionPullOrPushOptionId : break;
        case UseExistingDirPushOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--use-existing-dir"), &args);
            break;
        case CreatePrefixPushOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--create-prefix"), &args);
            break;
        default :
            Q_ASSERT(false); // Invalid option !
        }
    } // end foreach ()
    // Add arguments for common options
    if (!dstLocation.isEmpty())
        args << dstLocation;
    return args;
}

QStringList BazaarClient::commitArguments(const QStringList &files,
                                          const QString &commitMessageFile,
                                          const ExtraCommandOptions &extraOptions) const
{
    QStringList args;
    // Fetch extra options
    foreach (int iOption, extraOptions.keys()) {
        const QVariant iOptValue = extraOptions[iOption];
        switch (iOption) {
        case AuthorCommitOptionId :
        {
            Q_ASSERT(iOptValue.canConvert(QVariant::String));
            const QString committerInfo = iOptValue.toString();
            if (!committerInfo.isEmpty())
                args << QString("--author=%1").arg(committerInfo);
            break;
        }
        case FixesCommitOptionId :
        {
            Q_ASSERT(iOptValue.canConvert(QVariant::StringList));
            foreach (const QString& iFix, iOptValue.toStringList()) {
                if (!iFix.isEmpty())
                    args << QLatin1String("--fixes") << iFix;
            }
            break;
        }
        case LocalCommitOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--local"), &args);
            break;
        default :
            Q_ASSERT(false); // Invalid option !
        }
    } // end foreach ()
    // Add arguments for common options
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

QStringList BazaarClient::diffArguments(const QStringList &files) const
{
    QStringList args;
    if (!files.isEmpty())
        args.append(files);
    return args;
}

QStringList BazaarClient::logArguments(const QStringList &files) const
{
    return diffArguments(files);
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
    args << QLatin1String("-p") << QLatin1String("--gnu-changelog")
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

QStringList BazaarClient::commonPullOrPushArguments(const ExtraCommandOptions &extraOptions) const
{
    QStringList args;
    foreach (int iOption, extraOptions.keys()) {
        const QVariant iOptValue = extraOptions[iOption];
        switch (iOption) {
        case RememberPullOrPushOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--remember"), &args);
            break;
        case OverwritePullOrPushOptionId :
            ::addBoolArgument(iOptValue, QLatin1String("--overwrite"), &args);
            break;
        case RevisionPullOrPushOptionId :
            ::addRevisionArgument(iOptValue, &args);
            break;
        default :
            break; // Unknown option, do nothing
        }
    } // end foreach ()
    return args;
}

} //namespace Internal
} // namespace Bazaar
