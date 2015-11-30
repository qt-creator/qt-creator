/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

AndroidGdbServerKitInformation::AndroidGdbServerKitInformation()
{
    setId(AndroidGdbServerKitInformation::id());
    setPriority(27999); // Just one less than Debugger!
}

QVariant AndroidGdbServerKitInformation::defaultValue(Kit *kit) const
{
    return autoDetect(kit).toString();
}

QList<Task> AndroidGdbServerKitInformation::validate(const Kit *) const
{
    return QList<Task>();
}

KitInformation::ItemList AndroidGdbServerKitInformation::toUserOutput(const Kit *kit) const
{
    return KitInformation::ItemList()
            << qMakePair(tr("GDB server"), AndroidGdbServerKitInformation::gdbServer(kit).toUserOutput());
}

KitConfigWidget *AndroidGdbServerKitInformation::createConfigWidget(Kit *kit) const
{
    return new AndroidGdbServerKitInformationWidget(kit, this);
}

Core::Id AndroidGdbServerKitInformation::id()
{
    return "Android.GdbServer.Information";
}

bool AndroidGdbServerKitInformation::isAndroidKit(const Kit *kit)
{
    QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(kit);
    ToolChain *tc = ToolChainKitInformation::toolChain(kit);
    if (qt && tc)
        return qt->type() == QLatin1String(Constants::ANDROIDQT)
                && tc->typeId() == Constants::ANDROID_TOOLCHAIN_ID;
    return false;

}

FileName AndroidGdbServerKitInformation::gdbServer(const Kit *kit)
{
    return FileName::fromString(kit->value(AndroidGdbServerKitInformation::id()).toString());
}

void AndroidGdbServerKitInformation::setGdbSever(Kit *kit, const FileName &gdbServerCommand)
{
    kit->setValue(AndroidGdbServerKitInformation::id(), gdbServerCommand.toString());
}

FileName AndroidGdbServerKitInformation::autoDetect(Kit *kit)
{
    ToolChain *tc = ToolChainKitInformation::toolChain(kit);
    if (!tc || tc->typeId() != Constants::ANDROID_TOOLCHAIN_ID)
        return FileName();
    AndroidToolChain *atc = static_cast<AndroidToolChain *>(tc);
    return atc->suggestedGdbServer();
}

///////////////
// AndroidGdbServerKitInformationWidget
///////////////


AndroidGdbServerKitInformationWidget::AndroidGdbServerKitInformationWidget(Kit *kit, const KitInformation *ki)
    : KitConfigWidget(kit, ki),
      m_label(new ElidingLabel),
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

AndroidGdbServerKitInformationWidget::~AndroidGdbServerKitInformationWidget()
{
    delete m_button;
    delete m_label;
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
    return DeviceKitInformation::deviceId(m_kit) == Constants::ANDROID_DEVICE_ID;
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
    PathChooser *chooser = new PathChooser;
    chooser->setExpectedKind(PathChooser::ExistingCommand);
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

} // namespace Internal
} // namespace Android
