/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BUILDSETTINGSPROPERTIESPAGE_H
#define BUILDSETTINGSPROPERTIESPAGE_H

#include "iprojectproperties.h"

#include <QtGui/QWidget>
#include <QtGui/QIcon>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QMenu;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {

class BuildConfiguration;
class BuildConfigWidget;
class IBuildStepFactory;

namespace Internal {

const char * const BUILDSETTINGS_PANEL_ID("ProjectExplorer.BuildSettingsPanel");

class BuildSettingsPanelFactory : public ITargetPanelFactory
{
public:
    QString id() const;
    QString displayName() const;

    bool supports(Target *target);
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
    void addSubWidget(BuildConfigWidget *widget);
    QList<BuildConfigWidget *> subWidgets() const;

    void setupUi();

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
    QString uniqueName(const QString & name);

    Target *m_target;
    BuildConfiguration *m_buildConfiguration;

    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QPushButton *m_renameButton;
    QPushButton *m_makeActiveButton;
    QComboBox *m_buildConfigurationComboBox;
    QMenu *m_addButtonMenu;

    QList<BuildConfigWidget *> m_subWidgets;
    QList<QLabel *> m_labels;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // BUILDSETTINGSPROPERTIESPAGE_H
