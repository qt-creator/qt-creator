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

#include "buildstepspage.h"
#include "buildconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QSignalMapper>

#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QMenu>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

BuildStepsPage::BuildStepsPage(Project *project, bool clean) :
    BuildConfigWidget(),
    m_clean(clean),
    m_addButton(0),
    m_leftMargin(-1)
{
    Q_UNUSED(project);
}

BuildStepsPage::~BuildStepsPage()
{
    foreach(BuildStepsWidgetStruct s, m_buildSteps) {
        delete s.widget;
        delete s.detailsWidget;
    }
    m_buildSteps.clear();
}

void BuildStepsPage::updateSummary()
{
    BuildStepConfigWidget *widget = qobject_cast<BuildStepConfigWidget *>(sender());
    if (widget) {
        foreach(const BuildStepsWidgetStruct &s, m_buildSteps) {
            if (s.widget == widget) {
                s.detailsWidget->setSummaryText(widget->summaryText());
                break;
            }
        }
    }
}

QString BuildStepsPage::displayName() const
{
    return m_clean ? tr("Clean Steps") : tr("Build Steps");
}

void BuildStepsPage::init(BuildConfiguration *bc)
{
    QTC_ASSERT(bc, return);

    setupUi();

    foreach(BuildStepsWidgetStruct s, m_buildSteps) {
        delete s.widget;
        delete s.detailsWidget;
    }
    m_buildSteps.clear();

    m_configuration = bc;

    const QList<BuildStep *> &steps = m_clean ? m_configuration->cleanSteps() : m_configuration->buildSteps();
    int i = 0;
    foreach (BuildStep *bs, steps) {
        addBuildStepWidget(i, bs);
        ++i;
    }

    m_noStepsLabel->setVisible(steps.isEmpty());

    // make sure widget is updated
    foreach(BuildStepsWidgetStruct s, m_buildSteps) {
        s.widget->init();
        s.detailsWidget->setSummaryText(s.widget->summaryText());
    }
    updateBuildStepButtonsState();
}

void BuildStepsPage::updateAddBuildStepMenu()
{
    QMap<QString, QPair<QString, IBuildStepFactory *> > map;
    //Build up a list of possible steps and save map the display names to the (internal) name and factories.
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach (IBuildStepFactory *factory, factories) {
        QStringList ids = factory->canCreateForBuildConfiguration(m_configuration);
        foreach (const QString &id, ids) {
            map.insert(factory->displayNameForId(id), QPair<QString, IBuildStepFactory *>(id, factory));
        }
    }

    // Ask the user which one to add
    QMenu *menu = m_addButton->menu();
    m_addBuildStepHash.clear();
    menu->clear();
    if (!map.isEmpty()) {
        QStringList names;
        QMap<QString, QPair<QString, IBuildStepFactory *> >::const_iterator it, end;
        end = map.constEnd();
        for (it = map.constBegin(); it != end; ++it) {
            QAction *action = menu->addAction(it.key());
            connect(action, SIGNAL(triggered()),
                    this, SLOT(addBuildStep()));
            m_addBuildStepHash.insert(action, it.value());
        }
    }
}

void BuildStepsPage::addBuildStepWidget(int pos, BuildStep *step)
{
    // create everything
    BuildStepsWidgetStruct s;
    s.widget = step->createConfigWidget();
    s.detailsWidget = new Utils::DetailsWidget(this);
    s.detailsWidget->setSummaryText(s.widget->summaryText());
    s.detailsWidget->setWidget(s.widget);

    s.upButton = new QToolButton(this);
    s.upButton->setArrowType(Qt::UpArrow);
    s.upButton->setMaximumHeight(22);
    s.upButton->setMaximumWidth(22);
    s.downButton = new QToolButton(this);
    s.downButton->setArrowType(Qt::DownArrow);
    s.downButton->setMaximumHeight(22);
    s.downButton->setMaximumWidth(22);
#ifdef Q_OS_MAC
    s.upButton->setIconSize(QSize(10, 10));
    s.downButton->setIconSize(QSize(10, 10));
#endif
    s.removeButton = new QPushButton(this);
    s.removeButton->setText(QChar('X'));
    s.removeButton->setMaximumHeight(22);
    s.removeButton->setMaximumWidth(22);

    // layout
    QWidget *toolWidget = new QWidget(s.detailsWidget);
    toolWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *hbox = new QHBoxLayout();
    toolWidget->setLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(s.upButton);
    hbox->addWidget(s.downButton);
    hbox->addWidget(s.removeButton);

    s.detailsWidget->setToolWidget(toolWidget);

    const int leftMargin(qMax(m_leftMargin - toolWidget->width(), 0));
    s.detailsWidget->setContentsMargins(leftMargin, 0, 0, 1);

    m_buildSteps.insert(pos, s);

    m_vbox->insertWidget(pos, s.detailsWidget);

    connect(s.widget, SIGNAL(updateSummary()),
            this, SLOT(updateSummary()));

    connect(s.upButton, SIGNAL(clicked()),
            m_upMapper, SLOT(map()));
    connect(s.downButton, SIGNAL(clicked()),
            m_downMapper, SLOT(map()));
    connect(s.removeButton, SIGNAL(clicked()),
            m_removeMapper, SLOT(map()));
}

void BuildStepsPage::addBuildStep()
{
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        QPair<QString, IBuildStepFactory *> pair = m_addBuildStepHash.value(action);
        BuildStep *newStep = pair.second->create(m_configuration, pair.first);
        int pos = m_clean ? m_configuration->cleanSteps().count() : m_configuration->buildSteps().count();
        m_clean ? m_configuration->insertCleanStep(pos, newStep) : m_configuration->insertBuildStep(pos, newStep);

        addBuildStepWidget(pos, newStep);
        const BuildStepsWidgetStruct s = m_buildSteps.at(pos);
        s.widget->init();
        s.detailsWidget->setSummaryText(s.widget->summaryText());
        s.detailsWidget->setExpanded(true);
    }

    m_noStepsLabel->setVisible(false);
    updateBuildStepButtonsState();
}

void BuildStepsPage::stepMoveUp(int pos)
{
    m_clean ? m_configuration->moveCleanStepUp(pos) : m_configuration->moveBuildStepUp(pos);

    m_vbox->insertWidget(pos - 1, m_buildSteps.at(pos).detailsWidget);

    m_buildSteps.swap(pos - 1, pos);

    updateBuildStepButtonsState();
}

void BuildStepsPage::stepMoveDown(int pos)
{
    stepMoveUp(pos + 1);
}

void BuildStepsPage::stepRemove(int pos)
{
    BuildStepsWidgetStruct s = m_buildSteps.at(pos);
    delete s.widget;
    delete s.detailsWidget;
    m_buildSteps.removeAt(pos);
    m_clean ? m_configuration->removeCleanStep(pos) : m_configuration->removeBuildStep(pos);

    updateBuildStepButtonsState();

    bool hasSteps(m_clean ? m_configuration->cleanSteps().isEmpty() :
                            m_configuration->buildSteps().isEmpty());
    m_noStepsLabel->setVisible(hasSteps);
}

void BuildStepsPage::setupUi()
{
    if (0 != m_addButton)
        return;

    QMargins margins(contentsMargins());
    m_leftMargin = margins.left();
    margins.setLeft(0);
    setContentsMargins(margins);

    m_upMapper = new QSignalMapper(this);
    connect(m_upMapper, SIGNAL(mapped(int)),
            this, SLOT(stepMoveUp(int)));
    m_downMapper = new QSignalMapper(this);
    connect(m_downMapper, SIGNAL(mapped(int)),
            this, SLOT(stepMoveDown(int)));
    m_removeMapper = new QSignalMapper(this);
    connect(m_removeMapper, SIGNAL(mapped(int)),
            this, SLOT(stepRemove(int)));

    m_vbox = new QVBoxLayout(this);
    m_vbox->setContentsMargins(0, 0, 0, 0);
    m_vbox->setSpacing(0);

    m_noStepsLabel = new QLabel(tr("No Build Steps"), this);
    m_noStepsLabel->setContentsMargins(m_leftMargin, 0, 0, 0);
    m_vbox->addWidget(m_noStepsLabel);

    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->setContentsMargins(m_leftMargin, 4, 0, 0);
    m_addButton = new QPushButton(this);
    m_addButton->setText(m_clean ? tr("Add clean step") :  tr("Add build step"));
    m_addButton->setMenu(new QMenu(this));
    hboxLayout->addWidget(m_addButton);

    hboxLayout->addStretch(10);

#ifdef Q_OS_MAC
    m_addButton->setAttribute(Qt::WA_MacSmallSize);
#endif

    m_vbox->addLayout(hboxLayout);

    connect(m_addButton->menu(), SIGNAL(aboutToShow()),
            this, SLOT(updateAddBuildStepMenu()));
}

void BuildStepsPage::updateBuildStepButtonsState()
{
    const QList<BuildStep *> &steps = m_clean ? m_configuration->cleanSteps() : m_configuration->buildSteps();
    for(int i = 0; i < m_buildSteps.count(); ++i) {
        BuildStepsWidgetStruct s = m_buildSteps.at(i);
        s.removeButton->setEnabled(!steps.at(i)->immutable());
        m_removeMapper->setMapping(s.removeButton, i);
        s.upButton->setEnabled((i > 0) && !(steps.at(i)->immutable() && steps.at(i - 1)));
        m_upMapper->setMapping(s.upButton, i);
        s.downButton->setEnabled((i + 1 < steps.count()) && !(steps.at(i)->immutable() && steps.at(i + 1)->immutable()));
        m_downMapper->setMapping(s.downButton, i);
    }
}
