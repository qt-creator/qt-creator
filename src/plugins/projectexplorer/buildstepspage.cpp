/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "buildstepspage.h"

#include "buildconfiguration.h"
#include "buildsteplist.h"
#include "projectexplorerconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>

#include <QSignalMapper>

#include <QPropertyAnimation>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMessageBox>
#include <QGraphicsOpacityEffect>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace Utils;

ToolWidget::ToolWidget(QWidget *parent)
    : Utils::FadingPanel(parent), m_buildStepEnabled(true), m_targetOpacity(1.0f)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(4);
    layout->setSpacing(4);
    setLayout(layout);
    m_firstWidget = new Utils::FadingWidget(this);
    m_firstWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);
    m_firstWidget->setLayout(hbox);
    QSize buttonSize(20, HostOsInfo::isMacHost() ? 20 : 26);

    m_disableButton = new QToolButton(m_firstWidget);
    m_disableButton->setAutoRaise(true);
    m_disableButton->setToolTip(BuildStepListWidget::tr("Disable"));
    m_disableButton->setFixedSize(buttonSize);
    m_disableButton->setIcon(QIcon(QLatin1String(":/projectexplorer/images/disabledbuildstep.png")));
    m_disableButton->setCheckable(true);
    hbox->addWidget(m_disableButton);
    layout->addWidget(m_firstWidget);

    m_secondWidget = new Utils::FadingWidget(this);
    m_secondWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(4);
    m_secondWidget->setLayout(hbox);

    m_upButton = new QToolButton(m_secondWidget);
    m_upButton->setAutoRaise(true);
    m_upButton->setToolTip(BuildStepListWidget::tr("Move Up"));
    m_upButton->setFixedSize(buttonSize);
    m_upButton->setIcon(QIcon(QLatin1String(":/core/images/darkarrowup.png")));
    hbox->addWidget(m_upButton);

    m_downButton = new QToolButton(m_secondWidget);
    m_downButton->setAutoRaise(true);
    m_downButton->setToolTip(BuildStepListWidget::tr("Move Down"));
    m_downButton->setFixedSize(buttonSize);
    m_downButton->setIcon(QIcon(QLatin1String(":/core/images/darkarrowdown.png")));
    hbox->addWidget(m_downButton);

    m_removeButton  = new QToolButton(m_secondWidget);
    m_removeButton->setAutoRaise(true);
    m_removeButton->setToolTip(BuildStepListWidget::tr("Remove Item"));
    m_removeButton->setFixedSize(buttonSize);
    m_removeButton->setIcon(QIcon(QLatin1String(":/core/images/darkclose.png")));
    hbox->addWidget(m_removeButton);

    layout->addWidget(m_secondWidget);

    connect(m_disableButton, SIGNAL(clicked()), this, SIGNAL(disabledClicked()));
    connect(m_upButton, SIGNAL(clicked()), this, SIGNAL(upClicked()));
    connect(m_downButton, SIGNAL(clicked()), this, SIGNAL(downClicked()));
    connect(m_removeButton, SIGNAL(clicked()), this, SIGNAL(removeClicked()));
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

    detailsWidget = new Utils::DetailsWidget;
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
    NamedWidget(parent),
    m_buildStepList(0),
    m_addButton(0)
{
}

BuildStepListWidget::~BuildStepListWidget()
{
    qDeleteAll(m_buildStepsData);
    m_buildStepsData.clear();
}

void BuildStepListWidget::updateSummary()
{
    BuildStepConfigWidget *widget = qobject_cast<BuildStepConfigWidget *>(sender());
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
    BuildStepConfigWidget *widget = qobject_cast<BuildStepConfigWidget *>(sender());
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
    BuildStep *step = qobject_cast<BuildStep *>(sender());
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
        disconnect(m_buildStepList, SIGNAL(stepInserted(int)), this, SLOT(addBuildStep(int)));
        disconnect(m_buildStepList, SIGNAL(stepRemoved(int)), this, SLOT(removeBuildStep(int)));
        disconnect(m_buildStepList, SIGNAL(stepMoved(int,int)), this, SLOT(stepMoved(int,int)));
    }

    connect(bsl, SIGNAL(stepInserted(int)), this, SLOT(addBuildStep(int)));
    connect(bsl, SIGNAL(stepRemoved(int)), this, SLOT(removeBuildStep(int)));
    connect(bsl, SIGNAL(stepMoved(int,int)), this, SLOT(stepMoved(int,int)));

    qDeleteAll(m_buildStepsData);
    m_buildStepsData.clear();

    m_buildStepList = bsl;
    //: %1 is the name returned by BuildStepList::displayName
    setDisplayName(tr("%1 Steps").arg(m_buildStepList->displayName()));

    for (int i = 0; i < bsl->count(); ++i) {
        addBuildStep(i);
        // addBuilStep expands the config widget by default, which we don't want here
        if (m_buildStepsData.at(i)->widget->showWidget())
            m_buildStepsData.at(i)->detailsWidget->setState(Utils::DetailsWidget::Collapsed);
    }

    m_noStepsLabel->setVisible(bsl->isEmpty());
    m_noStepsLabel->setText(tr("No %1 Steps").arg(m_buildStepList->displayName()));

    m_addButton->setText(tr("Add %1 Step").arg(m_buildStepList->displayName()));

    updateBuildStepButtonsState();

    static QLatin1String buttonStyle(
            "QToolButton{ border-width: 2;}"
            "QToolButton:hover{border-image: url(:/welcome/images/btn_26_hover.png) 4;}"
            "QToolButton:pressed{ border-image: url(:/welcome/images/btn_26_pressed.png) 4;}");
    setStyleSheet(buttonStyle);
}

void BuildStepListWidget::updateAddBuildStepMenu()
{
    QMap<QString, QPair<Core::Id, IBuildStepFactory *> > map;
    //Build up a list of possible steps and save map the display names to the (internal) name and factories.
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::getObjects<IBuildStepFactory>();
    foreach (IBuildStepFactory *factory, factories) {
        QList<Core::Id> ids = factory->availableCreationIds(m_buildStepList);
        foreach (Core::Id id, ids)
            map.insert(factory->displayNameForId(id), QPair<Core::Id, IBuildStepFactory *>(id, factory));
    }

    // Ask the user which one to add
    QMenu *menu = m_addButton->menu();
    m_addBuildStepHash.clear();
    menu->clear();
    if (!map.isEmpty()) {
        QMap<QString, QPair<Core::Id, IBuildStepFactory *> >::const_iterator it, end;
        end = map.constEnd();
        for (it = map.constBegin(); it != end; ++it) {
            QAction *action = menu->addAction(it.key());
            connect(action, SIGNAL(triggered()),
                    this, SLOT(triggerAddBuildStep()));
            m_addBuildStepHash.insert(action, it.value());
        }
    }
}

void BuildStepListWidget::addBuildStepWidget(int pos, BuildStep *step)
{
    // create everything
    BuildStepsWidgetData *s = new BuildStepsWidgetData(step);
    m_buildStepsData.insert(pos, s);

    m_vbox->insertWidget(pos, s->detailsWidget);

    connect(s->widget, SIGNAL(updateSummary()),
            this, SLOT(updateSummary()));
    connect(s->widget, SIGNAL(updateAdditionalSummary()),
            this, SLOT(updateAdditionalSummary()));

    connect(s->step, SIGNAL(enabledChanged()),
            this, SLOT(updateEnabledState()));

    connect(s->toolWidget, SIGNAL(disabledClicked()),
            m_disableMapper, SLOT(map()));
    connect(s->toolWidget, SIGNAL(upClicked()),
            m_upMapper, SLOT(map()));
    connect(s->toolWidget, SIGNAL(downClicked()),
            m_downMapper, SLOT(map()));
    connect(s->toolWidget, SIGNAL(removeClicked()),
            m_removeMapper, SLOT(map()));
}

void BuildStepListWidget::triggerAddBuildStep()
{
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        QPair<Core::Id, IBuildStepFactory *> pair = m_addBuildStepHash.value(action);
        BuildStep *newStep = pair.second->create(m_buildStepList, pair.first);
        QTC_ASSERT(newStep, return);
        int pos = m_buildStepList->count();
        m_buildStepList->insertStep(pos, newStep);
    }
}

void BuildStepListWidget::addBuildStep(int pos)
{
    BuildStep *newStep = m_buildStepList->at(pos);
    addBuildStepWidget(pos, newStep);
    BuildStepsWidgetData *s = m_buildStepsData.at(pos);
    // Expand new build steps by default
    if (s->widget->showWidget())
        s->detailsWidget->setState(Utils::DetailsWidget::Expanded);
    else
        s->detailsWidget->setState(Utils::DetailsWidget::OnlySummary);

    m_noStepsLabel->setVisible(false);
    updateBuildStepButtonsState();
}

void BuildStepListWidget::triggerStepMoveUp(int pos)
{
    m_buildStepList->moveStepUp(pos);
}

void BuildStepListWidget::stepMoved(int from, int to)
{
    m_vbox->insertWidget(to, m_buildStepsData.at(from)->detailsWidget);

    Internal::BuildStepsWidgetData *data = m_buildStepsData.at(from);
    m_buildStepsData.removeAt(from);
    m_buildStepsData.insert(to, data);

    updateBuildStepButtonsState();
}

void BuildStepListWidget::triggerStepMoveDown(int pos)
{
    triggerStepMoveUp(pos + 1);
}

void BuildStepListWidget::triggerRemoveBuildStep(int pos)
{
    if (!m_buildStepList->removeStep(pos)) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("Removing Step failed"),
                             tr("Cannot remove build step while building"),
                             QMessageBox::Ok, QMessageBox::Ok);
    }
}

void BuildStepListWidget::removeBuildStep(int pos)
{
    delete m_buildStepsData.takeAt(pos);

    updateBuildStepButtonsState();

    bool hasSteps = m_buildStepList->isEmpty();
    m_noStepsLabel->setVisible(hasSteps);
}

void BuildStepListWidget::triggerDisable(int pos)
{
    BuildStep *bs = m_buildStepsData.at(pos)->step;
    bs->setEnabled(!bs->enabled());
    m_buildStepsData.at(pos)->toolWidget->setBuildStepEnabled(bs->enabled());
}

void BuildStepListWidget::setupUi()
{
    if (0 != m_addButton)
        return;

    m_disableMapper = new QSignalMapper(this);
    connect(m_disableMapper, SIGNAL(mapped(int)),
            this, SLOT(triggerDisable(int)));
    m_upMapper = new QSignalMapper(this);
    connect(m_upMapper, SIGNAL(mapped(int)),
            this, SLOT(triggerStepMoveUp(int)));
    m_downMapper = new QSignalMapper(this);
    connect(m_downMapper, SIGNAL(mapped(int)),
            this, SLOT(triggerStepMoveDown(int)));
    m_removeMapper = new QSignalMapper(this);
    connect(m_removeMapper, SIGNAL(mapped(int)),
            this, SLOT(triggerRemoveBuildStep(int)));

    m_vbox = new QVBoxLayout(this);
    m_vbox->setContentsMargins(0, 0, 0, 0);
    m_vbox->setSpacing(0);

    m_noStepsLabel = new QLabel(tr("No Build Steps"), this);
    m_noStepsLabel->setContentsMargins(0, 0, 0, 0);
    m_vbox->addWidget(m_noStepsLabel);

    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->setContentsMargins(0, 4, 0, 0);
    m_addButton = new QPushButton(this);
    m_addButton->setMenu(new QMenu(this));
    hboxLayout->addWidget(m_addButton);

    hboxLayout->addStretch(10);

    if (HostOsInfo::isMacHost())
        m_addButton->setAttribute(Qt::WA_MacSmallSize);

    m_vbox->addLayout(hboxLayout);

    connect(m_addButton->menu(), SIGNAL(aboutToShow()),
            this, SLOT(updateAddBuildStepMenu()));
}

void BuildStepListWidget::updateBuildStepButtonsState()
{
    if (m_buildStepsData.count() != m_buildStepList->count())
        return;
    for (int i = 0; i < m_buildStepsData.count(); ++i) {
        BuildStepsWidgetData *s = m_buildStepsData.at(i);
        m_disableMapper->setMapping(s->toolWidget, i);
        s->toolWidget->setRemoveEnabled(!m_buildStepList->at(i)->immutable());
        m_removeMapper->setMapping(s->toolWidget, i);

        s->toolWidget->setUpEnabled((i > 0)
                                    && !(m_buildStepList->at(i)->immutable()
                                         && m_buildStepList->at(i - 1)->immutable()));
        m_upMapper->setMapping(s->toolWidget, i);
        s->toolWidget->setDownEnabled((i + 1 < m_buildStepList->count())
                                      && !(m_buildStepList->at(i)->immutable()
                                           && m_buildStepList->at(i + 1)->immutable()));
        m_downMapper->setMapping(s->toolWidget, i);

        // Only show buttons when needed
        s->toolWidget->setDownVisible(m_buildStepList->count() != 1);
        s->toolWidget->setUpVisible(m_buildStepList->count() != 1);
    }
}

BuildStepsPage::BuildStepsPage(BuildConfiguration *bc, Core::Id id) :
    NamedWidget(),
    m_id(id),
    m_widget(new BuildStepListWidget(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_widget);

    m_widget->init(bc->stepList(m_id));

    if (m_id == Constants::BUILDSTEPS_BUILD)
        setDisplayName(tr("Build Steps"));
    if (m_id == Constants::BUILDSTEPS_CLEAN)
        setDisplayName(tr("Clean Steps"));
}

BuildStepsPage::~BuildStepsPage()
{ }
