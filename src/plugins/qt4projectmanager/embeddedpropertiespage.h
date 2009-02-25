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

#ifndef EMBEDDEDPROPERTIESPAGE_H
#define EMBEDDEDPROPERTIESPAGE_H

#include "ui_embeddedpropertiespage.h"

#include <projectexplorer/iprojectproperties.h>

#include <QtCore/QModelIndex>

namespace ProjectExplorer {
class Project;
}

namespace Qt4ProjectManager {

namespace Internal {

class EmbeddedPropertiesWidget;

class EmbeddedPropertiesPanelFactory : public ProjectExplorer::IPanelFactory
{
public:
    virtual bool supports(ProjectExplorer::Project *project);
    ProjectExplorer::PropertiesPanel *createPanel(ProjectExplorer::Project *project);
};

class EmbeddedPropertiesPanel : public ProjectExplorer::PropertiesPanel
{
    Q_OBJECT
public:
    EmbeddedPropertiesPanel(ProjectExplorer::Project *project);
    ~EmbeddedPropertiesPanel();

    QString name() const;
    QWidget *widget();

private:
    EmbeddedPropertiesWidget *m_widget;
};

class EmbeddedPropertiesWidget : public QWidget
{
    Q_OBJECT
public:
    EmbeddedPropertiesWidget(ProjectExplorer::Project *project);
    virtual ~EmbeddedPropertiesWidget();
 private:
    Ui_EmbeddedPropertiesPage m_ui;
    ProjectExplorer::Project *m_pro;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // EMBEDDEDPROPERTIESPAGE_H
