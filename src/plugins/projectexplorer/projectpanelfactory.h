/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "projectexplorer_export.h"

#include "panelswidget.h"
#include "projectwindow.h"

#include <utils/treemodel.h>

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

    using WidgetCreator = std::function<QWidget *(Project *)>;

    // interface for "implementations" of ProjectPanelFactory
    // by default all projects are supported, only set a custom supports function
    // if you need something different
    using SupportsFunction = std::function<bool (Project *)>;
    void setSupportsFunction(std::function<bool (Project *)> function);

    // This takes ownership.
    static void registerFactory(ProjectPanelFactory *factory);

    static QList<ProjectPanelFactory *> factories();

    Utils::TreeItem *createPanelItem(Project *project);

    QString icon() const;
    void setIcon(const QString &icon);

    void setCreateWidgetFunction(const WidgetCreator &createWidgetFunction);
    QWidget *createWidget(Project *project) const;

private:
    friend class ProjectExplorerPlugin;
    static void destroyFactories();

    int m_priority = 0;
    QString m_displayName;
    SupportsFunction m_supportsFunction;
    WidgetCreator m_widgetCreator;
    QString m_icon;
};

} // namespace ProjectExplorer
