/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "debuggerkitconfigwidget.h"
#include "debuggerkitinformation.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/kitinformation.h>

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

#include <QUrl>

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>

namespace Debugger {
namespace Internal {


static const char dgbToolsDownloadLink32C[] = "http://www.microsoft.com/whdc/devtools/debugging/installx86.Mspx";
static const char dgbToolsDownloadLink64C[] = "http://www.microsoft.com/whdc/devtools/debugging/install64bit.Mspx";

// -----------------------------------------------------------------------
// DebuggerKitConfigWidget:
// -----------------------------------------------------------------------

DebuggerKitConfigWidget::DebuggerKitConfigWidget(ProjectExplorer::Kit *k,
                                                 const DebuggerKitInformation *ki,
                                                 QWidget *parent) :
    ProjectExplorer::KitConfigWidget(parent),
    m_kit(k),
    m_info(ki),
    m_comboBox(new QComboBox(this)),
    m_label(new QLabel(this)),
    m_chooser(new Utils::PathChooser(this))
{
    setToolTip(tr("The debugger to use for this kit."));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    m_comboBox->addItem(DebuggerKitInformation::debuggerEngineName(GdbEngineType), QVariant(int(GdbEngineType)));
    if (ProjectExplorer::Abi::hostAbi().os() == ProjectExplorer::Abi::WindowsOS) {
        m_comboBox->addItem(DebuggerKitInformation::debuggerEngineName(CdbEngineType), QVariant(int(CdbEngineType)));
    } else {
        m_comboBox->addItem(DebuggerKitInformation::debuggerEngineName(LldbEngineType), QVariant(int(LldbEngineType)));
    }
    layout->addWidget(m_comboBox);

    m_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_label->setOpenExternalLinks(true);
    layout->addWidget(m_label);

    m_chooser->setContentsMargins(0, 0, 0, 0);
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_chooser->insertButton(0, tr("Auto detect"), this, SLOT(autoDetectDebugger()));

    layout->addWidget(m_chooser);

    discard();
    connect(m_chooser, SIGNAL(changed(QString)), this, SIGNAL(dirty()));
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dirty()));
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshLabel()));
}

QString DebuggerKitConfigWidget::displayName() const
{
    return tr("Debugger:");
}

void DebuggerKitConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
    m_chooser->setEnabled(false);
}

void DebuggerKitConfigWidget::apply()
{
    DebuggerKitInformation::setDebuggerItem(m_kit, DebuggerKitInformation::DebuggerItem(engineType(), fileName()));
}

void DebuggerKitConfigWidget::discard()
{
    const DebuggerKitInformation::DebuggerItem item = DebuggerKitInformation::debuggerItem(m_kit);
    setEngineType(item.engineType);
    setFileName(item.binary);
}

bool DebuggerKitConfigWidget::isDirty() const
{
    const DebuggerKitInformation::DebuggerItem item = DebuggerKitInformation::debuggerItem(m_kit);
    return item.engineType != engineType() || item.binary != fileName();
}

QWidget *DebuggerKitConfigWidget::buttonWidget() const
{
    return m_chooser->buttonAtIndex(1);
}

void DebuggerKitConfigWidget::autoDetectDebugger()
{
    const DebuggerKitInformation::DebuggerItem item = DebuggerKitInformation::autoDetectItem(m_kit);
    setEngineType(item.engineType);
    setFileName(item.binary);
}

DebuggerEngineType DebuggerKitConfigWidget::engineType() const
{
    const int index = m_comboBox->currentIndex();
    return static_cast<DebuggerEngineType>(m_comboBox->itemData(index).toInt());
}

void DebuggerKitConfigWidget::setEngineType(DebuggerEngineType et)
{
    const int size = m_comboBox->count();
    for (int i = 0; i < size; ++i) {
        if (m_comboBox->itemData(i).toInt() == et) {
            m_comboBox->setCurrentIndex(i);
            refreshLabel();
            break;
        }
    }
}

Utils::FileName DebuggerKitConfigWidget::fileName() const
{
    return m_chooser->fileName();
}

void DebuggerKitConfigWidget::setFileName(const Utils::FileName &fn)
{
    m_chooser->setFileName(fn);
}

void DebuggerKitConfigWidget::refreshLabel()
{
    QString text;
    switch (engineType()) {
    case CdbEngineType: {
#ifdef Q_OS_WIN
        const bool is64bit = Utils::winIs64BitSystem();
#else
        const bool is64bit = false;
#endif
        const QString link = is64bit ? QLatin1String(dgbToolsDownloadLink64C) : QLatin1String(dgbToolsDownloadLink32C);
        const QString versionString = is64bit ? tr("64-bit version") : tr("32-bit version");
        //: Label text for path configuration. %2 is "x-bit version".
        text = tr("<html><body><p>Specify the path to the "
                  "<a href=\"%1\">Windows Console Debugger executable</a>"
                  " (%2) here.</p>""</body></html>").arg(link, versionString);
    }
        break;
    default:
        break;
    }
    m_label->setText(text);
    m_label->setVisible(!text.isEmpty());
}

} // namespace Internal
} // namespace Debugger
