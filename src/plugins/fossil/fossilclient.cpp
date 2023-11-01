// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fossilclient.h"

#include "constants.h"
#include "fossileditor.h"
#include "fossiltr.h"

#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/process.h>
#include <utils/processenums.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QSyntaxHighlighter>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMap>
#include <QRegularExpression>

using namespace Utils;
using namespace VcsBase;

namespace Fossil {
namespace Internal {

const RunFlags s_pullFlags = RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage;

// Parameter widget controlling whitespace diff mode, associated with a parameter
class FossilDiffConfig : public VcsBaseEditorConfig
{
    Q_OBJECT

public:
    FossilDiffConfig(FossilClient *client, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        QTC_ASSERT(client, return);

        FossilClient::SupportedFeatures features = client->supportedFeatures();

        addReloadButton();
        if (features.testFlag(FossilClient::DiffIgnoreWhiteSpaceFeature)) {
            mapSetting(addToggleButton("-w", Tr::tr("Ignore All Whitespace")),
                       &settings().diffIgnoreAllWhiteSpace);
            mapSetting(addToggleButton("--strip-trailing-cr", Tr::tr("Strip Trailing CR")),
                       &settings().diffStripTrailingCR);
        }
    }
};

// Parameter widget controlling annotate/blame mode
class FossilAnnotateConfig : public VcsBaseEditorConfig
{
    Q_OBJECT

public:
    FossilAnnotateConfig(FossilClient *client, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        QTC_ASSERT(client, return);

        FossilClient::SupportedFeatures features = client->supportedFeatures();

        if (features.testFlag(FossilClient::AnnotateBlameFeature)) {
            mapSetting(addToggleButton("|BLAME|", Tr::tr("Show Committers")),
                       &settings().annotateShowCommitters);
        }

        // Force listVersions setting to false by default.
        // This way the annotated line number would not get offset by the version list.
        settings().annotateListVersions.setValue(false);

        mapSetting(addToggleButton("--log", Tr::tr("List Versions")),
                   &settings().annotateListVersions);
    }
};

class FossilLogCurrentFileConfig : public VcsBaseEditorConfig
{
    Q_OBJECT

public:
    FossilLogCurrentFileConfig(FossilClient *client, QToolBar *toolBar) :
        VcsBaseEditorConfig(toolBar)
    {
        QTC_ASSERT(client, return);
        addReloadButton();
    }
};

class FossilLogConfig : public VcsBaseEditorConfig
{
    Q_OBJECT

public:
    FossilLogConfig(QToolBar *toolBar)
        : VcsBaseEditorConfig(toolBar)
    {
        addReloadButton();
        addLineageComboBox();
        addVerboseToggleButton();
        addItemTypeComboBox();
    }

    void addLineageComboBox()
    {
        // ancestors/descendants filter
        // This is a positional argument not an option.
        // Normally it takes the checkin/branch/tag as an additional parameter
        // (trunk by default)
        // So we kludge this by coding it as a meta-option (pipe-separated),
        // then parse it out in arguments.
        // All-choice is a blank argument with no additional parameters
        const QList<ChoiceItem> lineageFilterChoices = {
            ChoiceItem(Tr::tr("Ancestors"), "ancestors"),
            ChoiceItem(Tr::tr("Descendants"), "descendants"),
            ChoiceItem(Tr::tr("Unfiltered"), "")
        };
        mapSetting(addChoices(Tr::tr("Lineage"), QStringList("|LINEAGE|%1|current"), lineageFilterChoices),
                   &settings().timelineLineageFilter);
    }

    void addVerboseToggleButton()
    {
        // show files
        mapSetting(addToggleButton("-showfiles", Tr::tr("Verbose"),
                                   Tr::tr("Show files changed in each revision")),
                   &settings().timelineVerbose);
    }

    void addItemTypeComboBox()
    {
        // option: -t <val>
        const QList<ChoiceItem> itemTypeChoices = {
            ChoiceItem(Tr::tr("All Items"), "all"),
            ChoiceItem(Tr::tr("File Commits"), "ci"),
            ChoiceItem(Tr::tr("Technical Notes"), "e"),
            ChoiceItem(Tr::tr("Tags"), "g"),
            ChoiceItem(Tr::tr("Tickets"), "t"),
            ChoiceItem(Tr::tr("Wiki Commits"), "w")
        };

        // here we setup the ComboBox to map to the "-t <val>", which will produce
        // the enquoted option-values (e.g "-t all").
        // Fossil expects separate arguments for option and value ( i.e. "-t" "all")
        // so we need to handle the splitting explicitly in arguments().
        mapSetting(addChoices(Tr::tr("Item Types"), QStringList("-t %1"), itemTypeChoices),
                   &settings().timelineItemType);
    }

    QStringList arguments() const final
    {
        QStringList args;

        // split "-t val" => "-t" "val"
        const QStringList arguments = VcsBaseEditorConfig::arguments();
        for (const QString &arg : arguments) {
            if (arg.startsWith("-t")) {
                args << arg.split(' ');

            } else if (arg.startsWith('|')){
                // meta-option: "|OPT|val|extra1|..."
                QStringList params = arg.split('|');
                QString option = params[1];
                for (int i = 2; i < params.size(); ++i) {
                    if (option == "LINEAGE" && params[i].isEmpty()) {
                        // empty lineage filter == Unfiltered
                        break;
                    }
                    args << params[i];
                }
            } else {
                args << arg;
            }
        }
        return args;
    }
};

unsigned FossilClient::makeVersionNumber(int major, int minor, int patch)
{
    return (QString().setNum(major).toUInt(0,16) << 16) +
           (QString().setNum(minor).toUInt(0,16) << 8) +
           (QString().setNum(patch).toUInt(0,16));
}

static inline QString versionPart(unsigned part)
{
    return QString::number(part & 0xff, 16);
}

QString FossilClient::makeVersionString(unsigned version)
{
    return QString::fromLatin1("%1.%2.%3")
                    .arg(versionPart(version >> 16))
                    .arg(versionPart(version >> 8))
                    .arg(versionPart(version));
}

FossilSettings &FossilClient::settings() const
{
    return Internal::settings();
}

FossilClient::FossilClient()
    : VcsBaseClient(&Internal::settings())
{
    setDiffConfigCreator([this](QToolBar *toolBar) {
        return new FossilDiffConfig(this, toolBar);
    });
}

unsigned int FossilClient::synchronousBinaryVersion() const
{
    if (settings().binaryPath().isEmpty())
        return 0;

    const CommandResult result = vcsSynchronousExec({}, QStringList{"version"});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return 0;

    const QString output = result.cleanedStdOut().trimmed();

    // fossil version:
    // "This is fossil version 1.27 [ccdefa355b] 2013-09-30 11:47:18 UTC"
    QRegularExpression versionPattern("(\\d+)\\.(\\d+)");
    QTC_ASSERT(versionPattern.isValid(), return 0);
    QRegularExpressionMatch versionMatch = versionPattern.match(output);
    QTC_ASSERT(versionMatch.hasMatch(), return 0);
    const int major = versionMatch.captured(1).toInt();
    const int minor = versionMatch.captured(2).toInt();
    const int patch = 0;
    return makeVersionNumber(major, minor, patch);
}

QList<BranchInfo> FossilClient::branchListFromOutput(const QString &output,
                                                     const BranchInfo::BranchFlags defaultFlags)
{
    // Branch list format:
    // "  branch-name"
    // "* current-branch"
    return Utils::transform(output.split('\n', Qt::SkipEmptyParts),
                            [=](const QString &l) -> BranchInfo {
        const QString &name = l.mid(2);
        QTC_ASSERT(!name.isEmpty(), return {});
        const BranchInfo::BranchFlags flags = (l.startsWith("* ")
                                               ? defaultFlags | BranchInfo::Current : defaultFlags);
        return {name, flags};
    });
}

BranchInfo FossilClient::synchronousCurrentBranch(const FilePath &workingDirectory)
{
    if (workingDirectory.isEmpty())
        return {};

    // First try to get the current branch from the list of open branches
    const CommandResult result = vcsSynchronousExec(workingDirectory, {"branch", "list"});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return {};

    const QString output = sanitizeFossilOutput(result.cleanedStdOut());
    BranchInfo currentBranch = Utils::findOrDefault(branchListFromOutput(output), [](const BranchInfo &b) {
        return b.isCurrent();
    });

    if (!currentBranch.isCurrent()) {
        // If not available from open branches, request it from the list of closed branches.
        const CommandResult result = vcsSynchronousExec(workingDirectory,
                                                        {"branch", "list", "--closed"});
        if (result.result() != ProcessResult::FinishedWithSuccess)
            return {};

        const QString output = sanitizeFossilOutput(result.cleanedStdOut());
        currentBranch = Utils::findOrDefault(branchListFromOutput(output, BranchInfo::Closed), [](const BranchInfo &b) {
            return b.isCurrent();
        });
    }

    return currentBranch;
}

QList<BranchInfo> FossilClient::synchronousBranchQuery(const FilePath &workingDirectory)
{
    // Return a list of all branches, including the closed ones.
    // Sort the list by branch name.

    if (workingDirectory.isEmpty())
        return {};

    // First get list of open branches
    CommandResult result = vcsSynchronousExec(workingDirectory, {"branch", "list"});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return {};

    QString output = sanitizeFossilOutput(result.cleanedStdOut());
    QList<BranchInfo> branches = branchListFromOutput(output);

    // Append a list of closed branches.
    result = vcsSynchronousExec(workingDirectory, {"branch", "list", "--closed"});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return {};

    output = sanitizeFossilOutput(result.cleanedStdOut());
    branches.append(branchListFromOutput(output, BranchInfo::Closed));

    std::sort(branches.begin(), branches.end(),
          [](const BranchInfo &a, const BranchInfo &b) { return a.name < b.name; });
    return branches;
}

QStringList FossilClient::parseRevisionCommentLine(const QString &commentLine)
{
    // "comment:      This is a (test) commit message (user: the.name)"

    const QRegularExpression commentRx("^comment:\\s+(.*)\\s\\(user:\\s(.*)\\)$",
                                       QRegularExpression::CaseInsensitiveOption);
    QTC_ASSERT(commentRx.isValid(), return {});

    const QRegularExpressionMatch match = commentRx.match(commentLine);
    if (!match.hasMatch())
        return {};

    return {match.captured(1), match.captured(2)};
}

RevisionInfo FossilClient::synchronousRevisionQuery(const FilePath &workingDirectory,
                                                    const QString &id,
                                                    bool getCommentMsg) const
{
    // Query details of the given revision/check-out id,
    // if none specified, provide information about current revision
    if (workingDirectory.isEmpty())
        return {};

    QStringList args("info");
    if (!id.isEmpty())
        args << id;

    const CommandResult result = vcsSynchronousExec(workingDirectory, args,
                                                    RunFlags::SuppressCommandLogging);
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return {};

    const QString output = sanitizeFossilOutput(result.cleanedStdOut());

    QString revisionId;
    QString parentId;
    QStringList mergeParentIds;
    QString commentMsg;
    QString committer;

    const QRegularExpression idRx("([0-9a-f]{5,40})");
    QTC_ASSERT(idRx.isValid(), return {});

    const QString hashToken =
            QString::fromUtf8(supportedFeatures().testFlag(InfoHashFeature) ? "hash: " : "uuid: ");

    for (const QString &l : output.split('\n', Qt::SkipEmptyParts)) {
        if (l.startsWith("checkout: ", Qt::CaseInsensitive)
            || l.startsWith(hashToken, Qt::CaseInsensitive)) {
            const QRegularExpressionMatch idMatch = idRx.match(l);
            QTC_ASSERT(idMatch.hasMatch(), return {});
            revisionId = idMatch.captured(1);

        } else if (l.startsWith("parent: ", Qt::CaseInsensitive)){
            const QRegularExpressionMatch idMatch = idRx.match(l);
            if (idMatch.hasMatch())
                parentId = idMatch.captured(1);
        } else if (l.startsWith("merged-from: ", Qt::CaseInsensitive)) {
            const QRegularExpressionMatch idMatch = idRx.match(l);
            if (idMatch.hasMatch())
                mergeParentIds.append(idMatch.captured(1));
        } else if (getCommentMsg && l.startsWith("comment: ", Qt::CaseInsensitive)) {
            const QStringList commentLineParts = parseRevisionCommentLine(l);
            commentMsg = commentLineParts.value(0);
            committer = commentLineParts.value(1);
        }
    }

    // make sure id at least partially matches the retrieved revisionId
    QTC_ASSERT(revisionId.startsWith(id, Qt::CaseInsensitive), return {});

    if (parentId.isEmpty())
        parentId = revisionId;  // root

    return {revisionId, parentId, mergeParentIds, commentMsg, committer};
}

QStringList FossilClient::synchronousTagQuery(const FilePath &workingDirectory, const QString &id)
{
    // Return a list of tags for the given revision.
    // If no revision specified, all defined tags are listed.
    // Tag list includes branch names.

    if (workingDirectory.isEmpty())
        return {};

    QStringList args({"tag", "list"});
    if (!id.isEmpty())
        args << id;
    const CommandResult result = vcsSynchronousExec(workingDirectory, args);
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return {};

    return sanitizeFossilOutput(result.cleanedStdOut()).split('\n', Qt::SkipEmptyParts);
}

RepositorySettings FossilClient::synchronousSettingsQuery(const FilePath &workingDirectory)
{
    if (workingDirectory.isEmpty())
        return {};

    const CommandResult result = vcsSynchronousExec(workingDirectory, QStringList{"settings"});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return {};
    const QString output = sanitizeFossilOutput(result.cleanedStdOut());

    RepositorySettings repoSettings;
    repoSettings.user = synchronousUserDefaultQuery(workingDirectory);
    if (repoSettings.user.isEmpty())
        repoSettings.user = settings().userName();

    for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
        // parse settings line:
        // <property> <(local|global)> <value>
        // Fossil properties are case-insensitive; force them to lower-case.
        // Values may be in mixed-case; force lower-case for fixed values.
        const QStringList fields = line.split(' ', Qt::SkipEmptyParts);

        const QString property = fields.at(0).toLower();
        const QString value = (fields.size() >= 3 ? fields.at(2) : QString());
        const QString lcValue = value.toLower();

        if (property == "autosync") {
            if (lcValue == "on"
                || lcValue == "1")
                repoSettings.autosync = RepositorySettings::AutosyncOn;
            else if (lcValue == "off"
                     || lcValue == "0")
                repoSettings.autosync = RepositorySettings::AutosyncOff;
            else if (lcValue == "pullonly"
                     || lcValue == "2")
                repoSettings.autosync = RepositorySettings::AutosyncPullOnly;
        }
        else if (property == "ssl-identity") {
            repoSettings.sslIdentityFile = value;
        }
    }

    return repoSettings;
}

bool FossilClient::synchronousSetSetting(const FilePath &workingDirectory, const QString &property,
                                         const QString &value, bool isGlobal)
{
    // set a repository property to the given value
    // if no value is given, unset the property

    if (workingDirectory.isEmpty() || property.isEmpty())
        return false;

    QStringList args;
    if (value.isEmpty())
        args << "unset" << property;
    else
        args << "settings" << property << value;

    if (isGlobal)
        args << "--global";

    return vcsSynchronousExec(workingDirectory, args).result()
            == ProcessResult::FinishedWithSuccess;
}

bool FossilClient::synchronousConfigureRepository(const FilePath &workingDirectory, const RepositorySettings &newSettings,
                                                  const RepositorySettings &currentSettings)
{
    if (workingDirectory.isEmpty())
        return false;

    // apply updated settings vs. current setting if given
    const bool applyAll = (currentSettings == RepositorySettings());

    if (!newSettings.user.isEmpty()
        && (applyAll || newSettings.user != currentSettings.user)
        && !synchronousSetUserDefault(workingDirectory, newSettings.user)) {
        return false;
    }

    if ((applyAll || newSettings.sslIdentityFile != currentSettings.sslIdentityFile)
        && !synchronousSetSetting(workingDirectory, "ssl-identity", newSettings.sslIdentityFile)) {
        return false;
    }

    if (applyAll || newSettings.autosync != currentSettings.autosync) {
        QString value;
        switch (newSettings.autosync) {
        case RepositorySettings::AutosyncOff:
            value = "off";
            break;
        case RepositorySettings::AutosyncOn:
            value = "on";
            break;
        case RepositorySettings::AutosyncPullOnly:
            value = "pullonly";
            break;
        }

        if (!synchronousSetSetting(workingDirectory, "autosync", value))
            return false;
    }

    return true;
}

QString FossilClient::synchronousUserDefaultQuery(const FilePath &workingDirectory)
{
    if (workingDirectory.isEmpty())
        return {};

    const CommandResult result = vcsSynchronousExec(workingDirectory, {"user", "default"});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return {};

    return sanitizeFossilOutput(result.cleanedStdOut()).trimmed();
}

bool FossilClient::synchronousSetUserDefault(const FilePath &workingDirectory, const QString &userName)
{
    if (workingDirectory.isEmpty() || userName.isEmpty())
        return false;

    // set repository-default user
    const QStringList args({"user", "default", userName, "--user", userName});
    return vcsSynchronousExec(workingDirectory, args).result()
            == ProcessResult::FinishedWithSuccess;
}

QString FossilClient::synchronousGetRepositoryURL(const FilePath &workingDirectory)
{
    if (workingDirectory.isEmpty())
        return {};

    const CommandResult result = vcsSynchronousExec(workingDirectory, QStringList{"remote-url"});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return {};
    const QString output = sanitizeFossilOutput(result.cleanedStdOut()).trimmed();

    // Fossil returns "off" when no remote-url is set.
    if (output.toLower() == "off")
        return {};

    return output;
}

QString FossilClient::synchronousTopic(const FilePath &workingDirectory)
{
    if (workingDirectory.isEmpty())
        return {};

    // return current branch name
    return synchronousCurrentBranch(workingDirectory).name;
}

bool FossilClient::synchronousCreateRepository(const FilePath &workingDirectory, const QStringList &extraOptions)
{
    VcsOutputWindow *outputWindow = VcsOutputWindow::instance();

    // init repository file of the same name as the working directory
    // use the configured default repository location for path
    // use the configured default user for admin

    const QString repoName = workingDirectory.fileName().simplified();
    const FilePath repoPath = settings().defaultRepoPath();
    const QString adminUser = settings().userName();

    if (repoName.isEmpty() || repoPath.isEmpty())
        return false;

    // @TODO: handle spaces in the path
    // @TODO: what about --template options?

    const FilePath fullRepoName = FilePath::fromStringWithExtension(repoName, Constants::FOSSIL_FILE_SUFFIX);
    const FilePath repoFilePath = repoPath.pathAppended(fullRepoName.toString());
    QStringList args(vcsCommandString(CreateRepositoryCommand));
    if (!adminUser.isEmpty())
        args << "--admin-user" << adminUser;
    args << extraOptions << repoFilePath.toUserOutput();
    CommandResult result = vcsSynchronousExec(workingDirectory, args);
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return false;
    outputWindow->append(sanitizeFossilOutput(result.cleanedStdOut()));

    // check out the created repository file into the working directory
    // --force as it may be not empty e.g. when creating a project from wizard
    result = vcsSynchronousExec(workingDirectory, {"open", "--force", repoFilePath.toUserOutput()});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return false;
    outputWindow->append(sanitizeFossilOutput(result.cleanedStdOut()));

    // set user default to admin if specified
    if (!adminUser.isEmpty()) {
        result = vcsSynchronousExec(workingDirectory,
                                    {"user", "default", adminUser, "--user", adminUser});
        if (result.result() != ProcessResult::FinishedWithSuccess)
            return false;
        outputWindow->append(sanitizeFossilOutput(result.cleanedStdOut()));
    }

    resetCachedVcsInfo(workingDirectory);
    return true;
}

bool FossilClient::synchronousMove(const FilePath &workingDir,
                                   const QString &from, const QString &to,
                                   const QStringList &extraOptions)
{
    // Fossil move does not rename actual file on disk, only changes it in repo
    // So try to move the actual file first, then move it in repo to preserve
    // history in case actual move fails.

    if (!QFile::rename(from, to))
        return false;

    QStringList args(vcsCommandString(MoveCommand));
    args << extraOptions << from << to;
    return vcsSynchronousExec(workingDir, args).result() == ProcessResult::FinishedWithSuccess;
}

bool FossilClient::synchronousPull(const FilePath &workingDir, const QString &srcLocation, const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(PullCommand));
    if (srcLocation.isEmpty()) {
        const QString defaultURL(synchronousGetRepositoryURL(workingDir));
        if (defaultURL.isEmpty())
            return false;
    } else {
        args << srcLocation;
    }

    args << extraOptions;
    const CommandResult result = vcsSynchronousExec(workingDir, args, s_pullFlags);
    const bool success = (result.result() == ProcessResult::FinishedWithSuccess);
    if (success)
        emit changed(workingDir.toVariant());
    return success;
}

bool FossilClient::synchronousPush(const FilePath &workingDir, const QString &dstLocation, const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(PushCommand));
    if (dstLocation.isEmpty()) {
        const QString defaultURL(synchronousGetRepositoryURL(workingDir));
        if (defaultURL.isEmpty())
            return false;
    } else {
        args << dstLocation;
    }

    args << extraOptions;
    return vcsSynchronousExec(workingDir, args, s_pullFlags).result()
            == ProcessResult::FinishedWithSuccess;
}

void FossilClient::commit(const FilePath &repositoryRoot, const QStringList &files,
                          const QString &commitMessageFile, const QStringList &extraOptions)
{
    VcsBaseClient::commit(repositoryRoot, files, commitMessageFile,
                          QStringList(extraOptions) << "-M" << commitMessageFile);
}

void FossilClient::annotate(const FilePath &workingDir, const QString &file, int lineNumber,
                            const QString &revision, const QStringList &extraOptions, int firstLine)
{
    Q_UNUSED(firstLine)
    // 'fossil annotate' command has a variant 'fossil blame'.
    // blame command attributes a committing username to source lines,
    // annotate shows line numbers

    QString vcsCmdString = vcsCommandString(AnnotateCommand);
    const Id kind = vcsEditorKind(AnnotateCommand);
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(file), revision);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const FilePath source = VcsBaseEditor::getSource(workingDir, file);

    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getCodec(source),
                                                  vcsCmdString.toLatin1().constData(), id);

    auto *fossilEditor = qobject_cast<FossilEditorWidget *>(editor);
    QTC_ASSERT(fossilEditor, return);

    if (!fossilEditor->editorConfig()) {
        if (VcsBaseEditorConfig *editorConfig = createAnnotateEditor(fossilEditor)) {
            editorConfig->setBaseArguments(extraOptions);
            // editor has been just created, createVcsEditor() didn't set a configuration widget yet
            connect(editorConfig, &VcsBaseEditorConfig::commandExecutionRequested, this, [=] {
                const int line = VcsBaseEditor::lineNumberOfCurrentEditor();
                annotate(workingDir, file, line, revision, editorConfig->arguments());
            });
            fossilEditor->setEditorConfig(editorConfig);
        }
    }
    QStringList effectiveArgs = extraOptions;
    if (VcsBaseEditorConfig *editorConfig = fossilEditor->editorConfig())
        effectiveArgs = editorConfig->arguments();

    // here we introduce a "|BLAME|" meta-option to allow both annotate and blame modes
    int pos = effectiveArgs.indexOf("|BLAME|");
    if (pos != -1) {
        vcsCmdString = "blame";
        effectiveArgs.removeAt(pos);
    }
    QStringList args(vcsCmdString);
    if (!revision.isEmpty() && supportedFeatures().testFlag(AnnotateRevisionFeature))
        args << "-r" << revision;
    args << effectiveArgs << file;

    // When version list requested, ignore the source line.
    if (args.contains("--log"))
        lineNumber = -1;
    editor->setDefaultLineNumber(lineNumber);

    enqueueJob(createCommand(workingDir, fossilEditor), args);
}

bool FossilClient::isVcsFileOrDirectory(const FilePath &filePath) const
{
    // false for any dir or file other than fossil checkout db-file
    return filePath.toFileInfo().isFile()
           && !filePath.fileName().compare(Constants::FOSSILREPO,
                                           HostOsInfo::fileNameCaseSensitivity());
}

FilePath FossilClient::findTopLevelForFile(const FilePath &file) const
{
    return findRepositoryForFile(file, Constants::FOSSILREPO);
}

bool FossilClient::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    const CommandResult result = vcsSynchronousExec(workingDirectory, {"finfo", fileName});
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return false;
    QString output = sanitizeFossilOutput(result.cleanedStdOut());
    return !output.startsWith("no history for file", Qt::CaseInsensitive);
}

unsigned int FossilClient::binaryVersion() const
{
    static unsigned int cachedBinaryVersion = 0;
    static FilePath cachedBinaryPath;

    const FilePath currentBinaryPath = settings().binaryPath();

    if (currentBinaryPath.isEmpty())
        return 0;

    // Invalidate cache on failed version result.
    // Assume that fossil client options have been changed and will change again.
    if (!cachedBinaryVersion || currentBinaryPath != cachedBinaryPath) {
        cachedBinaryVersion = synchronousBinaryVersion();
        if (cachedBinaryVersion)
            cachedBinaryPath = currentBinaryPath;
        else
            cachedBinaryPath.clear();
    }

    return cachedBinaryVersion;
}

QString FossilClient::binaryVersionString() const
{
    const unsigned int version = binaryVersion();

    // Fossil itself does not report patch version, only maj.min
    // Here we include the patch part for general convention consistency

    return makeVersionString(version);
}

FossilClient::SupportedFeatures FossilClient::supportedFeatures() const
{
    // use for legacy client support to test for feature presence
    // e.g. supportedFeatures().testFlag(TimelineWidthFeature)

    SupportedFeatures features = AllSupportedFeatures; // all inclusive by default (~0U)

    const unsigned int version = binaryVersion();

    if (version < 0x21200) {
        features &= ~InfoHashFeature;
        if (version < 0x20400)
            features &= ~AnnotateRevisionFeature;
        if (version < 0x13000)
            features &= ~TimelinePathFeature;
        if (version < 0x12900)
            features &= ~DiffIgnoreWhiteSpaceFeature;
        if (version < 0x12800) {
            features &= ~AnnotateBlameFeature;
            features &= ~TimelineWidthFeature;
        }
    }

    return features;
}

void FossilClient::view(const FilePath &source, const QString &id, const QStringList &extraOptions)
{
    const FilePath workingDirectory = source.isFile() ? source.absolutePath() : source;

    const RevisionInfo revisionInfo = synchronousRevisionQuery(workingDirectory, id);
    const QStringList args{"diff", "--from", revisionInfo.parentId, "--to", revisionInfo.id, "-v"};
    const Id kind = vcsEditorKind(DiffCommand);
    const QString title = vcsEditorTitle(vcsCommandString(DiffCommand), id);

    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getCodec(source), "view", id);
    editor->setWorkingDirectory(workingDirectory);

    enqueueJob(createCommand(workingDirectory, editor), args + extraOptions);
}

class FossilLogHighlighter : QSyntaxHighlighter
{
public:
    explicit FossilLogHighlighter(QTextDocument *parent);
    virtual void highlightBlock(const QString &text) final;

private:
    const QRegularExpression m_revisionIdRx;
    const QRegularExpression m_dateRx;
};

FossilLogHighlighter::FossilLogHighlighter(QTextDocument * parent) :
    QSyntaxHighlighter(parent),
    m_revisionIdRx(Constants::CHANGESET_ID),
    m_dateRx("([0-9]{4}-[0-9]{2}-[0-9]{2})")
{
    QTC_CHECK(m_revisionIdRx.isValid());
    QTC_CHECK(m_dateRx.isValid());
}

void FossilLogHighlighter::highlightBlock(const QString &text)
{
    // Match the revision-ids and dates -- highlight them for convenience.

    // Format revision-ids
    QRegularExpressionMatchIterator i = m_revisionIdRx.globalMatch(text);
    while (i.hasNext()) {
        const QRegularExpressionMatch revisionIdMatch = i.next();
        QTextCharFormat charFormat = format(0);
        charFormat.setForeground(Qt::darkBlue);
        //charFormat.setFontItalic(true);
        setFormat(revisionIdMatch.capturedStart(0), revisionIdMatch.capturedLength(0), charFormat);
    }

    // Format dates
    i = m_dateRx.globalMatch(text);
    while (i.hasNext()) {
        const QRegularExpressionMatch dateMatch = i.next();
        QTextCharFormat charFormat = format(0);
        charFormat.setForeground(Qt::darkBlue);
        charFormat.setFontWeight(QFont::DemiBold);
        setFormat(dateMatch.capturedStart(0), dateMatch.capturedLength(0), charFormat);
    }
}

void FossilClient::log(const FilePath &workingDir, const QStringList &files,
                       const QStringList &extraOptions,
                       bool enableAnnotationContextMenu,
                       const std::function<void(CommandLine &)> &addAuthOptions)
{
    // Show timeline for both repository and a file or path (--path <file-or-path>)
    // When used for log repository, the files list is empty

    // LEGACY:fallback to log current file with legacy clients
    SupportedFeatures features = supportedFeatures();
    if (!files.isEmpty()
        && !features.testFlag(TimelinePathFeature)) {
        logCurrentFile(workingDir, files, extraOptions, enableAnnotationContextMenu, addAuthOptions);
        return;
    }

    const QString vcsCmdString = vcsCommandString(LogCommand);
    const Id kind = vcsEditorKind(LogCommand);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const FilePath source = VcsBaseEditor::getSource(workingDir, files);
    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getCodec(source),
                                                  vcsCmdString.toLatin1().constData(), id);

    auto *fossilEditor = qobject_cast<FossilEditorWidget *>(editor);
    QTC_ASSERT(fossilEditor, return);

    fossilEditor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);

    if (!fossilEditor->editorConfig()) {
        if (VcsBaseEditorConfig *editorConfig = createLogEditor(fossilEditor)) {
            editorConfig->setBaseArguments(extraOptions);
            // editor has been just created, createVcsEditor() didn't set a configuration widget yet
            connect(editorConfig, &VcsBaseEditorConfig::commandExecutionRequested, this, [=] {
                log(workingDir, files, editorConfig->arguments(), enableAnnotationContextMenu,
                    addAuthOptions);
            });
            fossilEditor->setEditorConfig(editorConfig);
        }
    }
    QStringList effectiveArgs = extraOptions;
    if (VcsBaseEditorConfig *editorConfig = fossilEditor->editorConfig())
        effectiveArgs = editorConfig->arguments();

    //@TODO: move highlighter and widgets to fossil editor sources.

    new FossilLogHighlighter(fossilEditor->document());

    QStringList args(vcsCmdString);
    args << effectiveArgs;
    if (!files.isEmpty())
         args << "--path" << files;
    enqueueJob(createCommand(workingDir, fossilEditor), args);
}

void FossilClient::logCurrentFile(const FilePath &workingDir, const QStringList &files,
                                  const QStringList &extraOptions,
                                  bool enableAnnotationContextMenu,
                                  const std::function<void(CommandLine &)> &addAuthOptions)
{
    // Show commit history for the given file/file-revision
    // NOTE: 'fossil finfo' shows full history from all branches.

    // With newer clients, 'fossil timeline' can handle both repository and file
    SupportedFeatures features = supportedFeatures();
    if (features.testFlag(TimelinePathFeature)) {
        log(workingDir, files, extraOptions, enableAnnotationContextMenu, addAuthOptions);
        return;
    }

    const QString vcsCmdString = "finfo";
    const Id kind = vcsEditorKind(LogCommand);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const FilePath source = VcsBaseEditor::getSource(workingDir, files);
    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getCodec(source),
                                                  vcsCmdString.toLatin1().constData(), id);

    auto *fossilEditor = qobject_cast<FossilEditorWidget *>(editor);
    QTC_ASSERT(fossilEditor, return);

    fossilEditor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);

    if (!fossilEditor->editorConfig()) {
        if (VcsBaseEditorConfig *editorConfig = createLogCurrentFileEditor(fossilEditor)) {
            editorConfig->setBaseArguments(extraOptions);
            // editor has been just created, createVcsEditor() didn't set a configuration widget yet
            connect(editorConfig, &VcsBaseEditorConfig::commandExecutionRequested, this, [=] {
                logCurrentFile(workingDir, files, editorConfig->arguments(),
                               enableAnnotationContextMenu, addAuthOptions);
            });
            fossilEditor->setEditorConfig(editorConfig);
        }
    }
    QStringList effectiveArgs = extraOptions;
    if (VcsBaseEditorConfig *editorConfig = fossilEditor->editorConfig())
        effectiveArgs = editorConfig->arguments();

    //@TODO: move highlighter and widgets to fossil editor sources.

    new FossilLogHighlighter(fossilEditor->document());

    QStringList args(vcsCmdString);
    args << effectiveArgs << files;
    enqueueJob(createCommand(workingDir, fossilEditor), args);
}

void FossilClient::revertFile(const FilePath &workingDir,
                              const QString &file,
                              const QString &revision,
                              const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    if (!revision.isEmpty())
        args << "-r" << revision;
    args << extraOptions << file;

    // Indicate file list
    VcsCommand *cmd = createCommand(workingDir);
    const QStringList files = {workingDir.toString() + "/" + file};
    connect(cmd, &VcsCommand::done, this, [this, files, cmd] {
        if (cmd->result() == ProcessResult::FinishedWithSuccess)
            emit changed(files);
    });
    enqueueJob(cmd, args);
}

void FossilClient::revertAll(const FilePath &workingDir, const QString &revision, const QStringList &extraOptions)
{
    // Fossil allows whole tree revert to latest revision (effectively undoing uncommitted changes).
    // However it disallows revert to a specific revision for the whole tree, only for selected files.
    // Use checkout --force command for such case.
    // NOTE: all uncommitted changes will not be backed up by checkout, unlike revert.
    //       Thus undo for whole tree revert should not be possible.

    QStringList args;
    if (revision.isEmpty())
        args << vcsCommandString(RevertCommand) << extraOptions;
    else
        args << "checkout" << revision << "--force" << extraOptions;

    // Indicate repository change
    VcsCommand *cmd = createCommand(workingDir);
    const QStringList files = QStringList(workingDir.toString());
    connect(cmd, &VcsCommand::done, this, [this, files, cmd] {
        if (cmd->result() == ProcessResult::FinishedWithSuccess)
            emit changed(files);
    });
    enqueueJob(createCommand(workingDir), args);
}

QString FossilClient::sanitizeFossilOutput(const QString &output) const
{
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
    // Strip possible extra '\r' in output from the Fossil client on Windows.

    // Fossil client contained a long-standing bug which caused an extraneous '\r'
    // added to output lines from certain commands in excess of the expected <CR/LF>.
    // While the output appeared normal on a terminal, in non-interactive context
    // it would get incorrectly split, resulting in extra empty lines.
    // Bug fix is fairly recent, so for compatibility we need to strip the '\r'.
    QString result(output);
    return result.remove('\r');
#else
    return output;
#endif
}

QString FossilClient::vcsCommandString(VcsCommandTag cmd) const
{
    // override specific client commands
    // otherwise return baseclient command

    switch (cmd) {
    case RemoveCommand: return "rm";
    case MoveCommand: return "mv";
    case LogCommand: return "timeline";
    default: return VcsBaseClient::vcsCommandString(cmd);
    }
}

Id FossilClient::vcsEditorKind(VcsCommandTag cmd) const
{
    switch (cmd) {
    case AnnotateCommand:
        return Constants::ANNOTATELOG_ID;
    case DiffCommand:
        return Constants::DIFFLOG_ID;
    case LogCommand:
        return Constants::FILELOG_ID;
    default:
        return {};
    }
}

QStringList FossilClient::revisionSpec(const QString &revision) const
{
    // Pass the revision verbatim.
    // Fossil uses a variety of ways to spec the revisions.
    // In most cases revision is passed directly (SHA1) or via tag.
    // Tag name may need to be prefixed with tag: to disambiguate it from hex (beef).
    // Handle the revision option per specific command (e.g. diff, revert ).

    QStringList args;
    if (!revision.isEmpty())
        args << revision;
    return args;
}

FossilClient::StatusItem FossilClient::parseStatusLine(const QString &line) const
{
    StatusItem item;

    // Ref: fossil source 'src/checkin.c' status_report()
    // Expect at least one non-leading blank space.

    int pos = line.indexOf(' ');

    if (line.isEmpty() || pos < 1)
        return {};

    QString label(line.left(pos));
    QString flags;

    if (label == "EDITED")
        flags = Constants::FSTATUS_EDITED;
    else if (label == "ADDED")
        flags = Constants::FSTATUS_ADDED;
    else if (label == "RENAMED")
        flags = Constants::FSTATUS_RENAMED;
    else if (label == "DELETED")
        flags = Constants::FSTATUS_DELETED;
    else if (label == "MISSING")
        flags = "Missing";
    else if (label == "ADDED_BY_MERGE")
        flags = Constants::FSTATUS_ADDED_BY_MERGE;
    else if (label == "UPDATED_BY_MERGE")
        flags = Constants::FSTATUS_UPDATED_BY_MERGE;
    else if (label == "ADDED_BY_INTEGRATE")
        flags = Constants::FSTATUS_ADDED_BY_INTEGRATE;
    else if (label == "UPDATED_BY_INTEGRATE")
        flags = Constants::FSTATUS_UPDATED_BY_INTEGRATE;
    else if (label == "CONFLICT")
        flags = "Conflict";
    else if (label == "EXECUTABLE")
        flags = "Set Exec";
    else if (label == "SYMLINK")
        flags = "Set Symlink";
    else if (label == "UNEXEC")
        flags = "Unset Exec";
    else if (label == "UNLINK")
        flags = "Unset Symlink";
    else if (label == "NOT_A_FILE")
        flags = Constants::FSTATUS_UNKNOWN;

    if (flags.isEmpty())
        return {};

    // adjust the position to the last space before the file name
    for (int size = line.size(); (pos + 1) < size && line[pos + 1].isSpace(); ++pos)
        ;

    item.flags = flags;
    item.file = line.mid(pos + 1);
    return item;
}

VcsBaseEditorConfig *FossilClient::createAnnotateEditor(VcsBaseEditorWidget *editor)
{
    return new FossilAnnotateConfig(this, editor->toolBar());
}

VcsBaseEditorConfig *FossilClient::createLogCurrentFileEditor(VcsBaseEditorWidget *editor)
{
    SupportedFeatures features = supportedFeatures();

    if (features.testFlag(TimelinePathFeature))
        return createLogEditor(editor);

    return new FossilLogCurrentFileConfig(this, editor->toolBar());
}

VcsBaseEditorConfig *FossilClient::createLogEditor(VcsBaseEditorWidget *editor)
{
    return new FossilLogConfig(editor->toolBar());
}

} // namespace Internal
} // namespace Fossil

#include "fossilclient.moc"
