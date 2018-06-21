/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "buildstep.h"
#include "namedwidget.h"
#include <utils/detailsbutton.h>

QT_BEGIN_NAMESPACE
class QPushButton;
class QToolButton;
class QLabel;
class QVBoxLayout;
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
    explicit ToolWidget(QWidget *parent = nullptr);

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

    bool m_buildStepEnabled = true;
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
    BuildStepListWidget(QWidget *parent = nullptr);
    virtual ~BuildStepListWidget();

    void init(BuildStepList *bsl);

private:
    void updateAddBuildStepMenu();
    void addBuildStep(int pos);
    void updateSummary();
    void updateAdditionalSummary();
    void updateEnabledState();
    void stepMoved(int from, int to);
    void removeBuildStep(int pos);

    void setupUi();
    void updateBuildStepButtonsState();
    void addBuildStepWidget(int pos, BuildStep *step);

    BuildStepList *m_buildStepList = nullptr;

    QList<Internal::BuildStepsWidgetData *> m_buildStepsData;

    QVBoxLayout *m_vbox = nullptr;

    QLabel *m_noStepsLabel = nullptr;
    QPushButton *m_addButton = nullptr;
};

namespace Ui { class BuildStepsPage; }

class BuildStepsPage : public NamedWidget
{
    Q_OBJECT

public:
    BuildStepsPage(BuildConfiguration *bc, Core::Id id);

private:
    Core::Id m_id;
    BuildStepListWidget *m_widget = nullptr;
};

} // Internal
} // ProjectExplorer
