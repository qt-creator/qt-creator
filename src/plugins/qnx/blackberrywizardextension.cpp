/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrywizardextension.h"

#include "bardescriptorfileimagewizardpage.h"
#include "qnxconstants.h"

#include <coreplugin/dialogs/iwizard.h>
#include <coreplugin/generatedfile.h>

#include <QFileInfo>
#include <QDomDocument>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryWizardExtension::BlackBerryWizardExtension()
    : m_imageWizardPage(0)
{
}

QList<QWizardPage *> BlackBerryWizardExtension::extensionPages(const Core::IWizard *wizard)
{
    QStringList validIds;
    validIds << QLatin1String(Constants::QNX_BLACKBERRY_CASCADES_WIZARD_ID);
    validIds << QLatin1String(Constants::QNX_BAR_DESCRIPTOR_WIZARD_ID);
    validIds << QLatin1String(Constants::QNX_BLACKBERRY_QTQUICK_APP_WIZARD_ID);
    validIds << QLatin1String(Constants::QNX_BLACKBERRY_GUI_APP_WIZARD_ID);
    validIds << QLatin1String(Constants::QNX_BLACKBERRY_QTQUICK2_APP_WIZARD_ID);

    QList<QWizardPage*> pages;

    if (!validIds.contains(wizard->id()))
        return pages;

    m_imageWizardPage = new BarDescriptorFileImageWizardPage;

    pages << m_imageWizardPage;

    return pages;
}

bool BlackBerryWizardExtension::processFiles(const QList<Core::GeneratedFile> &files, bool *removeOpenProjectAttribute, QString *errorMessage)
{
    Q_UNUSED(files);
    Q_UNUSED(removeOpenProjectAttribute);
    Q_UNUSED(errorMessage);

    return true;
}

void BlackBerryWizardExtension::applyCodeStyle(Core::GeneratedFile *file) const
{
    QFileInfo fi(file->path());
    if (fi.fileName() == QLatin1String("bar-descriptor.xml"))
        addImagesToBarDescriptor(file);
}

void BlackBerryWizardExtension::addImagesToBarDescriptor(Core::GeneratedFile *file) const
{
    QDomDocument doc;
    doc.setContent(file->contents());

    QDomElement docElem = doc.documentElement();

    // Add asset elements
    QStringList fileAssets;
    fileAssets << m_imageWizardPage->icon();
    fileAssets << m_imageWizardPage->landscapeSplashScreen();
    fileAssets << m_imageWizardPage->portraitSplashScreen();
    Q_FOREACH (const QString &asset, fileAssets) {
        if (asset.isEmpty())
            continue;

        QDomElement assetElem = doc.createElement(QLatin1String("asset"));
        assetElem.setAttribute(QLatin1String("path"), asset);

        QDomText fileNameText = doc.createTextNode(QFileInfo(asset).fileName());
        assetElem.appendChild(fileNameText);

        docElem.appendChild(assetElem);
    }

    // Add icon element
    if (!m_imageWizardPage->icon().isEmpty()) {
        QDomElement iconElem = doc.createElement(QLatin1String("icon"));
        QDomElement imageElem = doc.createElement(QLatin1String("image"));
        QDomText fileNameText = doc.createTextNode(QFileInfo(m_imageWizardPage->icon()).fileName());

        imageElem.appendChild(fileNameText);
        iconElem.appendChild(imageElem);
        docElem.appendChild(iconElem);
    }

    // Add splashscreen element
    QString splashScreenEntry = QFileInfo(m_imageWizardPage->landscapeSplashScreen()).fileName();
    if (!m_imageWizardPage->portraitSplashScreen().isEmpty())
        splashScreenEntry.append(QLatin1Char(':') + QFileInfo(m_imageWizardPage->portraitSplashScreen()).fileName());

    if (!splashScreenEntry.isEmpty()) {
        QDomElement splashScreenElem = doc.createElement(QLatin1String("splashscreen"));
        QDomText splashScreenText = doc.createTextNode(splashScreenEntry);

        splashScreenElem.appendChild(splashScreenText);
        docElem.appendChild(splashScreenElem);
    }

    file->setContents(doc.toString(4));
}
