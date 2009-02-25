/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROJECTWIZARDPAGE_H
#define PROJECTWIZARDPAGE_H

#include <QtGui/QWizardPage>

namespace Core {
    class IVersionControl;
    class SCCManager;
    class FileManager;
}

namespace ProjectExplorer {

class ProjectNode;

namespace Internal {

namespace Ui {
class WizardPage;
}

class ProjectWizardPage : public QWizardPage {
    Q_OBJECT
    Q_DISABLE_COPY(ProjectWizardPage)
public:
    explicit ProjectWizardPage(QWidget *parent = 0);
    virtual ~ProjectWizardPage();

    void setProjects(const QStringList &);
    void setCurrentProjectIndex(int);
    int currentProjectIndex() const;

    void setAddToProjectEnabled(bool b);
    bool addToProject() const;

    bool addToVersionControl() const;
    void setAddToVersionControlEnabled(bool b);

    void setVCSDisplay(const QString &vcsName);
    void setFilesDisplay(const QStringList &files);

protected:
    virtual void changeEvent(QEvent *e);

private:
    Ui::WizardPage *m_ui;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWIZARDPAGE_H
