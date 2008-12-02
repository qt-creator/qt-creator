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
