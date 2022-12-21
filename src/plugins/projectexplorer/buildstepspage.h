// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildstep.h"
#include "namedwidget.h"
#include <utils/detailsbutton.h>

QT_BEGIN_NAMESPACE
class QPushButton;
class QToolButton;
class QLabel;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace ProjectExplorer {
namespace Internal {

class ToolWidget : public Utils::FadingPanel
{
    Q_OBJECT
public:
    explicit ToolWidget(QWidget *parent = nullptr);

    void fadeTo(qreal value) override;
    void setOpacity(qreal value) override;

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
    qreal m_targetOpacity = .999;
};

class BuildStepsWidgetData
{
public:
    BuildStepsWidgetData(BuildStep *s);
    ~BuildStepsWidgetData();

    BuildStep *step;
    QWidget *widget;
    Utils::DetailsWidget *detailsWidget;
    ToolWidget *toolWidget;
};

class BuildStepListWidget : public NamedWidget
{
    Q_OBJECT

public:
    explicit BuildStepListWidget(BuildStepList *bsl);
    ~BuildStepListWidget() override;

private:
    void updateAddBuildStepMenu();
    void addBuildStep(int pos);
    void stepMoved(int from, int to);
    void removeBuildStep(int pos);

    void setupUi();
    void updateBuildStepButtonsState();

    BuildStepList *m_buildStepList = nullptr;

    QList<Internal::BuildStepsWidgetData *> m_buildStepsData;

    QVBoxLayout *m_vbox = nullptr;

    QLabel *m_noStepsLabel = nullptr;
    QPushButton *m_addButton = nullptr;
};

} // Internal
} // ProjectExplorer
