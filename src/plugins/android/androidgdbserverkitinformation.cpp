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

#include "androidgdbserverkitinformation.h"
#include "androidconstants.h"
#include "androidtoolchain.h"

#include <utils/pathchooser.h>
#include <utils/elidinglabel.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>

using namespace Android;
using namespace Android::Internal;

namespace {
static const char ANDROIDGDBSERVER_INFORMATION[] = "Android.GdbServer.Information";
}

AndroidGdbServerKitInformation::AndroidGdbServerKitInformation()
{
}

Core::Id AndroidGdbServerKitInformation::dataId() const
{
    return Core::Id(ANDROIDGDBSERVER_INFORMATION);
}

unsigned int AndroidGdbServerKitInformation::priority() const
{
    return 27999; // Just one less than Debugger!
}

QVariant AndroidGdbServerKitInformation::defaultValue(ProjectExplorer::Kit *kit) const
{
    return autoDetect(kit).toString();
}

QList<ProjectExplorer::Task> AndroidGdbServerKitInformation::validate(const ProjectExplorer::Kit *) const
{
    return QList<ProjectExplorer::Task>();
}

ProjectExplorer::KitInformation::ItemList AndroidGdbServerKitInformation::toUserOutput(const ProjectExplorer::Kit *kit) const
{
    return ProjectExplorer::KitInformation::ItemList()
            << qMakePair(tr("GDB server"), AndroidGdbServerKitInformation::gdbServer(kit).toUserOutput());
}

ProjectExplorer::KitConfigWidget *AndroidGdbServerKitInformation::createConfigWidget(ProjectExplorer::Kit *kit) const
{
    return new AndroidGdbServerKitInformationWidget(kit, isSticky(kit));
}

Utils::FileName AndroidGdbServerKitInformation::gdbServer(const ProjectExplorer::Kit *kit)
{
    return Utils::FileName::fromString(kit->value(Core::Id(ANDROIDGDBSERVER_INFORMATION)).toString());
}

void AndroidGdbServerKitInformation::setGdbSever(ProjectExplorer::Kit *kit, const Utils::FileName &gdbServerCommand)
{
    kit->setValue(Core::Id(ANDROIDGDBSERVER_INFORMATION),
                  gdbServerCommand.toString());
}

Utils::FileName AndroidGdbServerKitInformation::autoDetect(ProjectExplorer::Kit *kit)
{
    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainKitInformation::toolChain(kit);
    if (!tc || tc->type() != QLatin1String(Constants::ANDROID_TOOLCHAIN_TYPE))
        return Utils::FileName();
    AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);
    return atc->suggestedGdbServer();
}

///////////////
// AndroidGdbServerKitInformationWidget
///////////////


AndroidGdbServerKitInformationWidget::AndroidGdbServerKitInformationWidget(ProjectExplorer::Kit *kit, bool sticky)
    : ProjectExplorer::KitConfigWidget(kit, sticky),
      m_label(new Utils::ElidingLabel),
      m_button(new QPushButton(tr("Manage...")))
{
    // ToolButton with Menu, defaulting to 'Autodetect'.
    QMenu *buttonMenu = new QMenu(m_button);
    QAction *autoDetectAction = buttonMenu->addAction(tr("Auto-detect"));
    connect(autoDetectAction, SIGNAL(triggered()), this, SLOT(autoDetectDebugger()));
    QAction *changeAction = buttonMenu->addAction(tr("Edit..."));
    connect(changeAction, SIGNAL(triggered()), this, SLOT(showDialog()));
    m_button->setMenu(buttonMenu);

    refresh();
}

QString AndroidGdbServerKitInformationWidget::displayName() const
{
    return tr("Android GDB server");
}

QString AndroidGdbServerKitInformationWidget::toolTip() const
{
    return tr("The GDB server to use for this kit.");
}

void AndroidGdbServerKitInformationWidget::makeReadOnly()
{
    m_button->setEnabled(false);
}

void AndroidGdbServerKitInformationWidget::refresh()
{
    m_label->setText(AndroidGdbServerKitInformation::gdbServer(m_kit).toString());
}

bool AndroidGdbServerKitInformationWidget::visibleInKit()
{
    return ProjectExplorer::DeviceKitInformation::deviceId(m_kit) == Core::Id(Constants::ANDROID_DEVICE_ID);
}

QWidget *AndroidGdbServerKitInformationWidget::mainWidget() const
{
    return m_label;
}

QWidget *AndroidGdbServerKitInformationWidget::buttonWidget() const
{
    return m_button;
}

void AndroidGdbServerKitInformationWidget::autoDetectDebugger()
{
    AndroidGdbServerKitInformation::setGdbSever(m_kit, AndroidGdbServerKitInformation::autoDetect(m_kit));
}

void AndroidGdbServerKitInformationWidget::showDialog()
{
    QDialog dialog;
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    QLabel *binaryLabel = new QLabel(tr("&Binary:"));
    Utils::PathChooser *chooser = new Utils::PathChooser;
    chooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    chooser->setPath(AndroidGdbServerKitInformation::gdbServer(m_kit).toString());
    binaryLabel->setBuddy(chooser);
    formLayout->addRow(binaryLabel, chooser);
    layout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    layout->addWidget(buttonBox);

    dialog.setWindowTitle(tr("GDB Server for \"%1\"").arg(m_kit->displayName()));

    if (dialog.exec() == QDialog::Accepted)
        AndroidGdbServerKitInformation::setGdbSever(m_kit, chooser->fileName());
}
