/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

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
