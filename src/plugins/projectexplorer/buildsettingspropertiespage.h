/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

const char * const BUILDSETTINGS_PANEL_ID("ProjectExplorer.BuildSettingsPanel");

class BuildSettingsPanelFactory : public IPanelFactory
{
public:
    QString id() const;
    QString displayName() const;
    bool supports(Project *project);
    bool supports(Target *target);
    IPropertiesPanel *createPanel(Project *project);
    IPropertiesPanel *createPanel(Target *target);
};

class BuildSettingsWidget;

class BuildSettingsPanel : public IPropertiesPanel
{
public:
    BuildSettingsPanel(Target *target);
    ~BuildSettingsPanel();
    QString displayName() const;
    QWidget *widget() const;
    QIcon icon() const;
    PanelFlags flags() const { return IPropertiesPanel::NoLeftMargin; }

private:
    BuildSettingsWidget *m_widget;
    const QIcon m_icon;
};

class BuildConfigurationsWidget;

class BuildSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    BuildSettingsWidget(Target *target);
    ~BuildSettingsWidget();

    void clear();
    void addSubWidget(const QString &name, QWidget *widget);
    QList<QWidget *> subWidgets() const;

    void setupUi();

private slots:
    void updateBuildSettings();
    void currentIndexChanged(int index);
    void currentBuildConfigurationChanged();

    void createConfiguration();
    void cloneConfiguration();
    void deleteConfiguration();
    void makeActive();
    void updateAddButtonMenu();

    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void buildConfigurationDisplayNameChanged();
    void updateActiveConfiguration();

private:
    void cloneConfiguration(BuildConfiguration *toClone);
    void deleteConfiguration(BuildConfiguration *toDelete);
    QString buildConfigurationItemName(const BuildConfiguration *bc) const;
    BuildConfiguration *currentBuildConfiguration() const;

    Target *m_target;
    BuildConfiguration *m_buildConfiguration;

    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_makeActiveButton;
    QComboBox *m_buildConfigurationComboBox;
    QMenu *m_addButtonMenu;

    QList<QWidget *> m_subWidgets;
    QList<QLabel *> m_labels;

    int m_leftMargin;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // BUILDSETTINGSPROPERTIESPAGE_H
