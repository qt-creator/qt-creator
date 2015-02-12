/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BUILDSTEPSPAGE_H
#define BUILDSTEPSPAGE_H

#include "buildstep.h"
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

namespace Utils { class DetailsWidget; }

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

namespace Ui { class BuildStepsPage; }

class BuildStepsPage : public NamedWidget
{
    Q_OBJECT

public:
    BuildStepsPage(BuildConfiguration *bc, Core::Id id);
    virtual ~BuildStepsPage();

private:
    Core::Id m_id;
    BuildStepListWidget *m_widget;
};

} // Internal
} // ProjectExplorer

#endif // BUILDSTEPSPAGE_H
