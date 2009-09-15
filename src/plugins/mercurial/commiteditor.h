#ifndef COMMITEDITOR_H
#define COMMITEDITOR_H

#include <vcsbase/vcsbasesubmiteditor.h>

#include <QtCore/QFileInfo>

namespace VCSBase {
class SubmitFileModel;
}

namespace Mercurial {
namespace Internal {

class MercurialCommitWidget;

class CommitEditor : public VCSBase::VCSBaseSubmitEditor
{
    Q_OBJECT
public:
    explicit CommitEditor(const VCSBase::VCSBaseSubmitEditorParameters *parameters,
                          QWidget *parent);

    void setFields(const QFileInfo &repositoryRoot, const QString &branch,
                   const QString &userName, const QString &email,
                   const QList<QPair<QString, QString> > &repoStatus);

    QString committerInfo();
    QString repoRoot();


private:
    inline MercurialCommitWidget *commitWidget();
    VCSBase::SubmitFileModel *fileModel;

};

}
}
#endif // COMMITEDITOR_H
