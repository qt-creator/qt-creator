// Copyright (C) 2016 Petar Perisin.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gerritpushdialog.h"

#include "branchcombobox.h"
#include "gerritremotechooser.h"

#include "../gitclient.h"
#include "../gitconstants.h"
#include "../gittr.h"
#include "../logchangedialog.h"

#include <utils/icon.h>
#include <utils/layoutbuilder.h>
#include <utils/theme/theme.h>

#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QVersionNumber>

using namespace Git::Internal;

namespace Gerrit {
namespace Internal {

static const int ReasonableDistance = 100;

class PushItemDelegate : public IconItemDelegate
{
public:
    PushItemDelegate(LogChangeWidget *widget)
        : IconItemDelegate(widget, Utils::Icon(":/git/images/arrowup.png"))
    {
    }

protected:
    bool hasIcon(int row) const override
    {
        return row >= currentRow();
    }
};

QString GerritPushDialog::determineRemoteBranch(const QString &localBranch)
{
    const QString earliestCommit = m_commitView->earliestCommit();
    if (earliestCommit.isEmpty())
        return {};

    QString output;
    QString error;

    if (!gitClient().synchronousBranchCmd(
                m_workingDir, {"-r", "--contains", earliestCommit + '^'}, &output, &error)) {
        return {};
    }
    const QString head = "/HEAD";
    const QStringList refs = output.split('\n');

    QString remoteTrackingBranch;
    if (localBranch != "HEAD")
        remoteTrackingBranch = gitClient().synchronousTrackingBranch(m_workingDir, localBranch);

    QString remoteBranch;
    for (const QString &reference : refs) {
        const QString ref = reference.trimmed();
        if (ref.contains(head) || ref.isEmpty())
            continue;

        if (remoteBranch.isEmpty())
            remoteBranch = ref;

        // Prefer remote tracking branch if it exists and contains the latest remote commit
        if (ref == remoteTrackingBranch)
            return ref;
    }
    return remoteBranch;
}

void GerritPushDialog::initRemoteBranches()
{
    QString output;
    const QString head = "/HEAD";

    const QString remotesPrefix("refs/remotes/");
    if (!gitClient().synchronousForEachRefCmd(
                m_workingDir, {"--format=%(refname)\t%(committerdate:raw)", remotesPrefix}, &output)) {
        return;
    }

    const QStringList refs = output.split("\n");
    for (const QString &reference : refs) {
        const QStringList entries = reference.split('\t');
        if (entries.count() < 2 || entries.first().endsWith(head))
            continue;
        const QString ref = entries.at(0).mid(remotesPrefix.size());
        int refBranchIndex = ref.indexOf('/');
        qint64 timeT = entries.at(1).left(entries.at(1).indexOf(' ')).toLongLong();
        BranchDate bd(ref.mid(refBranchIndex + 1), QDateTime::fromSecsSinceEpoch(timeT).date());
        m_remoteBranches.insertMulti(ref.left(refBranchIndex), bd);
    }
    m_remoteComboBox->updateRemotes(false);
}

GerritPushDialog::GerritPushDialog(const Utils::FilePath &workingDir, const QString &reviewerList,
                                   QSharedPointer<GerritParameters> parameters, QWidget *parent)
    : QDialog(parent)
    , m_localBranchComboBox(new BranchComboBox)
    , m_remoteComboBox(new GerritRemoteChooser)
    , m_targetBranchComboBox(new QComboBox)
    , m_commitView(new LogChangeWidget)
    , m_infoLabel(new QLabel(::Git::Tr::tr("Number of commits")))
    , m_topicLineEdit(new QLineEdit)
    , m_draftCheckBox(new QCheckBox(::Git::Tr::tr("&Draft/private")))
    , m_wipCheckBox(new QCheckBox(::Git::Tr::tr("&Work-in-progress")))
    , m_reviewersLineEdit(new QLineEdit)
    , m_buttonBox(new QDialogButtonBox)
    , m_workingDir(workingDir)
{
    m_draftCheckBox->setToolTip(::Git::Tr::tr("Checked - Mark change as private.\n"
                                              "Unchecked - Remove mark.\n"
                                              "Partially checked - Do not change current state."));
    m_commitView->setToolTip(::Git::Tr::tr(
            "Pushes the selected commit and all commits it depends on."));
    m_reviewersLineEdit->setToolTip(::Git::Tr::tr("Comma-separated list of reviewers.\n"
            "\n"
            "Reviewers can be specified by nickname or email address. Spaces not allowed.\n"
            "\n"
            "Partial names can be used if they are unambiguous."));
    m_wipCheckBox->setTristate(true);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    using namespace Layouting;

    Grid {
        ::Git::Tr::tr("Push:"), workingDir.toUserOutput(), m_localBranchComboBox, br,
        ::Git::Tr::tr("To:"), m_remoteComboBox, m_targetBranchComboBox, br,
        ::Git::Tr::tr("Commits:"), br,
        Span(3, m_commitView), br,
        Span(3, m_infoLabel), br,
        Span(3, Form {
            ::Git::Tr::tr("&Topic:"), Row { m_topicLineEdit, m_draftCheckBox, m_wipCheckBox }, br,
            ::Git::Tr::tr("&Reviewers:"), m_reviewersLineEdit, br
        }), br,
        Span(3, m_buttonBox)
    }.attachTo(this);

    m_remoteComboBox->setRepository(workingDir);
    m_remoteComboBox->setParameters(parameters);
    m_remoteComboBox->setAllowDups(true);

    auto delegate = new PushItemDelegate(m_commitView);
    delegate->setParent(this);

    initRemoteBranches();

    if (m_remoteComboBox->isEmpty()) {
        m_initErrorMessage = Git::Tr::tr("Cannot find a Gerrit remote. Add one and try again.");
        return;
    }

    m_localBranchComboBox->init(workingDir);
    connect(m_localBranchComboBox, &QComboBox::currentIndexChanged,
            this, &GerritPushDialog::updateCommits);
    connect(m_targetBranchComboBox, &QComboBox::currentIndexChanged,
            this, &GerritPushDialog::setChangeRange);
    connect(m_targetBranchComboBox, &QComboBox::currentTextChanged,
            this, &GerritPushDialog::validate);

    updateCommits(m_localBranchComboBox->currentIndex());
    onRemoteChanged(true);

    QRegularExpressionValidator *noSpaceValidator = new QRegularExpressionValidator(QRegularExpression("^\\S+$"), this);
    m_reviewersLineEdit->setText(reviewerList);
    m_reviewersLineEdit->setValidator(noSpaceValidator);
    m_topicLineEdit->setValidator(noSpaceValidator);
    m_wipCheckBox->setCheckState(Qt::PartiallyChecked);

    connect(m_remoteComboBox, &GerritRemoteChooser::remoteChanged,
            this, [this] { onRemoteChanged(); });

    resize(740, 410);
}

QString GerritPushDialog::selectedCommit() const
{
    return m_commitView->commit();
}

QString GerritPushDialog::calculateChangeRange(const QString &branch)
{
    const QString remote = selectedRemoteName() + '/' + selectedRemoteBranchName();
    QString number;
    QString error;
    gitClient().synchronousRevListCmd(
                m_workingDir, { remote + ".." + branch, "--count" }, &number, &error);
    number.chop(1);
    return number;
}

void GerritPushDialog::setChangeRange()
{
    if (m_targetBranchComboBox->itemData(m_targetBranchComboBox->currentIndex()) == 1) {
        setRemoteBranches(true);
        return;
    }
    const QString remoteBranchName = selectedRemoteBranchName();
    if (remoteBranchName.isEmpty())
        return;
    const QString branch = m_localBranchComboBox->currentText();
    const QString range = calculateChangeRange(branch);
    if (range.isEmpty()) {
        m_infoLabel->hide();
        return;
    }
    m_infoLabel->show();
    const QString remote = selectedRemoteName() + '/' + remoteBranchName;
    QString labelText =
        Git::Tr::tr("Number of commits between %1 and %2: %3").arg(branch, remote, range);
    const int currentRange = range.toInt();
    QPalette palette = QApplication::palette();
    if (currentRange > ReasonableDistance) {
        const QColor errorColor = Utils::creatorTheme()->color(Utils::Theme::TextColorError);
        palette.setColor(QPalette::WindowText, errorColor);
        palette.setColor(QPalette::ButtonText, errorColor);
        labelText.append("\n" + Git::Tr::tr("Are you sure you selected the right target branch?"));
    }
    m_infoLabel->setPalette(palette);
    m_targetBranchComboBox->setPalette(palette);
    m_infoLabel->setText(labelText);
}

static bool versionSupportsWip(const QString &version)
{
    return QVersionNumber::fromString(version) >= QVersionNumber(2, 15);
}

void GerritPushDialog::onRemoteChanged(bool force)
{
    setRemoteBranches();
    const QString version = m_remoteComboBox->currentServer().version;
    const QString remote = m_remoteComboBox->currentRemoteName();

    m_commitView->setExcludedRemote(remote);
    const QString branch = m_localBranchComboBox->itemText(m_localBranchComboBox->currentIndex());
    m_hasLocalCommits = m_commitView->init(m_workingDir, branch, LogChangeWidget::Silent);
    validate();

    const bool supportsWip = versionSupportsWip(version);
    if (!force && supportsWip == m_currentSupportsWip)
        return;
    m_currentSupportsWip = supportsWip;
    m_wipCheckBox->setEnabled(supportsWip);
    if (supportsWip) {
        m_wipCheckBox->setToolTip(Git::Tr::tr("Checked - Mark change as WIP.\n"
                                  "Unchecked - Mark change as ready for review.\n"
                                  "Partially checked - Do not change current state."));
        m_draftCheckBox->setTristate(true);
        if (m_draftCheckBox->checkState() != Qt::Checked)
            m_draftCheckBox->setCheckState(Qt::PartiallyChecked);
        m_draftCheckBox->setToolTip(Git::Tr::tr("Checked - Mark change as private.\n"
                                    "Unchecked - Remove mark.\n"
                                    "Partially checked - Do not change current state."));
    } else {
        m_wipCheckBox->setToolTip(Git::Tr::tr("Supported on Gerrit 2.15 and later."));
        m_draftCheckBox->setTristate(false);
        if (m_draftCheckBox->checkState() != Qt::Checked)
            m_draftCheckBox->setCheckState(Qt::Unchecked);
        m_draftCheckBox->setToolTip(Git::Tr::tr("Checked - The change is a draft.\n"
                                    "Unchecked - The change is not a draft."));
    }
}

QString GerritPushDialog::initErrorMessage() const
{
    return m_initErrorMessage;
}

QString GerritPushDialog::pushTarget() const
{
    QStringList options;
    QString target = selectedCommit();
    if (target.isEmpty())
        target = "HEAD";
    target += ":refs/";
    if (versionSupportsWip(m_remoteComboBox->currentServer().version)) {
        target += "for";
        const Qt::CheckState draftState = m_draftCheckBox->checkState();
        const Qt::CheckState wipState = m_wipCheckBox->checkState();
        if (draftState == Qt::Checked)
            options << "private";
        else if (draftState == Qt::Unchecked)
            options << "remove-private";

        if (wipState == Qt::Checked)
            options << "wip";
        else if (wipState == Qt::Unchecked)
            options << "ready";
    } else {
        target += QLatin1String(m_draftCheckBox->isChecked() ? "drafts" : "for");
    }
    target += '/' + selectedRemoteBranchName();
    const QString topic = selectedTopic();
    if (!topic.isEmpty())
        target += '/' + topic;

    const QStringList reviewersInput = reviewers().split(',', Qt::SkipEmptyParts);
    for (const QString &reviewer : reviewersInput)
        options << "r=" + reviewer;

    if (!options.isEmpty())
        target += '%' + options.join(',');
    return target;
}

void GerritPushDialog::storeTopic()
{
    const QString branch = m_localBranchComboBox->currentText();
    gitClient().setConfigValue(
                m_workingDir, QString("branch.%1.topic").arg(branch), selectedTopic());
}

void GerritPushDialog::setRemoteBranches(bool includeOld)
{
    {
        QSignalBlocker blocker(m_targetBranchComboBox);
        m_targetBranchComboBox->clear();

        const QString remoteName = selectedRemoteName();
        if (!m_remoteBranches.contains(remoteName)) {
            const QStringList remoteBranches =
                    gitClient().synchronousRepositoryBranches(remoteName, m_workingDir);
            for (const QString &branch : remoteBranches)
                m_remoteBranches.insertMulti(remoteName, {branch, {}});
            if (remoteBranches.isEmpty()) {
                m_targetBranchComboBox->setEditable(true);
                m_targetBranchComboBox->setToolTip(
                    Git::Tr::tr("No remote branches found. This is probably the initial commit."));
                if (QLineEdit *lineEdit = m_targetBranchComboBox->lineEdit())
                    lineEdit->setPlaceholderText(Git::Tr::tr("Branch name"));
            }
        }

        int i = 0;
        bool excluded = false;
        const QList<BranchDate> remoteBranches = m_remoteBranches.values(remoteName);
        for (const BranchDate &bd : remoteBranches) {
            const bool isSuggested = bd.first == m_suggestedRemoteBranch;
            if (includeOld || isSuggested || !bd.second.isValid()
                    || bd.second.daysTo(QDate::currentDate()) <= Git::Constants::OBSOLETE_COMMIT_AGE_IN_DAYS) {
                m_targetBranchComboBox->addItem(bd.first);
                if (isSuggested)
                    m_targetBranchComboBox->setCurrentIndex(i);
                ++i;
            } else {
                excluded = true;
            }
        }
        if (excluded)
            m_targetBranchComboBox->addItem(Git::Tr::tr("... Include older branches ..."), 1);
        setChangeRange();
    }
    validate();
}

void GerritPushDialog::updateCommits(int index)
{
    const QString branch = m_localBranchComboBox->itemText(index);
    m_hasLocalCommits = m_commitView->init(m_workingDir, branch, LogChangeWidget::Silent);
    const QString topic = gitClient().readConfigValue(
                m_workingDir, QString("branch.%1.topic").arg(branch));
    if (!topic.isEmpty())
        m_topicLineEdit->setText(topic);

    const QString remoteBranch = determineRemoteBranch(branch);
    if (!remoteBranch.isEmpty()) {
        const int slash = remoteBranch.indexOf('/');

        m_suggestedRemoteBranch = remoteBranch.mid(slash + 1);
        const QString remote = remoteBranch.left(slash);

        if (!m_remoteComboBox->setCurrentRemote(remote))
            onRemoteChanged();
    }
    validate();
}

void GerritPushDialog::validate()
{
    const bool valid = m_hasLocalCommits && !selectedRemoteBranchName().isEmpty();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

QString GerritPushDialog::selectedRemoteName() const
{
    return m_remoteComboBox->currentRemoteName();
}

QString GerritPushDialog::selectedRemoteBranchName() const
{
    return m_targetBranchComboBox->currentText();
}

QString GerritPushDialog::selectedTopic() const
{
    return m_topicLineEdit->text().trimmed();
}

QString GerritPushDialog::reviewers() const
{
    return m_reviewersLineEdit->text();
}

} // namespace Internal
} // namespace Gerrit
