/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "bardescriptorfileimagewizardpage.h"
#include "ui_bardescriptorfileimagewizardpage.h"

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorFileImageWizardPage::BarDescriptorFileImageWizardPage(QWidget *parent)
    : QWizardPage(parent)
    , m_ui(new Ui::BarDescriptorFileImageWizardPage)
    , m_iconValidationResult(Valid)
    , m_landscapeSplashScreenValidationResult(Valid)
    , m_portraitSplashScreenValidationResult(Valid)
{
    m_ui->setupUi(this);

    setTitle(tr("Images"));

    const QString dialogFilter = tr("Images (*.jpg *.png)");

    m_ui->icon->setExpectedKind(Utils::PathChooser::File);
    m_ui->icon->setPromptDialogFilter(dialogFilter);
    connect(m_ui->icon, SIGNAL(changed(QString)), this, SLOT(validateIcon(QString)));

    m_ui->landscapeSplashScreen->setExpectedKind(Utils::PathChooser::File);
    m_ui->landscapeSplashScreen->setPromptDialogFilter(dialogFilter);
    connect(m_ui->landscapeSplashScreen, SIGNAL(changed(QString)), this, SLOT(validateLandscapeSplashScreen(QString)));

    m_ui->portraitSplashScreen->setExpectedKind(Utils::PathChooser::File);
    m_ui->portraitSplashScreen->setPromptDialogFilter(dialogFilter);
    connect(m_ui->portraitSplashScreen, SIGNAL(changed(QString)), this, SLOT(validatePortraitSplashScreen(QString)));
}

BarDescriptorFileImageWizardPage::~BarDescriptorFileImageWizardPage()
{
    delete m_ui;
}

bool BarDescriptorFileImageWizardPage::isComplete() const
{
    return m_iconValidationResult == Valid
            && m_landscapeSplashScreenValidationResult == Valid
            && m_portraitSplashScreenValidationResult == Valid;
}

QString BarDescriptorFileImageWizardPage::icon() const
{
    return m_ui->icon->path();
}

QString BarDescriptorFileImageWizardPage::landscapeSplashScreen() const
{
    return m_ui->landscapeSplashScreen->path();
}

QString BarDescriptorFileImageWizardPage::portraitSplashScreen() const
{
    return m_ui->portraitSplashScreen->path();
}

void BarDescriptorFileImageWizardPage::validateIcon(const QString &path)
{
    m_iconValidationResult = validateImage(path, QSize(1, 1), QSize(90, 90));

    switch (m_iconValidationResult) {
    case Valid:
        m_ui->iconValidationLabel->clear();
        break;
    case CouldNotLoad:
        m_ui->iconValidationLabel->setText(tr("<font color=\"red\">Could not open '%1' for reading.</font>").arg(path));
        break;
    case IncorrectSize: {
        const QSize size = imageSize(path);
        m_ui->iconValidationLabel->setText(tr("<font color=\"red\">Incorrect icon size (%1x%2). The recommended size is "
                                              "86x86 pixels with a maximum size of 90x90 pixels.</font>").arg(size.width()).arg(size.height()));
        break;
    }
    default:
        break;
    }

    emit completeChanged();
}

void BarDescriptorFileImageWizardPage::validateLandscapeSplashScreen(const QString &path)
{
    m_landscapeSplashScreenValidationResult = validateImage(path, QSize(1024, 600), QSize(1024, 600));
    updateSplashScreenValidationLabel();
    emit completeChanged();
}

void BarDescriptorFileImageWizardPage::validatePortraitSplashScreen(const QString &path)
{
    m_portraitSplashScreenValidationResult = validateImage(path, QSize(600, 1024), QSize(600, 1024));
    updateSplashScreenValidationLabel();
    emit completeChanged();
}

void BarDescriptorFileImageWizardPage::updateSplashScreenValidationLabel()
{
    if (m_landscapeSplashScreenValidationResult == Valid
            && m_portraitSplashScreenValidationResult == Valid) {
        m_ui->splashScreenValidationLabel->clear();
        return;
    }

    switch (m_landscapeSplashScreenValidationResult) {
    case CouldNotLoad:
        m_ui->splashScreenValidationLabel->setText(tr("<font color=\"red\">Could not open '%1' for reading.</font>")
                                                   .arg(m_ui->landscapeSplashScreen->fileName().toString()));
        break;
    case IncorrectSize: {
        const QSize size = imageSize(m_ui->landscapeSplashScreen->fileName().toString());
        m_ui->splashScreenValidationLabel->setText(tr("<font color=\"red\">Incorrect landscape splash screen size (%1x%2). The required "
                                                      "size is 1024x600 pixels.</font>").arg(size.width()).arg(size.height()));
        break;
    }
    case Valid:
    default:
        break;
    }

    switch (m_portraitSplashScreenValidationResult) {
    case CouldNotLoad:
        m_ui->splashScreenValidationLabel->setText(tr("<font color=\"red\">Could not open '%1' for reading.</font>")
                                                   .arg(m_ui->portraitSplashScreen->fileName().toString()));
        break;
    case IncorrectSize: {
        const QSize size = imageSize(m_ui->portraitSplashScreen->fileName().toString());
        m_ui->splashScreenValidationLabel->setText(tr("<font color=\"red\">Incorrect portrait splash screen size (%1x%2). The required "
                                                      "size is 600x1024 pixels.</font>").arg(size.width()).arg(size.height()));
        break;
    }
    case Valid:
    default:
        break;
    }
}

BarDescriptorFileImageWizardPage::ImageValidationResult BarDescriptorFileImageWizardPage::validateImage(const QString &path, const QSize &minimumSize, const QSize &maximumSize)
{
    if (path.isEmpty())
        return Valid; // Empty is ok, since <icon> and <splashscreen> entries are optional

    QImage img(path);
    if (img.isNull())
        return CouldNotLoad;

    const QSize imgSize = img.size();
    if (imgSize.width() < minimumSize.width() || imgSize.height() < minimumSize.height()
            || imgSize.width() > maximumSize.width() || imgSize.height() > maximumSize.height())
        return IncorrectSize;

    return Valid;
}

QSize BarDescriptorFileImageWizardPage::imageSize(const QString &path)
{
    QImage img(path);
    if (img.isNull())
        return QSize();

    return img.size();
}

