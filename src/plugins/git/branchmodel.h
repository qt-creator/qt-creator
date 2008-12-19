#ifndef BRANCHMODEL_H
#define BRANCHMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QList>

namespace Git {
    namespace Internal {

class GitClient;

/* A model to list git branches in a simple list of branch names. Local
 * branches will have a read-only checkbox indicating the current one. The
 * [delayed] tooltip describes the latest commit. */

class BranchModel : public QAbstractListModel {
public:
    enum Type { LocalBranches, RemoteBranches };

    explicit BranchModel(GitClient *client,
                         Type type = LocalBranches,
                         QObject *parent = 0);

    bool refresh(const QString &workingDirectory, QString *errorMessage);

    int currentBranch() const;
    QString branchName(int row) const;

    // QAbstractListModel
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

private:
    QString toolTip(const QString &sha) const;

    struct Branch {
        bool parse(const QString &line, bool *isCurrent);

        QString name;
        QString currentSHA;
        mutable QString toolTip;
    };
    typedef QList<Branch> BranchList;

    const Type m_type;
    const Qt::ItemFlags m_flags;
    GitClient *m_client;

    QString m_workingDirectory;
    BranchList m_branches;
    int m_currentBranch;
};

}
}

#endif // BRANCHMODEL_H
