/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef BUILDSETTINGSPROPERTIESPAGE_H
#define BUILDSETTINGSPROPERTIESPAGE_H

#include "iprojectproperties.h"

#include <QWidget>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QMenu;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {

class BuildConfiguration;
class IBuildStepFactory;
class NamedWidget;

namespace Internal {

const char BUILDSETTINGS_PANEL_ID[] = "ProjectExplorer.BuildSettingsPanel";

class BuildSettingsPanelFactory : public ITargetPanelFactory
{
public:
    QString id() const;
    QString displayName() const;
    int priority() const;

    bool supports(Target *target);
    PropertiesPanel *createPanel(Target *target);
};

class BuildConfigurationsWidget;

class BuildSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    BuildSettingsWidget(Target *target);
    ~BuildSettingsWidget();

    void clear();
    void addSubWidget(ProjectExplorer::NamedWidget *widget);
    QList<ProjectExplorer::NamedWidget *> subWidgets() const;

private slots:
    void updateBuildSettings();
    void currentIndexChanged(int index);

    void createConfiguration();
    void cloneConfiguration();
    void deleteConfiguration();
    void renameConfiguration();
    void updateAddButtonMenu();

    void updateActiveConfiguration();

private:
    void cloneConfiguration(BuildConfiguration *toClone);
    void deleteConfiguration(BuildConfiguration *toDelete);
    QString uniqueName(const QString &name);

    Target *m_target;
    BuildConfiguration *m_buildConfiguration;

    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_renameButton;
    QPushButton *m_makeActiveButton;
    QComboBox *m_buildConfigurationComboBox;
    QMenu *m_addButtonMenu;

    QList<NamedWidget *> m_subWidgets;
    QList<QLabel *> m_labels;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // BUILDSETTINGSPROPERTIESPAGE_H
