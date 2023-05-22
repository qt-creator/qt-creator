// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "workspacedialog.h"

#include "advanceddockingsystemtr.h"
#include "dockmanager.h"
#include "workspaceview.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

namespace ADS {

WorkspaceDialog::WorkspaceDialog(DockManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
    , m_workspaceView(new WorkspaceView(manager))
    , m_btCreateNew(new QPushButton(Tr::tr("&New")))
    , m_btRename(new QPushButton(Tr::tr("&Rename")))
    , m_btClone(new QPushButton(Tr::tr("C&lone")))
    , m_btDelete(new QPushButton(Tr::tr("&Delete")))
    , m_btReset(new QPushButton(Tr::tr("Reset")))
    , m_btSwitch(new QPushButton(Tr::tr("&Switch To")))
    , m_btImport(new QPushButton(Tr::tr("Import")))
    , m_btExport(new QPushButton(Tr::tr("Export")))
    , m_btUp(new QPushButton(Tr::tr("Move Up")))
    , m_btDown(new QPushButton(Tr::tr("Move Down")))
    , m_autoLoadCheckBox(new QCheckBox(Tr::tr("Restore last workspace on startup")))
{
    setWindowTitle(Tr::tr("Workspace Manager"));
    resize(550, 380);

    m_workspaceView->setActivationMode(Utils::DoubleClickActivation);

    QLabel *whatsAWorkspaceLabel = new QLabel(Tr::tr("<a href=\"qthelp://org.qt-project.qtcreator/doc/"
           "creator-project-managing-workspaces.html\">What is a Workspace?</a>"));
    whatsAWorkspaceLabel->setOpenExternalLinks(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    using namespace Layouting;

    Column{Row{Column{m_workspaceView, m_autoLoadCheckBox},
               Column{m_btCreateNew,
                      m_btRename,
                      m_btClone,
                      m_btDelete,
                      m_btReset,
                      m_btSwitch,
                      st,
                      m_btUp,
                      m_btDown,
                      st,
                      m_btImport,
                      m_btExport}},
           hr,
           Row{whatsAWorkspaceLabel, buttonBox}}
        .attachTo(this);

    connect(m_btCreateNew,
            &QAbstractButton::clicked,
            m_workspaceView,
            &WorkspaceView::createNewWorkspace);
    connect(m_btClone, &QAbstractButton::clicked,
            m_workspaceView, &WorkspaceView::cloneCurrentWorkspace);
    connect(m_btDelete, &QAbstractButton::clicked,
            m_workspaceView, &WorkspaceView::deleteSelectedWorkspaces);
    connect(m_btSwitch, &QAbstractButton::clicked,
            m_workspaceView, &WorkspaceView::switchToCurrentWorkspace);
    connect(m_btRename, &QAbstractButton::clicked,
            m_workspaceView, &WorkspaceView::renameCurrentWorkspace);
    connect(m_btReset, &QAbstractButton::clicked,
            m_workspaceView, &WorkspaceView::resetCurrentWorkspace);
    connect(m_workspaceView, &WorkspaceView::workspaceActivated,
            m_workspaceView, &WorkspaceView::switchToCurrentWorkspace);
    connect(m_workspaceView, &WorkspaceView::workspacesSelected,
            this, &WorkspaceDialog::updateActions);
    connect(m_btImport, &QAbstractButton::clicked,
            m_workspaceView, &WorkspaceView::importWorkspace);
    connect(m_btExport,
            &QAbstractButton::clicked,
            m_workspaceView,
            &WorkspaceView::exportCurrentWorkspace);

    connect(m_btUp, &QAbstractButton::clicked, m_workspaceView, &WorkspaceView::moveWorkspaceUp);
    connect(m_btDown, &QAbstractButton::clicked, m_workspaceView, &WorkspaceView::moveWorkspaceDown);

    updateActions(m_workspaceView->selectedWorkspaces());
}

void WorkspaceDialog::setAutoLoadWorkspace(bool check)
{
    m_autoLoadCheckBox->setChecked(check);
}

bool WorkspaceDialog::autoLoadWorkspace() const
{
    return m_autoLoadCheckBox->checkState() == Qt::Checked;
}

DockManager *WorkspaceDialog::dockManager() const
{
    return m_manager;
}

void WorkspaceDialog::updateActions(const QStringList &fileNames)
{
    if (fileNames.isEmpty()) {
        m_btDelete->setEnabled(false);
        m_btRename->setEnabled(false);
        m_btClone->setEnabled(false);
        m_btReset->setEnabled(false);
        m_btSwitch->setEnabled(false);
        m_btUp->setEnabled(false);
        m_btDown->setEnabled(false);
        m_btExport->setEnabled(false);
        return;
    }
    const bool presetIsSelected = Utils::anyOf(fileNames, [this](const QString &fileName) {
        Workspace *workspace = m_manager->workspace(fileName);
        if (!workspace)
            return false;

        return workspace->isPreset();
    });
    const bool activeIsSelected = Utils::anyOf(fileNames, [this](const QString &fileName) {
        Workspace *workspace = m_manager->workspace(fileName);
        if (!workspace)
            return false;

        return *workspace == *m_manager->activeWorkspace();
    });
    const bool canMoveUp = Utils::anyOf(fileNames, [this](const QString &fileName) {
        return m_manager->workspaceIndex(fileName) > 0;
    });
    const int count = m_manager->workspaces().count();
    const bool canMoveDown = Utils::anyOf(fileNames, [this, &count](const QString &fileName) {
        const int i = m_manager->workspaceIndex(fileName);
        return i < (count - 1);
    });

    m_btDelete->setEnabled(!activeIsSelected && !presetIsSelected);
    m_btRename->setEnabled(fileNames.size() == 1 && !presetIsSelected);
    m_btClone->setEnabled(fileNames.size() == 1);
    m_btReset->setEnabled(presetIsSelected);
    m_btSwitch->setEnabled(fileNames.size() == 1 && !activeIsSelected);
    m_btUp->setEnabled(fileNames.size() == 1 && canMoveUp);
    m_btDown->setEnabled(fileNames.size() == 1 && canMoveDown);
    m_btExport->setEnabled(fileNames.size() == 1);
}

} // namespace ADS
