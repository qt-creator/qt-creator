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
#include "propertiespanel.h"
#include "panelswidget.h"

#include <QObject>
#include <QIcon>
#include <QWidget>

namespace ProjectExplorer {
class Project;
class Target;

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
    QWidget *createWidget(ProjectExplorer::Project *project);

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
    void setSimpleCreateWidgetFunction(const QIcon &icon)
    {
        m_createWidgetFunction = [icon, this](Project *project) -> QWidget * {
            PropertiesPanel *panel = new PropertiesPanel;
            panel->setDisplayName(this->displayName());
            panel->setWidget(new T(project)),
            panel->setIcon(icon);
            PanelsWidget *panelsWidget = new PanelsWidget();
            panelsWidget->addPropertiesPanel(panel);
            return panelsWidget;
        };
    }

    void setCreateWidgetFunction(std::function<QWidget *(Project *)> function)
    {
        m_createWidgetFunction = function;
    }

    static bool supportsAllProjects(Project *);

private:
    int m_priority;
    QString m_displayName;
    std::function<bool (Project *)> m_supportsFunction;
    std::function<QWidget *(Project *)> m_createWidgetFunction;
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
