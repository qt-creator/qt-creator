// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "projectsettingswidget.h"

#include <utils/id.h>
#include <utils/treemodel.h>

#include <functional>

namespace ProjectExplorer {

class Project;

class PROJECTEXPLORER_EXPORT ProjectPanelFactory
{
public:
    ProjectPanelFactory();

    Utils::Id id() const;
    void setId(Utils::Id id);

    // simple properties
    QString displayName() const;
    void setDisplayName(const QString &name);
    int priority() const;
    void setPriority(int priority);

    // interface for users of ProjectPanelFactory
    bool supports(Project *project);
    // Widgets created by this function should use setWindowTitle() to specify
    // their tab title.
    QWidget *createWidget(Project *project) const;

    // interface for "implementations" of ProjectPanelFactory
    // by default all projects are supported, only set a custom supports function
    // if you need something different
    using SupportsFunction = std::function<bool (Project *)>;
    void setSupportsFunction(std::function<bool (Project *)> function);

    static QList<ProjectPanelFactory *> factories();

    Utils::TreeItem *createPanelItem(Project *project);

    using WidgetCreator = std::function<QWidget *(Project *)>;
    void setCreateWidgetFunction(const WidgetCreator &createWidgetFunction);

private:
    Utils::Id m_id;
    int m_priority = 0;
    QString m_displayName;
    SupportsFunction m_supportsFunction;
    WidgetCreator m_widgetCreator;
};

} // namespace ProjectExplorer
