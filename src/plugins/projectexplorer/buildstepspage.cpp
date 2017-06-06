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

#include "buildstepspage.h"

#include "buildconfiguration.h"
#include "buildsteplist.h"
#include "projectexplorerconstants.h"
#include "projectexplorericons.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreicons.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>
#include <utils/theme/theme.h>

#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace Utils;

ToolWidget::ToolWidget(QWidget *parent) : FadingPanel(parent),
    m_targetOpacity(1.0f)
{
    auto layout = new QHBoxLayout;
    layout->setMargin(4);
    layout->setSpacing(4);
    setLayout(layout);
    m_firstWidget = new FadingWidget(this);
    m_firstWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto hbox = new QHBoxLayout();
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);
    m_firstWidget->setLayout(hbox);
    QSize buttonSize(20, HostOsInfo::isMacHost() ? 20 : 26);

    m_disableButton = new QToolButton(m_firstWidget);
    m_disableButton->setAutoRaise(true);
    m_disableButton->setToolTip(BuildStepListWidget::tr("Disable"));
    m_disableButton->setFixedSize(buttonSize);
    m_disableButton->setIcon(Icons::BUILDSTEP_DISABLE.icon());
    m_disableButton->setCheckable(true);
    hbox->addWidget(m_disableButton);
    layout->addWidget(m_firstWidget);

    m_secondWidget = new FadingWidget(this);
    m_secondWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(4);
    m_secondWidget->setLayout(hbox);

    m_upButton = new QToolButton(m_secondWidget);
    m_upButton->setAutoRaise(true);
    m_upButton->setToolTip(BuildStepListWidget::tr("Move Up"));
    m_upButton->setFixedSize(buttonSize);
    m_upButton->setIcon(Icons::BUILDSTEP_MOVEUP.icon());
    hbox->addWidget(m_upButton);

    m_downButton = new QToolButton(m_secondWidget);
    m_downButton->setAutoRaise(true);
    m_downButton->setToolTip(BuildStepListWidget::tr("Move Down"));
    m_downButton->setFixedSize(buttonSize);
    m_downButton->setIcon(Icons::BUILDSTEP_MOVEDOWN.icon());
    hbox->addWidget(m_downButton);

    m_removeButton  = new QToolButton(m_secondWidget);
    m_removeButton->setAutoRaise(true);
    m_removeButton->setToolTip(BuildStepListWidget::tr("Remove Item"));
    m_removeButton->setFixedSize(buttonSize);
    m_removeButton->setIcon(Icons::BUILDSTEP_REMOVE.icon());
    hbox->addWidget(m_removeButton);

    layout->addWidget(m_secondWidget);

    connect(m_disableButton, &QAbstractButton::clicked, this, &ToolWidget::disabledClicked);
    connect(m_upButton, &QAbstractButton::clicked, this, &ToolWidget::upClicked);
    connect(m_downButton, &QAbstractButton::clicked, this, &ToolWidget::downClicked);
    connect(m_removeButton, &QAbstractButton::clicked, this, &ToolWidget::removeClicked);
}

void ToolWidget::setOpacity(qreal value)
{
    m_targetOpacity = value;
    if (m_buildStepEnabled)
        m_firstWidget->setOpacity(value);
    m_secondWidget->setOpacity(value);
}

void ToolWidget::fadeTo(qreal value)
{
    m_targetOpacity = value;
    if (m_buildStepEnabled)
        m_firstWidget->fadeTo(value);
    m_secondWidget->fadeTo(value);
}

void ToolWidget::setBuildStepEnabled(bool b)
{
    m_buildStepEnabled = b;
    if (m_buildStepEnabled) {
        if (HostOsInfo::isMacHost())
            m_firstWidget->setOpacity(m_targetOpacity);
        else
            m_firstWidget->fadeTo(m_targetOpacity);
    } else {
        if (HostOsInfo::isMacHost())
            m_firstWidget->setOpacity(1.0);
        else
            m_firstWidget->fadeTo(1.0);
    }
    m_disableButton->setChecked(!b);
}

void ToolWidget::setUpEnabled(bool b)
{
    m_upButton->setEnabled(b);
}

void ToolWidget::setDownEnabled(bool b)
{
    m_downButton->setEnabled(b);
}

void ToolWidget::setRemoveEnabled(bool b)
{
    m_removeButton->setEnabled(b);
}

void ToolWidget::setUpVisible(bool b)
{
    m_upButton->setVisible(b);
}

void ToolWidget::setDownVisible(bool b)
{
    m_downButton->setVisible(b);
}

BuildStepsWidgetData::BuildStepsWidgetData(BuildStep *s) :
    step(s), widget(0), detailsWidget(0)
{
    widget = s->createConfigWidget();
    Q_ASSERT(widget);

    detailsWidget = new DetailsWidget;
    detailsWidget->setWidget(widget);

    toolWidget = new ToolWidget(detailsWidget);
    toolWidget->setBuildStepEnabled(step->enabled());

    detailsWidget->setToolWidget(toolWidget);
    detailsWidget->setContentsMargins(0, 0, 0, 1);
    detailsWidget->setSummaryText(widget->summaryText());
    detailsWidget->setAdditionalSummaryText(widget->additionalSummaryText());
}

BuildStepsWidgetData::~BuildStepsWidgetData()
{
    delete detailsWidget; // other widgets are children of that!
    // We do not own the step
}

BuildStepListWidget::BuildStepListWidget(QWidget *parent) :
    NamedWidget(parent)
{ }

BuildStepListWidget::~BuildStepListWidget()
{
    qDeleteAll(m_buildStepsData);
    m_buildStepsData.clear();
}

void BuildStepListWidget::updateSummary()
{
    auto widget = qobject_cast<BuildStepConfigWidget *>(sender());
    if (widget) {
        foreach (const BuildStepsWidgetData *s, m_buildStepsData) {
            if (s->widget == widget) {
                s->detailsWidget->setSummaryText(widget->summaryText());
                break;
            }
        }
    }
}

void BuildStepListWidget::updateAdditionalSummary()
{
    auto widget = qobject_cast<BuildStepConfigWidget *>(sender());
    if (widget) {
        foreach (const BuildStepsWidgetData *s, m_buildStepsData) {
            if (s->widget == widget) {
                s->detailsWidget->setAdditionalSummaryText(widget->additionalSummaryText());
                break;
            }
        }
    }
}

void BuildStepListWidget::updateEnabledState()
{
    auto step = qobject_cast<BuildStep *>(sender());
    if (step) {
        foreach (const BuildStepsWidgetData *s, m_buildStepsData) {
            if (s->step == step) {
                s->toolWidget->setBuildStepEnabled(step->enabled());
                break;
            }
        }
    }
}

void BuildStepListWidget::init(BuildStepList *bsl)
{
    Q_ASSERT(bsl);
    if (bsl == m_buildStepList)
        return;

    setupUi();

    if (m_buildStepList) {
        disconnect(m_buildStepList, &BuildStepList::stepInserted,
                   this, &BuildStepListWidget::addBuildStep);
        disconnect(m_buildStepList, &BuildStepList::stepRemoved,
                   this, &BuildStepListWidget::removeBuildStep);
        disconnect(m_buildStepList, &BuildStepList::stepMoved,
                   this, &BuildStepListWidget::stepMoved);
    }

    connect(bsl, &BuildStepList::stepInserted, this, &BuildStepListWidget::addBuildStep);
    connect(bsl, &BuildStepList::stepRemoved, this, &BuildStepListWidget::removeBuildStep);
    connect(bsl, &BuildStepList::stepMoved, this, &BuildStepListWidget::stepMoved);

    qDeleteAll(m_buildStepsData);
    m_buildStepsData.clear();

    m_buildStepList = bsl;
    //: %1 is the name returned by BuildStepList::displayName
    setDisplayName(tr("%1 Steps").arg(m_buildStepList->displayName()));

    for (int i = 0; i < bsl->count(); ++i) {
        addBuildStep(i);
        // addBuilStep expands the config widget by default, which we don't want here
        if (m_buildStepsData.at(i)->widget->showWidget())
            m_buildStepsData.at(i)->detailsWidget->setState(DetailsWidget::Collapsed);
    }

    m_noStepsLabel->setVisible(bsl->isEmpty());
    m_noStepsLabel->setText(tr("No %1 Steps").arg(m_buildStepList->displayName()));

    m_addButton->setText(tr("Add %1 Step").arg(m_buildStepList->displayName()));

    updateBuildStepButtonsState();
}

void BuildStepListWidget::updateAddBuildStepMenu()
{
    QMap<QString, QPair<Core::Id, IBuildStepFactory *> > map;
    //Build up a list of possible steps and save map the display names to the (internal) name and factories.
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::getObjects<IBuildStepFactory>();
    foreach (IBuildStepFactory *factory, factories) {
        const QList<BuildStepInfo> infos = factory->availableSteps(m_buildStepList);
        for (const BuildStepInfo &info : infos) {
            if (info.flags & BuildStepInfo::Uncreatable)
                continue;
            if ((info.flags & BuildStepInfo::UniqueStep) && m_buildStepList->contains(info.id))
                continue;
            map.insert(info.displayName, qMakePair(info.id, factory));
        }
    }

    // Ask the user which one to add
    QMenu *menu = m_addButton->menu();
    menu->clear();
    if (!map.isEmpty()) {
        QMap<QString, QPair<Core::Id, IBuildStepFactory *> >::const_iterator it, end;
        end = map.constEnd();
        for (it = map.constBegin(); it != end; ++it) {
            QAction *action = menu->addAction(it.key());
            IBuildStepFactory *factory = it.value().second;
            Core::Id id = it.value().first;

            connect(action, &QAction::triggered, [id, factory, this]() {
                BuildStep *newStep = factory->create(m_buildStepList, id);
                QTC_ASSERT(newStep, return);
                int pos = m_buildStepList->count();
                m_buildStepList->insertStep(pos, newStep);
            });
        }
    }
}

void BuildStepListWidget::addBuildStepWidget(int pos, BuildStep *step)
{
    // create everything
    auto s = new BuildStepsWidgetData(step);
    m_buildStepsData.insert(pos, s);

    m_vbox->insertWidget(pos, s->detailsWidget);

    connect(s->widget, &BuildStepConfigWidget::updateSummary,
            this, &BuildStepListWidget::updateSummary);
    connect(s->widget, &BuildStepConfigWidget::updateAdditionalSummary,
            this, &BuildStepListWidget::updateAdditionalSummary);

    connect(s->step, &BuildStep::enabledChanged,
            this, &BuildStepListWidget::updateEnabledState);
}

void BuildStepListWidget::addBuildStep(int pos)
{
    BuildStep *newStep = m_buildStepList->at(pos);
    addBuildStepWidget(pos, newStep);
    BuildStepsWidgetData *s = m_buildStepsData.at(pos);
    // Expand new build steps by default
    if (s->widget->showWidget())
        s->detailsWidget->setState(DetailsWidget::Expanded);
    else
        s->detailsWidget->setState(DetailsWidget::OnlySummary);

    m_noStepsLabel->setVisible(false);
    updateBuildStepButtonsState();
}

void BuildStepListWidget::stepMoved(int from, int to)
{
    m_vbox->insertWidget(to, m_buildStepsData.at(from)->detailsWidget);

    Internal::BuildStepsWidgetData *data = m_buildStepsData.at(from);
    m_buildStepsData.removeAt(from);
    m_buildStepsData.insert(to, data);

    updateBuildStepButtonsState();
}

void BuildStepListWidget::removeBuildStep(int pos)
{
    delete m_buildStepsData.takeAt(pos);

    updateBuildStepButtonsState();

    bool hasSteps = m_buildStepList->isEmpty();
    m_noStepsLabel->setVisible(hasSteps);
}

void BuildStepListWidget::setupUi()
{
    if (m_addButton)
        return;

    m_vbox = new QVBoxLayout(this);
    m_vbox->setContentsMargins(0, 0, 0, 0);
    m_vbox->setSpacing(0);

    m_noStepsLabel = new QLabel(tr("No Build Steps"), this);
    m_noStepsLabel->setContentsMargins(0, 0, 0, 0);
    m_vbox->addWidget(m_noStepsLabel);

    auto hboxLayout = new QHBoxLayout();
    hboxLayout->setContentsMargins(0, 4, 0, 0);
    m_addButton = new QPushButton(this);
    m_addButton->setMenu(new QMenu(this));
    hboxLayout->addWidget(m_addButton);

    hboxLayout->addStretch(10);

    if (HostOsInfo::isMacHost())
        m_addButton->setAttribute(Qt::WA_MacSmallSize);

    m_vbox->addLayout(hboxLayout);

    connect(m_addButton->menu(), &QMenu::aboutToShow,
            this, &BuildStepListWidget::updateAddBuildStepMenu);
}

void BuildStepListWidget::updateBuildStepButtonsState()
{
    if (m_buildStepsData.count() != m_buildStepList->count())
        return;
    for (int i = 0; i < m_buildStepsData.count(); ++i) {
        BuildStepsWidgetData *s = m_buildStepsData.at(i);
        disconnect(s->toolWidget, nullptr, this, nullptr);
        connect(s->toolWidget, &ToolWidget::disabledClicked,
                this, [s] {
            BuildStep *bs = s->step;
            bs->setEnabled(!bs->enabled());
            s->toolWidget->setBuildStepEnabled(bs->enabled());
        });
        s->toolWidget->setRemoveEnabled(!m_buildStepList->at(i)->immutable());
        connect(s->toolWidget, &ToolWidget::removeClicked,
                this, [this, i] {
            if (!m_buildStepList->removeStep(i)) {
                QMessageBox::warning(Core::ICore::mainWindow(),
                                     tr("Removing Step failed"),
                                     tr("Cannot remove build step while building"),
                                     QMessageBox::Ok, QMessageBox::Ok);
            }
        });

        s->toolWidget->setUpEnabled((i > 0)
                                    && !(m_buildStepList->at(i)->immutable()
                                         && m_buildStepList->at(i - 1)->immutable()));
        connect(s->toolWidget, &ToolWidget::upClicked,
                this, [this, i] { m_buildStepList->moveStepUp(i); });
        s->toolWidget->setDownEnabled((i + 1 < m_buildStepList->count())
                                      && !(m_buildStepList->at(i)->immutable()
                                           && m_buildStepList->at(i + 1)->immutable()));
        connect(s->toolWidget, &ToolWidget::downClicked,
                this, [this, i] { m_buildStepList->moveStepUp(i + 1); });

        // Only show buttons when needed
        s->toolWidget->setDownVisible(m_buildStepList->count() != 1);
        s->toolWidget->setUpVisible(m_buildStepList->count() != 1);
    }
}

BuildStepsPage::BuildStepsPage(BuildConfiguration *bc, Core::Id id) :
    m_id(id),
    m_widget(new BuildStepListWidget(this))
{
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_widget);

    m_widget->init(bc->stepList(m_id));

    if (m_id == Constants::BUILDSTEPS_BUILD)
        setDisplayName(tr("Build Steps"));
    if (m_id == Constants::BUILDSTEPS_CLEAN)
        setDisplayName(tr("Clean Steps"));
}
