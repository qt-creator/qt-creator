/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef BUILDSTEPSPAGE_H
#define BUILDSTEPSPAGE_H

#include "buildstep.h"
#include "deployconfiguration.h"
#include "namedwidget.h"
#include <utils/detailsbutton.h>

QT_BEGIN_NAMESPACE
class QPushButton;
class QToolButton;
class QLabel;
class QVBoxLayout;
class QSignalMapper;
class QGraphicsOpacityEffect;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
}

namespace ProjectExplorer {

class Target;
class BuildConfiguration;

namespace Internal {

class ToolWidget : public Utils::FadingPanel
{
    Q_OBJECT
public:
    ToolWidget(QWidget *parent = 0);

    void fadeTo(qreal value);
    void setOpacity(qreal value);

    void setBuildStepEnabled(bool b);
    void setUpEnabled(bool b);
    void setDownEnabled(bool b);
    void setRemoveEnabled(bool b);
    void setUpVisible(bool b);
    void setDownVisible(bool b);
signals:
    void disabledClicked();
    void upClicked();
    void downClicked();
    void removeClicked();

private:
    QToolButton *m_disableButton;
    QToolButton *m_upButton;
    QToolButton *m_downButton;
    QToolButton *m_removeButton;

    bool m_buildStepEnabled;
    Utils::FadingWidget *m_firstWidget;
    Utils::FadingWidget *m_secondWidget;
    qreal m_targetOpacity;
};

class BuildStepsWidgetData
{
public:
    BuildStepsWidgetData(BuildStep *s);
    ~BuildStepsWidgetData();

    BuildStep *step;
    BuildStepConfigWidget *widget;
    Utils::DetailsWidget *detailsWidget;
    ToolWidget *toolWidget;
};

class BuildStepListWidget : public NamedWidget
{
    Q_OBJECT

public:
    BuildStepListWidget(QWidget *parent = 0);
    virtual ~BuildStepListWidget();

    void init(BuildStepList *bsl);

private slots:
    void updateAddBuildStepMenu();
    void triggerAddBuildStep();
    void addBuildStep(int pos);
    void updateSummary();
    void updateAdditionalSummary();
    void updateEnabledState();
    void triggerStepMoveUp(int pos);
    void stepMoved(int from, int to);
    void triggerStepMoveDown(int pos);
    void triggerRemoveBuildStep(int pos);
    void removeBuildStep(int pos);
    void triggerDisable(int pos);

private:
    void setupUi();
    void updateBuildStepButtonsState();
    void addBuildStepWidget(int pos, BuildStep *step);

    BuildStepList *m_buildStepList;
    QHash<QAction *, QPair<Core::Id, ProjectExplorer::IBuildStepFactory *> > m_addBuildStepHash;

    QList<Internal::BuildStepsWidgetData *> m_buildStepsData;

    QVBoxLayout *m_vbox;

    QLabel *m_noStepsLabel;
    QPushButton *m_addButton;

    QSignalMapper *m_disableMapper;
    QSignalMapper *m_upMapper;
    QSignalMapper *m_downMapper;
    QSignalMapper *m_removeMapper;

    int m_leftMargin;
};

namespace Ui {
    class BuildStepsPage;
}

class BuildStepsPage : public BuildConfigWidget
{
    Q_OBJECT

public:
    BuildStepsPage(Target *target, Core::Id id);
    virtual ~BuildStepsPage();

    QString displayName() const;
    void init(BuildConfiguration *bc);

private:
    Core::Id m_id;
    BuildStepListWidget *m_widget;
};

} // Internal
} // ProjectExplorer

#endif // BUILDSTEPSPAGE_H
