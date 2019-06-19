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

#include "androidgdbserverkitinformation.h"
#include "androidconstants.h"
#include "androidtoolchain.h"
#include "androidconfigurations.h"

#include <utils/pathchooser.h>
#include <utils/elidinglabel.h>
#include <utils/qtcassert.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QDialog>
#include <QVBoxLayout>
#include <QFormLayout>

#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

class AndroidGdbServerKitAspectWidget : public KitAspectWidget
{
    Q_DECLARE_TR_FUNCTIONS(Android::Internal::AndroidGdbServerKitAspect)
public:
    AndroidGdbServerKitAspectWidget(Kit *kit, const KitAspect *ki);
    ~AndroidGdbServerKitAspectWidget() override;

    void makeReadOnly() override;
    void refresh() override;

    QWidget *mainWidget() const override;
    QWidget *buttonWidget() const override;

private:
    void autoDetectDebugger();
    void showDialog();

    QLabel *m_label;
    QPushButton *m_button;
};


AndroidGdbServerKitAspect::AndroidGdbServerKitAspect()
{
    setId(AndroidGdbServerKitAspect::id());
    setDisplayName(tr("Android GDB server"));
    setDescription(tr("The GDB server to use for this kit."));
    setPriority(27999); // Just one less than Debugger!
}

void AndroidGdbServerKitAspect::setup(Kit *kit)
{
    if (kit && !kit->hasValue(id()))
        kit->setValue(id(), autoDetect(kit).toString());
}

Tasks AndroidGdbServerKitAspect::validate(const Kit *) const
{
    return {};
}

bool AndroidGdbServerKitAspect::isApplicableToKit(const Kit *k) const
{
    return DeviceKitAspect::deviceId(k) == Constants::ANDROID_DEVICE_ID;
}

KitAspect::ItemList AndroidGdbServerKitAspect::toUserOutput(const Kit *kit) const
{
    return {{tr("GDB server"), AndroidGdbServerKitAspect::gdbServer(kit).toUserOutput()}};
}

KitAspectWidget *AndroidGdbServerKitAspect::createConfigWidget(Kit *kit) const
{
    QTC_ASSERT(kit, return nullptr);
    return new AndroidGdbServerKitAspectWidget(kit, this);
}

Core::Id AndroidGdbServerKitAspect::id()
{
    return "Android.GdbServer.Information";
}

FilePath AndroidGdbServerKitAspect::gdbServer(const Kit *kit)
{
    QTC_ASSERT(kit, return FilePath());
    return FilePath::fromString(kit->value(AndroidGdbServerKitAspect::id()).toString());
}

void AndroidGdbServerKitAspect::setGdbSever(Kit *kit, const FilePath &gdbServerCommand)
{
    QTC_ASSERT(kit, return);
    kit->setValue(AndroidGdbServerKitAspect::id(), gdbServerCommand.toString());
}

FilePath AndroidGdbServerKitAspect::autoDetect(const Kit *kit)
{
    ToolChain *tc = ToolChainKitAspect::toolChain(kit, ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!tc || tc->typeId() != Constants::ANDROID_TOOLCHAIN_TYPEID)
        return FilePath();
    return AndroidConfigurations::currentConfig().gdbServer(tc->targetAbi());
}

///////////////
// AndroidGdbServerKitAspectWidget
///////////////


AndroidGdbServerKitAspectWidget::AndroidGdbServerKitAspectWidget(Kit *kit, const KitAspect *ki) :
    KitAspectWidget(kit, ki),
    m_label(new ElidingLabel),
    m_button(new QPushButton(tr("Manage...")))
{
    // ToolButton with Menu, defaulting to 'Autodetect'.
    auto buttonMenu = new QMenu(m_button);
    QAction *autoDetectAction = buttonMenu->addAction(tr("Auto-detect"));
    connect(autoDetectAction, &QAction::triggered,
            this, &AndroidGdbServerKitAspectWidget::autoDetectDebugger);
    QAction *changeAction = buttonMenu->addAction(tr("Edit..."));
    connect(changeAction, &QAction::triggered,
            this, &AndroidGdbServerKitAspectWidget::showDialog);
    m_button->setMenu(buttonMenu);

    refresh();
}

AndroidGdbServerKitAspectWidget::~AndroidGdbServerKitAspectWidget()
{
    delete m_button;
    delete m_label;
}

void AndroidGdbServerKitAspectWidget::makeReadOnly()
{
    m_button->setEnabled(false);
}

void AndroidGdbServerKitAspectWidget::refresh()
{
    m_label->setText(AndroidGdbServerKitAspect::gdbServer(m_kit).toString());
}

QWidget *AndroidGdbServerKitAspectWidget::mainWidget() const
{
    return m_label;
}

QWidget *AndroidGdbServerKitAspectWidget::buttonWidget() const
{
    return m_button;
}

void AndroidGdbServerKitAspectWidget::autoDetectDebugger()
{
    AndroidGdbServerKitAspect::setGdbSever(m_kit, AndroidGdbServerKitAspect::autoDetect(m_kit));
}

void AndroidGdbServerKitAspectWidget::showDialog()
{
    QDialog dialog;
    auto layout = new QVBoxLayout(&dialog);
    auto formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    auto binaryLabel = new QLabel(tr("&Binary:"));
    auto chooser = new PathChooser;
    chooser->setExpectedKind(PathChooser::ExistingCommand);
    chooser->setPath(AndroidGdbServerKitAspect::gdbServer(m_kit).toString());
    binaryLabel->setBuddy(chooser);
    formLayout->addRow(binaryLabel, chooser);
    layout->addLayout(formLayout);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.setWindowTitle(tr("GDB Server for \"%1\"").arg(m_kit->displayName()));

    if (dialog.exec() == QDialog::Accepted)
        AndroidGdbServerKitAspect::setGdbSever(m_kit, chooser->fileName());
}

} // namespace Internal
} // namespace Android
