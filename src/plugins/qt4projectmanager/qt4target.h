/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QT4TARGET_H
#define QT4TARGET_H

#include "qt4targetsetupwidget.h"

#include <qtsupport/baseqtversion.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>

namespace Utils {
class DetailsWidget;
class PathChooser;
}

QT_BEGIN_NAMESPACE
class QCheckBox;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Node;
}

namespace Qt4ProjectManager {
class Qt4Project;
class Qt4BaseTargetFactory;
class Qt4ProFileNode;
class Qt4BuildConfiguration;

class QT4PROJECTMANAGER_EXPORT Qt4BaseTarget : public ProjectExplorer::Target
{
    Q_OBJECT
public:
    Qt4BaseTarget(Qt4Project *parent, const QString &id);
    virtual ~Qt4BaseTarget();

    ProjectExplorer::BuildConfigWidget *createConfigWidget();

    Qt4BuildConfiguration *activeQt4BuildConfiguration() const;
    Qt4ProjectManager::Qt4Project *qt4Project() const;

    // This is the same for almost all Qt4Targets
    // so for now offer a convience function
    Qt4BuildConfiguration *addQt4BuildConfiguration(QString defaultDisplayName,
                                                    QString displayName,
                                                    QtSupport::BaseQtVersion *qtversion,
                                                    QtSupport::BaseQtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                    QString additionalArguments,
                                                    QString directory,
                                                    bool importing);

    virtual void createApplicationProFiles() = 0;

    virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Node *n) = 0;

    QList<ProjectExplorer::ToolChain *> possibleToolChains(ProjectExplorer::BuildConfiguration *bc) const;
    ProjectExplorer::ToolChain *preferredToolChain(ProjectExplorer::BuildConfiguration *) const;

    virtual Utils::FileName mkspec(const Qt4BuildConfiguration *bc) const;

signals:
    void buildDirectoryInitialized();
    /// emitted if the build configuration changed in a way that
    /// should trigger a reevaluation of all .pro files
    void proFileEvaluateNeeded(Qt4ProjectManager::Qt4BaseTarget *);

protected:
    void removeUnconfiguredCustomExectutableRunConfigurations();

private slots:
    void onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void onProFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *bc);
    void emitProFileEvaluateNeeded();
};

class Qt4DefaultTargetSetupWidget : public Qt4TargetSetupWidget
{
    Q_OBJECT
public:
    enum ShadowBuildOption { DISABLE, ENABLE, USER };
    Qt4DefaultTargetSetupWidget(Qt4BaseTargetFactory *factory,
                                const QString &id,
                                const QString &proFilePath,
                                const QList<BuildConfigurationInfo> &info,
                                const QtSupport::QtVersionNumber &minimumQtVersion,
                                const QtSupport::QtVersionNumber &maximumQtVersion,
                                bool importEnabled,
                                const QList<BuildConfigurationInfo> &importInfos,
                                ShadowBuildOption shadowBuild);
    ~Qt4DefaultTargetSetupWidget();
    bool isTargetSelected() const;
    void setTargetSelected(bool b);
    QList<BuildConfigurationInfo> usedImportInfos();

    QList<BuildConfigurationInfo> buildConfigurationInfos() const;
    void setProFilePath(const QString &proFilePath);

    void setBuildConfiguraionComboBoxVisible(bool b);

    enum BuildConfigurationTemplate { PERQT = 0,
                                      ONEQT = 1,
                                      MANUALLY = 2,
                                      NONE = 3 };
    BuildConfigurationTemplate buildConfigurationTemplate() const;
    void setBuildConfigurationTemplate(BuildConfigurationTemplate value);

    void storeSettings() const;
private slots:
    void addImportClicked();
    void checkBoxToggled(bool b);
    void importCheckBoxToggled(bool b);
    void pathChanged();
    void shadowBuildingToggled();
    void buildConfigurationComboBoxChanged();
    void qtVersionChanged();
    void targetCheckBoxToggled(bool b);

private:
    void updateWidgetVisibility();
    void setBuildConfigurationInfos(const QList<BuildConfigurationInfo> &list, bool resetEnabled = true);
    bool reportIssues(int index);
    QPair<ProjectExplorer::Task::TaskType, QString> findIssues(const BuildConfigurationInfo &info);
    void createImportWidget(const BuildConfigurationInfo &info, int pos);

    QString m_id;
    Qt4BaseTargetFactory *m_factory;
    QString m_proFilePath;
    QtSupport::QtVersionNumber m_minimumQtVersion;
    QtSupport::QtVersionNumber m_maximumQtVersion;
    Utils::DetailsWidget *m_detailsWidget;
    QGridLayout *m_importLayout;
    QGridLayout *m_newBuildsLayout;
    QCheckBox *m_shadowBuildEnabled;
    QLabel *m_buildConfigurationLabel;
    QComboBox *m_buildConfigurationComboBox;
    QLabel *m_versionLabel;
    QComboBox *m_versionComboBox;

    // import line widgets
    QHBoxLayout *m_importLineLayout;
    QLabel *m_importLineLabel;
    Utils::PathChooser *m_importLinePath;
    QPushButton *m_importLineButton;

    void setupWidgets();
    void clearWidgets();
    void setupImportWidgets();
    QString displayNameFrom(const BuildConfigurationInfo &info);
    QList<QCheckBox *> m_checkboxes;
    QList<Utils::PathChooser *> m_pathChoosers;
    QList<BuildConfigurationInfo> m_infos;
    QList<bool> m_enabled;
    QList<QCheckBox *> m_importCheckBoxes;
    QList<BuildConfigurationInfo> m_importInfos;
    QList<bool> m_importEnabled;
    QList<QLabel *> m_reportIssuesLabels;
    QList<bool> m_issues;
    bool m_directoriesEnabled;
    bool m_hasInSourceBuild;
    bool m_ignoreChange;
    bool m_showImport;
    bool m_buildConfigurationTemplateUnchanged;
    bool m_shadowBuildCheckBoxVisible;
    int m_selected; // Number of selected buildconfiguartions
    int m_qtVersionId; // version id for "One Qt" entry
};

} // namespace Qt4ProjectManager

#endif // QT4TARGET_H
