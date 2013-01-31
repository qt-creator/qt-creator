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
#include "maemosettingspages.h"

#include "maemoqemusettings.h"
#include "maemoqemusettingswidget.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QIcon>
#include <QMainWindow>

namespace Madde {
namespace Internal {
namespace {

class MaemoQemuCrashDialog : public QDialog
{
    Q_OBJECT
public:
    MaemoQemuCrashDialog(QWidget *parent = 0) : QDialog(parent)
    {
        setWindowTitle(tr("Qemu error"));
        QString message = tr("Qemu crashed.") + QLatin1String(" <p>");
        const MaemoQemuSettings::OpenGlMode openGlMode
            = MaemoQemuSettings::openGlMode();
        const QString linkString = QLatin1String("</p><a href=\"dummy\">")
            + tr("Click here to change the OpenGL mode.")
            + QLatin1String("</a>");
        if (openGlMode == MaemoQemuSettings::HardwareAcceleration) {
            message += tr("You have configured Qemu to use OpenGL "
                "hardware acceleration, which might not be supported by "
                "your system. You could try using software rendering instead.");
            message += linkString;
        } else if (openGlMode == MaemoQemuSettings::AutoDetect) {
            message += tr("Qemu is currently configured to auto-detect the "
                "OpenGL mode, which is known to not work in some cases. "
                "You might want to use software rendering instead.");
            message += linkString;
        }
        QLabel * const messageLabel = new QLabel(message, this);
        messageLabel->setWordWrap(true);
        messageLabel->setTextFormat(Qt::RichText);
        connect(messageLabel, SIGNAL(linkActivated(QString)),
            SLOT(showSettingsPage()));
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(messageLabel);
        QFrame * const separator = new QFrame;
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        mainLayout->addWidget(separator);
        QDialogButtonBox * const buttonBox = new QDialogButtonBox;
        buttonBox->addButton(QDialogButtonBox::Ok);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        mainLayout->addWidget(buttonBox);
    }

private:
    Q_SLOT void showSettingsPage()
    {
        Core::ICore::showOptionsDialog(MaemoQemuSettingsPage::pageCategory(),
            MaemoQemuSettingsPage::pageId());
        accept();
    }
};

} // anonymous namespace


MaemoQemuSettingsPage::MaemoQemuSettingsPage(QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId(pageId());
    setDisplayName(tr("MeeGo Qemu Settings"));
    setCategory(pageCategory());
    //setDisplayCategory(QString()); // Will be set by device configurations page.
    //setCategoryIcon(QIcon()) // See above.
}

bool MaemoQemuSettingsPage::matches(const QString &searchKeyWord) const
{
    return m_widget->keywords().contains(searchKeyWord, Qt::CaseInsensitive);
}

QWidget *MaemoQemuSettingsPage::createPage(QWidget *parent)
{
    m_widget = new MaemoQemuSettingsWidget(parent);
    return m_widget;
}

void MaemoQemuSettingsPage::apply()
{
    m_widget->saveSettings();
}

void MaemoQemuSettingsPage::finish()
{
}

void MaemoQemuSettingsPage::showQemuCrashDialog()
{
    MaemoQemuCrashDialog dlg(Core::ICore::mainWindow());
    dlg.exec();
}

Core::Id MaemoQemuSettingsPage::pageId()
{
    return "ZZ.Qemu Settings";
}

Core::Id MaemoQemuSettingsPage::pageCategory()
{
    return ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY;
}

} // namespace Internal
} // namespace Madde

#include "maemosettingspages.moc"
