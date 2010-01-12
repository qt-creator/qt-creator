/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef BUILDSETTINGSPROPERTIESPAGE_H
#define BUILDSETTINGSPROPERTIESPAGE_H

#include "iprojectproperties.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QMenu;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {

class BuildConfiguration;
class IBuildStepFactory;

namespace Internal {

class BuildSettingsPanelFactory : public IPanelFactory
{
public:
    bool supports(Project *project);
    IPropertiesPanel *createPanel(Project *project);
};

class BuildSettingsWidget;

class BuildSettingsPanel : public IPropertiesPanel
{
public:
    BuildSettingsPanel(Project *project);
    ~BuildSettingsPanel();
    QString displayName() const;
    QWidget *widget() const;
    QIcon icon() const;

private:
    BuildSettingsWidget *m_widget;
    const QIcon m_icon;
};

class BuildConfigurationsWidget;

class BuildSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    BuildSettingsWidget(Project *project);
    ~BuildSettingsWidget();

    void clear();
    void addSubWidget(const QString &name, QWidget *widget);
    QList<QWidget *> subWidgets() const;

private slots:
    void updateBuildSettings();
    void currentIndexChanged(int index);
    void activeBuildConfigurationChanged();

    void createConfiguration();
    void cloneConfiguration();
    void deleteConfiguration();
    void updateAddButtonMenu();
    void checkMakeActiveLabel();
    void makeActive();

    void setupUi();

    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void buildConfigurationDisplayNameChanged();
private:
    void cloneConfiguration(BuildConfiguration *toClone);
    void deleteConfiguration(BuildConfiguration *toDelete);

    Project *m_project;
    BuildConfiguration *m_buildConfiguration;

    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QComboBox *m_buildConfigurationComboBox;
    QMenu *m_addButtonMenu;
    QLabel *m_makeActiveLabel;

    QList<QWidget *> m_subWidgets;
    QList<QLabel *> m_labels;

    int m_leftMargin;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // BUILDSETTINGSPROPERTIESPAGE_H
