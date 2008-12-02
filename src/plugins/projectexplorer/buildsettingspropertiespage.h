/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#ifndef BUILDSETTINGSPROPERTIESPAGE_H
#define BUILDSETTINGSPROPERTIESPAGE_H

#include "iprojectproperties.h"
#include "ui_buildsettingspropertiespage.h"

namespace ProjectExplorer {

class IBuildStepFactory;

namespace Internal {

class BuildSettingsPanelFactory : public IPanelFactory
{
public:
    bool supports(Project *project);
    PropertiesPanel *createPanel(Project *project);
};

class BuildSettingsWidget;

class BuildSettingsPanel : public PropertiesPanel
{
    Q_OBJECT
public:
    BuildSettingsPanel(Project *project);
    ~BuildSettingsPanel();
    QString name() const;
    QWidget *widget();

private:
    BuildSettingsWidget *m_widget;
};

class BuildConfigurationsWidget;

class BuildSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    BuildSettingsWidget(Project *project);
    ~BuildSettingsWidget();

private slots:
    void buildConfigurationDisplayNameChanged(const QString &buildConfiguration);
    void updateBuildSettings();
    void updateSettingsWidget(QTreeWidgetItem *newItem, QTreeWidgetItem *oldItem);
    void showContextMenu(const QPoint & pos);

    void setActiveConfiguration();
    void createConfiguration();
    void cloneConfiguration();
    void deleteConfiguration();

    void itemChanged(QTreeWidgetItem *item);

private:
    void setActiveConfiguration(const QString &configuration);
    void cloneConfiguration(const QString &toClone);
    void deleteConfiguration(const QString &toDelete);

    Ui::BuildSettingsPropertiesPage m_ui;
    Project *m_project;
    QHash<QTreeWidgetItem*, QWidget*> m_itemToWidget;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // BUILDSETTINGSPROPERTIESPAGE_H
