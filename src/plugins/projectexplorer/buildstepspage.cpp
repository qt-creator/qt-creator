/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "buildstepspage.h"

#include "buildconfiguration.h"
#include "buildsteplist.h"
#include "detailsbutton.h"
#include "projectexplorerconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/detailswidget.h>

#include <QtCore/QSignalMapper>

#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QMenu>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

BuildStepListWidget::BuildStepListWidget(QWidget *parent) :
    NamedWidget(parent),
    m_buildStepList(0),
    m_addButton(0)
{
    setStyleSheet("background: red");
}

BuildStepListWidget::~BuildStepListWidget()
{
    foreach(const BuildStepsWidgetStruct &s, m_buildSteps) {
        delete s.widget;
        delete s.detailsWidget;
    }
    m_buildSteps.clear();
}

void BuildStepListWidget::updateSummary()
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

void BuildStepListWidget::init(BuildStepList *bsl)
{
    Q_ASSERT(bsl);

    setupUi();

    foreach(BuildStepsWidgetStruct s, m_buildSteps) {
        delete s.widget;
        delete s.detailsWidget;
    }
    m_buildSteps.clear();

    m_buildStepList = bsl;
    //: %1 is the name returned by BuildStepList::displayName
    setDisplayName(tr("%1 Steps").arg(m_buildStepList->displayName()));

    for (int i = 0; i < bsl->count(); ++i)
        addBuildStepWidget(i, m_buildStepList->at(i));

    m_noStepsLabel->setVisible(bsl->isEmpty());
    m_noStepsLabel->setText(tr("No %1 Steps").arg(m_buildStepList->displayName()));

    m_addButton->setText(tr("Add %1 Step").arg(m_buildStepList->displayName()));

    // make sure widget is updated
    foreach(BuildStepsWidgetStruct s, m_buildSteps) {
        s.widget->init();
        s.detailsWidget->setSummaryText(s.widget->summaryText());
    }
    updateBuildStepButtonsState();

    static QLatin1String buttonStyle(
            "QToolButton{ border-width: 2;}"
            "QToolButton:hover{border-image: url(:/welcome/images/btn_26_hover.png) 4;}"
            "QToolButton:pressed{ border-image: url(:/welcome/images/btn_26_pressed.png) 4;}");
    setStyleSheet(buttonStyle);
}

void BuildStepListWidget::updateAddBuildStepMenu()
{
    QMap<QString, QPair<QString, IBuildStepFactory *> > map;
    //Build up a list of possible steps and save map the display names to the (internal) name and factories.
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach (IBuildStepFactory *factory, factories) {
        QStringList ids = factory->availableCreationIds(m_buildStepList);
        foreach (const QString &id, ids) {
            map.insert(factory->displayNameForId(id), QPair<QString, IBuildStepFactory *>(id, factory));
        }
    }

    // Ask the user which one to add
    QMenu *menu = m_addButton->menu();
    m_addBuildStepHash.clear();
    menu->clear();
    if (!map.isEmpty()) {
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

void BuildStepListWidget::addBuildStepWidget(int pos, BuildStep *step)
{
    // create everything
    BuildStepsWidgetStruct s;
    s.widget = step->createConfigWidget();
    Q_ASSERT(s.widget);
    s.widget->init();

    s.detailsWidget = new Utils::DetailsWidget(this);
    s.detailsWidget->setSummaryText(s.widget->summaryText());
    s.detailsWidget->setWidget(s.widget);

    // layout
    Utils::FadingPanel *toolWidget = new Utils::FadingPanel(s.detailsWidget);
#ifdef Q_WS_MAC
    QSize buttonSize(20, 20);
#else
    QSize buttonSize(20, 26);
#endif

    s.upButton = new QToolButton(toolWidget);
    s.upButton->setAutoRaise(true);
    s.upButton->setToolTip(tr("Move Up"));
    s.upButton->setFixedSize(buttonSize);
    s.upButton->setIcon(QIcon(QLatin1String(":/core/images/darkarrowup.png")));

    s.downButton = new QToolButton(toolWidget);
    s.downButton->setAutoRaise(true);
    s.downButton->setToolTip(tr("Move Down"));
    s.downButton->setFixedSize(buttonSize);
    s.downButton->setIcon(QIcon(QLatin1String(":/core/images/darkarrowdown.png")));

    s.removeButton  = new QToolButton(toolWidget);
    s.removeButton->setAutoRaise(true);
    s.removeButton->setToolTip(tr("Remove Item"));
    s.removeButton->setFixedSize(buttonSize);
    s.removeButton->setIcon(QIcon(QLatin1String(":/core/images/darkclose.png")));

    toolWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    QHBoxLayout *hbox = new QHBoxLayout();
    toolWidget->setLayout(hbox);
    hbox->setMargin(4);
    hbox->setSpacing(0);
    hbox->addWidget(s.upButton);
    hbox->addWidget(s.downButton);
    hbox->addWidget(s.removeButton);

    s.detailsWidget->setToolWidget(toolWidget);

    s.detailsWidget->setContentsMargins(0, 0, 0, 1);

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

void BuildStepListWidget::addBuildStep()
{
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        QPair<QString, IBuildStepFactory *> pair = m_addBuildStepHash.value(action);
        BuildStep *newStep = pair.second->create(m_buildStepList, pair.first);
        int pos = m_buildStepList->count();
        m_buildStepList->insertStep(pos, newStep);

        addBuildStepWidget(pos, newStep);
        const BuildStepsWidgetStruct s = m_buildSteps.at(pos);
        s.detailsWidget->setState(Utils::DetailsWidget::Expanded);
    }

    m_noStepsLabel->setVisible(false);
    updateBuildStepButtonsState();
}

void BuildStepListWidget::stepMoveUp(int pos)
{
    m_buildStepList->moveStepUp(pos);

    m_vbox->insertWidget(pos - 1, m_buildSteps.at(pos).detailsWidget);

    m_buildSteps.swap(pos - 1, pos);

    updateBuildStepButtonsState();
}

void BuildStepListWidget::stepMoveDown(int pos)
{
    stepMoveUp(pos + 1);
}

void BuildStepListWidget::stepRemove(int pos)
{
    if (m_buildStepList->removeStep(pos)) {
        BuildStepsWidgetStruct s = m_buildSteps.at(pos);
        delete s.widget;
        delete s.detailsWidget;
        m_buildSteps.removeAt(pos);

        updateBuildStepButtonsState();

        bool hasSteps = m_buildStepList->isEmpty();
        m_noStepsLabel->setVisible(hasSteps);
    } else {
        QMessageBox::warning(Core::ICore::instance()->mainWindow(),
                             tr("Removing Step failed"),
                             tr("Cannot remove build step while building"),
                             QMessageBox::Ok, QMessageBox::Ok);
    }
}

void BuildStepListWidget::setupUi()
{
    if (0 != m_addButton)
        return;

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
    m_noStepsLabel->setContentsMargins(0, 0, 0, 0);
    m_vbox->addWidget(m_noStepsLabel);

    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->setContentsMargins(0, 4, 0, 0);
    m_addButton = new QPushButton(this);
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

void BuildStepListWidget::updateBuildStepButtonsState()
{
    for(int i = 0; i < m_buildSteps.count(); ++i) {
        BuildStepsWidgetStruct s = m_buildSteps.at(i);
        s.removeButton->setEnabled(!m_buildStepList->at(i)->immutable());
        m_removeMapper->setMapping(s.removeButton, i);

        s.upButton->setEnabled((i > 0)
                               && !(m_buildStepList->at(i)->immutable()
                               && m_buildStepList->at(i - 1)));
        m_upMapper->setMapping(s.upButton, i);
        s.downButton->setEnabled((i + 1 < m_buildStepList->count())
                                 && !(m_buildStepList->at(i)->immutable()
                                      && m_buildStepList->at(i + 1)->immutable()));
        m_downMapper->setMapping(s.downButton, i);

        // Only show buttons when needed
        s.downButton->setVisible(m_buildStepList->count() != 1);
        s.upButton->setVisible(m_buildStepList->count() != 1);
    }
}

BuildStepsPage::BuildStepsPage(Target *target, const QString &id) :
    BuildConfigWidget(),
    m_id(id),
    m_widget(new BuildStepListWidget(this))
{
    Q_UNUSED(target);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_widget);
}

BuildStepsPage::~BuildStepsPage()
{ }

QString BuildStepsPage::displayName() const
{
    if (m_id == QLatin1String(Constants::BUILDSTEPS_BUILD))
        return tr("Build Steps");
    if (m_id == QLatin1String(Constants::BUILDSTEPS_CLEAN))
        return tr("Clean Steps");
    return QString();
}

void BuildStepsPage::init(BuildConfiguration *bc)
{
    m_widget->init(bc->stepList(m_id));
}
