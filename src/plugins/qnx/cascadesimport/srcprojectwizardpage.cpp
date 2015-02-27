/****************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry Limited (blackberry-qt@qnx.com)
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

#include "srcprojectwizardpage.h"
#include "ui_srcprojectwizardpage.h"

#include <utils/wizard.h>

#include <QDir>
#include <QDirIterator>

namespace Qnx {
namespace Internal {

static bool validateProjectPath(const QString &path, QString *errorMessage)
{
    bool proFound = false;
    bool barDescriptorFound = false;
    QDirIterator di(path);
    while (di.hasNext()) {
        di.next();
        QFileInfo fi = di.fileInfo();
        if (fi.isFile()) {
            if (fi.fileName() == QLatin1String("bar-descriptor.xml"))
                barDescriptorFound = true;
            else if (fi.fileName().endsWith(QLatin1String(".pro")))
                proFound = true;
        }
        if (barDescriptorFound && proFound)
            break;
    }
    const bool ret = barDescriptorFound && proFound;
    if (!ret && errorMessage) {
        *errorMessage = QCoreApplication::translate("Qnx",
                "Directory does not seem to be a valid Cascades project.");
    }
    return ret;
}


SrcProjectWizardPage::SrcProjectWizardPage(QWidget *parent)
    : QWizardPage(parent), m_complete(false)
{
    ui = new Ui::SrcProjectWizardPage;
    ui->setupUi(this);
    ui->pathChooser->setPromptDialogTitle(tr("Choose imported Cascades project directory"));
    ui->pathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    ui->pathChooser->setAdditionalPathValidator(validateProjectPath);

    connect(ui->pathChooser, SIGNAL(pathChanged(QString)), this, SLOT(onPathChooserPathChanged(QString)));

    setPath(QDir::homePath());

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Momentics"));
}

SrcProjectWizardPage::~SrcProjectWizardPage()
{
    delete ui;
}

void SrcProjectWizardPage::onPathChooserPathChanged(const QString &newPath)
{
    bool newComplete = ui->pathChooser->isValid();
    if (newComplete != m_complete) {
        m_complete = newComplete;
        emit completeChanged();
    }
    if (newComplete) emit validPathChanged(newPath);
}

QString SrcProjectWizardPage::projectName() const
{
    return path().section(QLatin1Char('/'), -1);
}

QString SrcProjectWizardPage::path() const
{
    return Utils::FileName::fromUserInput(ui->pathChooser->path()).toString();
}

void SrcProjectWizardPage::setPath(const QString &path)
{
    ui->pathChooser->setPath(path);
}

bool SrcProjectWizardPage::isComplete() const
{
    return m_complete;
}

} // namespace Internal
} // namespace Qnx
