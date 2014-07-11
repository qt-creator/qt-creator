/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef IPROJECTPROPERTIES_H
#define IPROJECTPROPERTIES_H

#include "projectexplorer_export.h"

#include <QObject>
#include <QIcon>
#include <QWidget>

namespace ProjectExplorer {
class Project;
class Target;

namespace Constants {
    const int PANEL_LEFT_MARGIN = 70;
}

class PROJECTEXPLORER_EXPORT PropertiesPanel
{
    Q_DISABLE_COPY(PropertiesPanel)

public:
    PropertiesPanel() {}
    ~PropertiesPanel() { delete m_widget; }

    QString displayName() const { return m_displayName; }
    QIcon icon() const { return m_icon; }
    QWidget *widget() const { return m_widget; }

    void setDisplayName(const QString &name) { m_displayName = name; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setWidget(QWidget *widget) { m_widget = widget; }

private:
    QString m_displayName;
    QWidget *m_widget;
    QIcon m_icon;
};

class PROJECTEXPLORER_EXPORT IProjectPanelFactory : public QObject
{
    Q_OBJECT
public:
    IProjectPanelFactory();
    // simple properties
    QString displayName() const;
    void setDisplayName(const QString &name);
    int priority() const;
    void setPriority(int priority);

    // helper to sort by priority
    static bool prioritySort(IProjectPanelFactory *a, IProjectPanelFactory *b);

    // interface for users of IProjectPanelFactory
    bool supports(Project *project);
    ProjectExplorer::PropertiesPanel *createPanel(ProjectExplorer::Project *project);

    // interface for "implementations" of IProjectPanelFactory
    // by default all projects are supported, only set a custom supports function
    // if you need something different
    void setSupportsFunction(std::function<bool (Project *)> function);

    // the simpleCreatePanelFunction creates new instance of T
    // wraps that into a PropertiesPanel
    // sets the passed in icon on it
    // and uses displayName() for the displayname
    // Note: call setDisplayName before calling this
    template<typename T>
    void setSimpleCreatePanelFunction(const QIcon &icon)
    {
        m_createPanelFunction = [icon, this](Project *project) -> PropertiesPanel * {
            PropertiesPanel *panel = new PropertiesPanel;
            panel->setDisplayName(this->displayName());
            panel->setWidget(new T(project)),
            panel->setIcon(icon);
            return panel;
        };
    }

    static bool supportsAllProjects(Project *);

private:
    int m_priority;
    QString m_displayName;
    std::function<bool (Project *)> m_supportsFunction;
    std::function<ProjectExplorer::PropertiesPanel *(Project *)> m_createPanelFunction;
};

class PROJECTEXPLORER_EXPORT ITargetPanelFactory : public QObject
{
    Q_OBJECT
public:
    virtual bool supports(Target *target) = 0;
    virtual PropertiesPanel *createPanel(Target *target) = 0;

    virtual QString id() const = 0;
};

} // namespace ProjectExplorer

#endif // IPROJECTPROPERTIES_H
