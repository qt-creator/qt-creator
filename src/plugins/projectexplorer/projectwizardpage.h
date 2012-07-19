/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef PROJECTWIZARDPAGE_H
#define PROJECTWIZARDPAGE_H

#include <QWizardPage>

namespace ProjectExplorer {
namespace Internal {

namespace Ui {
class WizardPage;
}

// Documentation inside.
class ProjectWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ProjectWizardPage(QWidget *parent = 0);
    virtual ~ProjectWizardPage();

    void setProjects(const QStringList &);
    void setProjectToolTips(const QStringList &);

    int currentProjectIndex() const;
    void setCurrentProjectIndex(int);

    void setNoneLabel(const QString &label);
    void setAdditionalInfo(const QString &text);

    void setVersionControls(const QStringList &);

    int versionControlIndex() const;
    void setVersionControlIndex(int);

    // Returns the common path
    void setFilesDisplay(const QString &commonPath, const QStringList &files);

    void setAddingSubProject(bool addingSubProject);

    void setProjectComoBoxVisible(bool visible);

protected:
    virtual void changeEvent(QEvent *e);

private slots:
    void slotProjectChanged(int);
    void slotManageVcs();

private:
    inline void setProjectToolTip(const QString &);

    Ui::WizardPage *m_ui;
    QStringList m_projectToolTips;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWIZARDPAGE_H
