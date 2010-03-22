/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "projectwelcomepagewidget.h"
#include "ui_projectwelcomepagewidget.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/dialogs/iwizard.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QPair>
#include <QtGui/QLabel>
#include <QtGui/QTreeWidgetItem>

#include <QtCore/QDebug>

using namespace ProjectExplorer::Internal;

bool ProjectWelcomePageWidget::WelcomePageData::operator==(const WelcomePageData &rhs) const
{
    return previousSession == rhs.previousSession
        && activeSession   == rhs.activeSession
        && sessionList     == rhs.sessionList
        && projectList     == rhs.projectList;
}

bool ProjectWelcomePageWidget::WelcomePageData::operator!=(const WelcomePageData &rhs) const
{
    return previousSession != rhs.previousSession
        || activeSession   != rhs.activeSession
        || sessionList     != rhs.sessionList
        || projectList     != rhs.projectList;
}

QDebug operator<<(QDebug dgb, const ProjectWelcomePageWidget::WelcomePageData &d)
{
    dgb.nospace() << "PreviousSession=" << d.previousSession
        << " activeSession=" << d.activeSession
        << " sessionList=" << d.sessionList
        << " projectList=" << d.projectList;
    return dgb;
}

ProjectWelcomePageWidget::ProjectWelcomePageWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProjectWelcomePageWidget)
{
    ui->setupUi(this);
    ui->projTitleLabel->setStyledText(tr("Open Recent Project"));
    ui->recentSessionsTitleLabel->setStyledText(tr("Resume Session"));
    updateWelcomePage(WelcomePageData());

    connect(ui->sessTreeWidget, SIGNAL(activated(QString)), SLOT(slotSessionClicked(QString)));
    connect(ui->projTreeWidget, SIGNAL(activated(QString)), SLOT(slotProjectClicked(QString)));
    connect(ui->createNewProjectButton, SIGNAL(clicked()), SLOT(slotCreateNewProject()));
    connect(ui->manageSessionsButton, SIGNAL(clicked()), SIGNAL(manageSessions()));
}

ProjectWelcomePageWidget::~ProjectWelcomePageWidget()
{
    delete ui;
}

void ProjectWelcomePageWidget::updateWelcomePage(const WelcomePageData &welcomePageData)
{
    // Update only if data are modified
    if (welcomePageData == lastData)
        return;
    lastData = welcomePageData;

    setUpdatesEnabled(false);
    ui->sessTreeWidget->clear();
    ui->projTreeWidget->clear();

    if (welcomePageData.sessionList.count() > 0) {
        foreach (const QString &s, welcomePageData.sessionList) {
            QString str = s;
            if (welcomePageData.activeSession.isEmpty()) {
                if (s == welcomePageData.previousSession)
                    str = tr("%1 (last session)").arg(s);
            } else {
                if (s == welcomePageData.activeSession)
                    str = tr("%1 (current session)").arg(s);
            }
            ui->sessTreeWidget->addItem(str, s);
        }
        ui->sessTreeWidget->updateGeometry();
        ui->sessTreeWidget->show();
    } else {
        ui->sessTreeWidget->hide();
    }

    typedef QPair<QString, QString> QStringPair;
    if (welcomePageData.projectList.count() > 0) {
        foreach (const QStringPair &it, welcomePageData.projectList) {
            const QFileInfo fi(it.first);
            ui->projTreeWidget->addItem(it.second, it.first,
                                        QDir::toNativeSeparators(fi.absolutePath()));
        }
    } else {
        ui->projTreeWidget->hide();
    }
    ui->projTreeWidget->updateGeometry();
    setUpdatesEnabled(true);
}

void ProjectWelcomePageWidget::activateEditMode()
{
    Core::ModeManager *modeManager = Core::ModeManager::instance();
    if (modeManager->currentMode() == modeManager->mode(Core::Constants::MODE_WELCOME))
        modeManager->activateMode(Core::Constants::MODE_EDIT);
}


void ProjectWelcomePageWidget::slotSessionClicked(const QString &data)
{
    emit requestSession(data);
    activateEditMode();
}

void ProjectWelcomePageWidget::slotProjectClicked(const QString &data)
{
    emit requestProject(data);
    activateEditMode();
}

void ProjectWelcomePageWidget::slotCreateNewProject()
{
    Core::ICore::instance()->showNewItemDialog(tr("New Project..."),
                                               Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard));
}
