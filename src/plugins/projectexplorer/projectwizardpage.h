/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PROJECTWIZARDPAGE_H
#define PROJECTWIZARDPAGE_H

#include <QtGui/QWizardPage>

namespace ProjectExplorer {
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

protected:
    virtual void changeEvent(QEvent *e);

private slots:
    void slotProjectChanged(int);

private:
    inline void setProjectToolTip(const QString &);

    Ui::WizardPage *m_ui;
    QStringList m_projectToolTips;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWIZARDPAGE_H
