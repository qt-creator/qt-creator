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

#include <QtCore/QHash>
#include <QtGui/QComboBox>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>

namespace ProjectExplorer {

class IBuildStepFactory;

namespace Internal {

class BuildSettingsSubWidgets : public QWidget
{
    Q_OBJECT
public:
    BuildSettingsSubWidgets(QWidget *parent);
    ~BuildSettingsSubWidgets();
    void clear();
    void addWidget(const QString &name, QWidget *widget);
    QList<QWidget *> widgets() const;
private:
    QList<QWidget *> m_widgets;
    QList<QLabel *> m_labels;
};

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
    void currentIndexChanged(int index);
    void activeBuildConfigurationChanged();

    void createConfiguration();
    void cloneConfiguration();
    void deleteConfiguration();

private:
    void cloneConfiguration(const QString &toClone);
    void deleteConfiguration(const QString &toDelete);
    void createAddButtonMenu();

    Project *m_project;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QComboBox *m_buildConfigurationComboBox;
    BuildSettingsSubWidgets *m_subWidgets;
    QString m_buildConfiguration;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // BUILDSETTINGSPROPERTIESPAGE_H
