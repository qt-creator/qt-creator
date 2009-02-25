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

#ifndef BUILDSTEPSPAGE_H
#define BUILDSTEPSPAGE_H

#include "buildstep.h"

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Project;

namespace Internal {

namespace Ui {
    class BuildStepsPage;
}

class BuildStepsPage : public BuildStepConfigWidget {
    Q_OBJECT
    Q_DISABLE_COPY(BuildStepsPage)
public:
    explicit BuildStepsPage(Project *project);
    virtual ~BuildStepsPage();

    QString displayName() const;
    void init(const QString &buildConfiguration);

protected:
    virtual void changeEvent(QEvent *e);

private slots:
    void displayNameChanged(BuildStep *bs, const QString &displayName);
    void updateBuildStepWidget(QTreeWidgetItem *newItem, QTreeWidgetItem *oldItem);
    void updateAddBuildStepMenu();
    void addBuildStep();
    void removeBuildStep();
    void upBuildStep();
    void downBuildStep();

private:
    void buildStepMoveUp(int pos);
    void updateBuildStepButtonsState();

    Ui::BuildStepsPage *m_ui;
    Project *m_pro;
    QString m_configuration;
    QHash<QAction *, QPair<QString, ProjectExplorer::IBuildStepFactory *> > m_addBuildStepHash;
};

} // Internal
} // ProjectExplorer

#endif // BUILDSTEPSPAGE_H
