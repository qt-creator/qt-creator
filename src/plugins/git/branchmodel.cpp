#include "branchmodel.h"
#include "gitclient.h"

#include <QtCore/QDebug>

enum { debug = 0 };

namespace Git {
    namespace Internal {

// Parse a branch line: " *name sha description".  Return  true if it is
// the current one
bool BranchModel::Branch::parse(const QString &lineIn, bool *isCurrent)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << lineIn;

    *isCurrent = lineIn.startsWith(QLatin1String("* "));
    if (lineIn.size() < 3)
        return false;

    const QStringList tokens =lineIn.mid(2).split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (tokens.size() < 2)
        return false;
    name = tokens.at(0);
    currentSHA= tokens.at(1);
    toolTip.clear();
    return true;
}

static inline Qt::ItemFlags typeToModelFlags(BranchModel::Type t)
{
    Qt::ItemFlags rc = Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    if (t == BranchModel::LocalBranches)
        rc |= Qt::ItemIsUserCheckable;
    return rc;
}

// --- BranchModel
BranchModel::BranchModel(GitClient *client, Type type, QObject *parent) :
    QAbstractListModel(parent),
    m_type(type),
    m_flags(typeToModelFlags(type)),
    m_client(client),
    m_currentBranch(-1)
{
}

int BranchModel::currentBranch() const
{
    return m_currentBranch;
}

QString BranchModel::branchName(int row) const
{
    return m_branches.at(row).name;
}

int BranchModel::rowCount(const QModelIndex & /* parent */) const
{
    return m_branches.size();
}

QVariant BranchModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    switch (role) {
        case Qt::DisplayRole:
        return branchName(row);
        case Qt::ToolTipRole:
        if (m_branches.at(row).toolTip.isEmpty())
            m_branches.at(row).toolTip = toolTip(m_branches.at(row).currentSHA);
        return m_branches.at(row).toolTip;
        break;
        case Qt::CheckStateRole:
        if (m_type == RemoteBranches)
            return QVariant();
        return row == m_currentBranch ? Qt::Checked : Qt::Unchecked;
        default:
        break;
    }
    return QVariant();
}

Qt::ItemFlags BranchModel::flags(const QModelIndex & /*index */) const
{
    return m_flags;
}

bool BranchModel::refresh(const QString &workingDirectory, QString *errorMessage)
{
    // Run branch command with verbose.
    QStringList branchArgs(QLatin1String("-v"));
    QString output;
    if (m_type == RemoteBranches)
        branchArgs.push_back(QLatin1String("-r"));
    if (!m_client->synchronousBranchCmd(workingDirectory, branchArgs, &output, errorMessage))
        return false;
    if (debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << output;
    // Parse output
    m_workingDirectory = workingDirectory;
    m_branches.clear();
    m_currentBranch = -1;
    const QStringList branches = output.split(QLatin1Char('\n'));
    const int branchCount = branches.size();
    bool isCurrent;
    for (int b = 0; b < branchCount; b++) {
        Branch newBranch;
        if (newBranch.parse(branches.at(b), &isCurrent)) {
            m_branches.push_back(newBranch);
            if (isCurrent)
                m_currentBranch = b;
        }
    }
    reset();
    return true;
}

QString BranchModel::toolTip(const QString &sha) const
{
    // Show the sha description excluding diff as toolTip
    QString output;
    QString errorMessage;
    if (!m_client->synchronousShow(m_workingDirectory, sha, &output, &errorMessage))
        return errorMessage;
    // Remove 'diff' output
    const int diffPos = output.indexOf(QLatin1String("\ndiff --"));
    if (diffPos != -1)
        output.remove(diffPos, output.size() - diffPos);
    return output;
}

}
}

