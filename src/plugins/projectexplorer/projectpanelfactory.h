/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROJECTPANELFACTORY_H
#define PROJECTPANELFACTORY_H

#include "projectexplorer_export.h"
#include "propertiespanel.h"
#include "panelswidget.h"

#include <functional>

namespace ProjectExplorer {

class Project;
class ProjectExplorerPlugin;

class PROJECTEXPLORER_EXPORT ProjectPanelFactory
{
public:
    ProjectPanelFactory();

    // simple properties
    QString displayName() const;
    void setDisplayName(const QString &name);
    int priority() const;
    void setPriority(int priority);

    // interface for users of ProjectPanelFactory
    bool supports(Project *project);
    QWidget *createWidget(Project *project);

    // interface for "implementations" of ProjectPanelFactory
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
            QWidget *widget = new T(project);
            panel->setWidget(widget);
            panel->setIcon(icon);
            PanelsWidget *panelsWidget = new PanelsWidget();
            panelsWidget->addPropertiesPanel(panel);
            panelsWidget->setFocusProxy(widget);
            return panelsWidget;
        };
    }

    void setCreateWidgetFunction(std::function<QWidget *(Project *)> function)
    {
        m_createWidgetFunction = function;
    }

    // This takes ownership.
    static void registerFactory(ProjectPanelFactory *factory);

    static QList<ProjectPanelFactory *> factories();

private:
    friend class ProjectExplorerPlugin;
    static void destroyFactories();

    int m_priority;
    QString m_displayName;
    std::function<bool (Project *)> m_supportsFunction;
    std::function<QWidget *(Project *)> m_createWidgetFunction;
};

} // namespace ProjectExplorer

#endif // PROJECTPANELFACTORY_H
