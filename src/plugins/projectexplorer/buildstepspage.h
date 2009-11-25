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

#ifndef BUILDSTEPSPAGE_H
#define BUILDSTEPSPAGE_H

#include "buildstep.h"
#include <utils/detailswidget.h>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
class QHBoxLayout;
class QPushButton;
class QAbstractButton;
class QToolButton;
class QLabel;
class QVBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Project;

namespace Internal {

namespace Ui {
    class BuildStepsPage;
}

struct BuildStepsWidgetStruct
{
    BuildStepConfigWidget *widget;
    Utils::DetailsWidget *detailsWidget;
    QToolButton *upButton;
    QToolButton *downButton;
    QHBoxLayout *hbox;
};

class BuildStepsPage : public BuildConfigWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(BuildStepsPage)
public:
    explicit BuildStepsPage(Project *project, bool clean = false);
    virtual ~BuildStepsPage();

    QString displayName() const;
    void init(BuildConfiguration *bc);

private slots:
    void updateAddBuildStepMenu();
    void addBuildStep();
    void updateRemoveBuildStepMenu();
    void removeBuildStep();
    void upBuildStep();
    void downBuildStep();
    void updateSummary();

private:
    void stepMoveUp(int pos);
    void updateBuildStepButtonsState();
    void addBuildStepWidget(int pos, BuildStep *step);

    BuildConfiguration *m_configuration;
    QHash<QAction *, QPair<QString, ProjectExplorer::IBuildStepFactory *> > m_addBuildStepHash;
    bool m_clean;

    QList<QHBoxLayout *> m_titleLayouts;
    QList<BuildStepsWidgetStruct> m_buildSteps;

    QVBoxLayout *m_vbox;

    QLabel *m_noStepsLabel;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
};

} // Internal
} // ProjectExplorer

#endif // BUILDSTEPSPAGE_H
