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
#include "projectexplorer.h"
#include "ui_projectwelcomepagewidget.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/mainwindow.h>
#include <coreplugin/filemanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QPair>
#include <QtGui/QLabel>
#include <QtGui/QTreeWidgetItem>

#include <QtCore/QDebug>

#define MAX_RECENT_PROJECT_ITEMS 6

#ifdef Q_OS_MAC
#  define MAX_RECENT_SESSION_ITEMS 8
#else
#  define MAX_RECENT_SESSION_ITEMS 9
#endif

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
    updateWelcomePage(WelcomePageData());

    connect(ui->sessTreeWidget, SIGNAL(activated(QString)), SLOT(slotSessionClicked(QString)));
    connect(ui->projTreeWidget, SIGNAL(activated(QString)), SLOT(slotProjectClicked(QString)));
    connect(ui->createNewProjectButton, SIGNAL(clicked()), SLOT(slotCreateNewProject()));
    connect(ui->openProjectButton, SIGNAL(clicked()),
            ProjectExplorer::ProjectExplorerPlugin::instance(),
            SLOT(openOpenProjectDialog()));
    connect(ui->manageSessionsButton, SIGNAL(clicked()), SIGNAL(manageSessions()));

    ui->createNewProjectButton->setIcon(
            QIcon::fromTheme("document-new", ui->createNewProjectButton->icon()));
    ui->openProjectButton->setIcon(
            QIcon::fromTheme("document-open", ui->openProjectButton->icon()));
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
        int items = 0;
        foreach (const QString &s, welcomePageData.sessionList) {
            if (++items > MAX_RECENT_SESSION_ITEMS)
                break;
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
    }

    typedef QPair<QString, QString> QStringPair;
    if (welcomePageData.projectList.count() > 0) {
        int items = 0;
        QFontMetrics fm = fontMetrics();
        foreach (const QStringPair &it, welcomePageData.projectList) {
            if (++items > MAX_RECENT_PROJECT_ITEMS)
                break;
            const QFileInfo fi(it.first);
            QString label = "<b>" + it.second +
                            "</b><br><font color=gray>" +
                            fm.elidedText(it.first, Qt::ElideMiddle, 250);
            ui->projTreeWidget->addItem(label, it.first,
                                        QDir::toNativeSeparators(fi.absolutePath()));
        }
        ui->projTreeWidget->updateGeometry();
    }
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
    Core::ICore::instance()->showNewItemDialog(tr("New Project"),
                                               Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard));
}
