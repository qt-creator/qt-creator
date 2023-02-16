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
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QValidator>

namespace ADS {

class WorkspaceValidator : public QValidator
{
public:
    WorkspaceValidator(QObject *parent, const QStringList &workspaces);
    void fixup(QString &input) const override;
    QValidator::State validate(QString &input, int &pos) const override;

private:
    QStringList m_workspaces;
};

WorkspaceValidator::WorkspaceValidator(QObject *parent, const QStringList &workspaces)
    : QValidator(parent)
    , m_workspaces(workspaces)
{}

QValidator::State WorkspaceValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos)

    static const QRegularExpression rx("^[a-zA-Z0-9 ()\\-]*$");

    if (!rx.match(input).hasMatch())
        return QValidator::Invalid;

    if (m_workspaces.contains(input))
        return QValidator::Intermediate;
    else
        return QValidator::Acceptable;
}

void WorkspaceValidator::fixup(QString &input) const
{
    int i = 2;
    QString copy;
    do {
        copy = input + QLatin1String(" (") + QString::number(i) + QLatin1Char(')');
        ++i;
    } while (m_workspaces.contains(copy));
    input = copy;
}

WorkspaceNameInputDialog::WorkspaceNameInputDialog(DockManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    auto label = new QLabel(Tr::tr("Enter the name of the workspace:"), this);
    m_newWorkspaceLineEdit = new QLineEdit(this);
    m_newWorkspaceLineEdit->setValidator(new WorkspaceValidator(this, m_manager->workspaces()));
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                        Qt::Horizontal,
                                        this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_switchToButton = new QPushButton;
    buttons->addButton(m_switchToButton, QDialogButtonBox::AcceptRole);
    connect(m_switchToButton, &QPushButton::clicked, this, [this] { m_usedSwitchTo = true; });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    using namespace Utils::Layouting;

    Column {
        label,
        m_newWorkspaceLineEdit,
        buttons
    }.attachTo(this);
}

void WorkspaceNameInputDialog::setActionText(const QString &actionText,
                                             const QString &openActionText)
{
    m_okButton->setText(actionText);
    m_switchToButton->setText(openActionText);
}

void WorkspaceNameInputDialog::setValue(const QString &value)
{
    m_newWorkspaceLineEdit->setText(value);
}

QString WorkspaceNameInputDialog::value() const
{
    return m_newWorkspaceLineEdit->text();
}

bool WorkspaceNameInputDialog::isSwitchToRequested() const
{
    return m_usedSwitchTo;
}

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
    , m_autoLoadCheckBox(new QCheckBox(Tr::tr("Restore last workspace on startup")))
{
    setWindowTitle(Tr::tr("Workspace Manager"));

    m_workspaceView->setActivationMode(Utils::DoubleClickActivation);

    QLabel *whatsAWorkspaceLabel = new QLabel(Tr::tr("<a href=\"qthelp://org.qt-project.qtcreator/doc/"
           "creator-project-managing-workspaces.html\">What is a Workspace?</a>"));
    whatsAWorkspaceLabel->setOpenExternalLinks(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    using namespace Utils::Layouting;

    Column {
        Row {
            Column {
                m_workspaceView,
                m_autoLoadCheckBox
            },
            Column {
                m_btCreateNew,
                m_btRename,
                m_btClone,
                m_btDelete,
                m_btReset,
                m_btSwitch,
                st,
                m_btImport,
                m_btExport
            }
        },
        hr,
        Row {
            whatsAWorkspaceLabel,
            buttonBox
        }
    }.attachTo(this);


    connect(m_btCreateNew, &QAbstractButton::clicked,
            m_workspaceView, &WorkspaceView::createNewWorkspace);
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
    connect(m_btExport, &QAbstractButton::clicked,
            m_workspaceView, &WorkspaceView::exportCurrentWorkspace);

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

void WorkspaceDialog::updateActions(const QStringList &workspaces)
{
    if (workspaces.isEmpty()) {
        m_btDelete->setEnabled(false);
        m_btRename->setEnabled(false);
        m_btClone->setEnabled(false);
        m_btReset->setEnabled(false);
        m_btSwitch->setEnabled(false);
        m_btExport->setEnabled(false);
        return;
    }
    const bool presetIsSelected = Utils::anyOf(workspaces, [this](const QString &workspace) {
        return m_manager->isWorkspacePreset(workspace);
    });
    const bool activeIsSelected = Utils::anyOf(workspaces, [this](const QString &workspace) {
        return workspace == m_manager->activeWorkspace();
    });
    m_btDelete->setEnabled(!activeIsSelected && !presetIsSelected);
    m_btRename->setEnabled(workspaces.size() == 1 && !presetIsSelected);
    m_btClone->setEnabled(workspaces.size() == 1);
    m_btReset->setEnabled(presetIsSelected);
    m_btSwitch->setEnabled(workspaces.size() == 1);
    m_btExport->setEnabled(workspaces.size() == 1);
}

} // namespace ADS
