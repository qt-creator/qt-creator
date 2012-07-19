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

#ifndef UNCONFIGUREDPROJECTPANEL_H
#define UNCONFIGUREDPROJECTPANEL_H

#include <projectexplorer/iprojectproperties.h>

#include <QString>

QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace ProjectExplorer { class Profile; }

namespace Qt4ProjectManager {
class TargetSetupPage;
class Qt4Project;

namespace Internal {

class UnconfiguredProjectPanel : public ProjectExplorer::IProjectPanelFactory
{
    Q_OBJECT
public:
    UnconfiguredProjectPanel();
    virtual QString id() const;
    virtual QString displayName() const;
    int priority() const;
    virtual bool supports(ProjectExplorer::Project *project);
    virtual ProjectExplorer::PropertiesPanel *createPanel(ProjectExplorer::Project *project);
};

class TargetSetupPageWrapper : public QWidget
{
    Q_OBJECT
public:
    TargetSetupPageWrapper(ProjectExplorer::Project *project);
protected:
    void keyReleaseEvent(QKeyEvent *event);
    void keyPressEvent(QKeyEvent *event);
private slots:
    void done();
    void noteTextLinkActivated();
    void profileUpdated(ProjectExplorer::Profile *profile);
    void updateNoteText();
    void completeChanged();

private:
    Qt4Project *m_project;
    TargetSetupPage *m_targetSetupPage;
    QPushButton *m_configureButton;
};

}
}

#endif // UNCONFIGUREDPROJECTPANEL_H
