/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "debuggerprofileconfigwidget.h"

#include "debuggerprofileinformation.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/profileinformation.h>

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

namespace Debugger {
namespace Internal {


static const char dgbToolsDownloadLink32C[] = "http://www.microsoft.com/whdc/devtools/debugging/installx86.Mspx";
static const char dgbToolsDownloadLink64C[] = "http://www.microsoft.com/whdc/devtools/debugging/install64bit.Mspx";
// -----------------------------------------------------------------------
// DebuggerProfileConfigWidget:
// -----------------------------------------------------------------------

DebuggerProfileConfigWidget::DebuggerProfileConfigWidget(ProjectExplorer::Profile *p,
                                                         const DebuggerProfileInformation *pi,
                                                         QWidget *parent) :
    ProjectExplorer::ProfileConfigWidget(parent),
    m_profile(p),
    m_info(pi),
    m_chooser(new Utils::PathChooser)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(p);
    if (tc && tc->targetAbi().os() == ProjectExplorer::Abi::WindowsOS
            && tc->targetAbi().osFlavor() != ProjectExplorer::Abi::WindowsMSysFlavor) {
        QLabel *msvcDebuggerConfigLabel = new QLabel;
#ifdef Q_OS_WIN
        const bool is64bit = Utils::winIs64BitSystem();
#else
        const bool is64bit = false;
#endif
        const QString link = is64bit ? QLatin1String(dgbToolsDownloadLink64C) : QLatin1String(dgbToolsDownloadLink32C);
        //: Label text for path configuration. %2 is "x-bit version".
        msvcDebuggerConfigLabel->setText(tr("<html><body><p>Specify the path to the "
                                            "<a href=\"%1\">Windows Console Debugger executable</a>"
                                            " (%2) here.</p>"
                                            "</body></html>").arg(link, (is64bit ? tr("64-bit version")
                                                                                 : tr("32-bit version"))));
        msvcDebuggerConfigLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        msvcDebuggerConfigLabel->setOpenExternalLinks(true);
        layout->addWidget(msvcDebuggerConfigLabel);

    }

    m_chooser->setContentsMargins(0, 0, 0, 0);
    m_chooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_chooser->insertButton(0, tr("Auto detect"), this, SLOT(autoDetectDebugger()));

    layout->addWidget(m_chooser);

    discard();
    connect(m_chooser, SIGNAL(changed(QString)), this, SIGNAL(dirty()));
}

QString DebuggerProfileConfigWidget::displayName() const
{
    return tr("Debugger command:");
}

void DebuggerProfileConfigWidget::makeReadOnly()
{
    m_chooser->setEnabled(false);
}

void DebuggerProfileConfigWidget::apply()
{
    Utils::FileName fn = m_chooser->fileName();
    DebuggerProfileInformation::setDebuggerCommand(m_profile, fn);
}

void DebuggerProfileConfigWidget::discard()
{
    m_chooser->setFileName(DebuggerProfileInformation::debuggerCommand(m_profile));
}

bool DebuggerProfileConfigWidget::isDirty() const
{
    return m_chooser->fileName() != DebuggerProfileInformation::debuggerCommand(m_profile);
}

QWidget *DebuggerProfileConfigWidget::buttonWidget() const
{
    return m_chooser->buttonAtIndex(1);
}

void DebuggerProfileConfigWidget::autoDetectDebugger()
{
    QVariant v = m_info->defaultValue(m_profile);
    m_chooser->setFileName(Utils::FileName::fromString(v.toString()));
}

} // namespace Internal

} // namespace Debugger
