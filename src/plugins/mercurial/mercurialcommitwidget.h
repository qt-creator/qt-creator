#ifndef MERCURIALCOMMITWIDGET_H
#define MERCURIALCOMMITWIDGET_H

#include "ui_mercurialcommitpanel.h"

#include <utils/submiteditorwidget.h>

namespace Mercurial {
namespace Internal {

/*submit editor widget based on git SubmitEditor
  Some extra fields have been added to the standard SubmitEditorWidget,
  to help to conform to the commit style that is used by both git and Mercurial*/

class MercurialCommitWidget : public Core::Utils::SubmitEditorWidget
{

public:
    explicit MercurialCommitWidget(QWidget *parent = 0);

    void setFields(const QString &repositoryRoot, const QString &branch,
                   const QString &userName, const QString &email);

    QString committer();
    QString repoRoot();

private:
    QWidget *mercurialCommitPanel;
    Ui::MercurialCommitPanel mercurialCommitPanelUi;
};

} // namespace Internal
} // namespace Mercurial

#endif // MERCURIALCOMMITWIDGET_H
