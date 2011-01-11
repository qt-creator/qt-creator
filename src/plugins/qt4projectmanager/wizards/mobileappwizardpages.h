/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef MOBILEAPPWIZARDPAGES_H
#define MOBILEAPPWIZARDPAGES_H

#include "abstractmobileapp.h"

#include <QtGui/QWizardPage>

namespace Qt4ProjectManager {
namespace Internal {

class MobileAppWizardGenericOptionsPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(MobileAppWizardGenericOptionsPage)

public:
    explicit MobileAppWizardGenericOptionsPage(QWidget *parent = 0);
    virtual ~MobileAppWizardGenericOptionsPage();

    void setOrientation(AbstractMobileApp::ScreenOrientation orientation);
    AbstractMobileApp::ScreenOrientation orientation() const;

private:
    class MobileAppWizardGenericOptionsPagePrivate *m_d;
};

class MobileAppWizardSymbianOptionsPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(MobileAppWizardSymbianOptionsPage)

public:
    explicit MobileAppWizardSymbianOptionsPage(QWidget *parent = 0);
    virtual ~MobileAppWizardSymbianOptionsPage();

    QString svgIcon() const;
    void setSvgIcon(const QString &icon);
    QString symbianUid() const;
    void setNetworkEnabled(bool enableIt);
    bool networkEnabled() const;
    void setSymbianUid(const QString &uid);

private slots:
    void openSvgIcon(); // Via file open dialog

private:
    class MobileAppWizardSymbianOptionsPagePrivate *m_d;
};

class MobileAppWizardMaemoOptionsPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(MobileAppWizardMaemoOptionsPage)

public:
    explicit MobileAppWizardMaemoOptionsPage(QWidget *parent = 0);
    virtual ~MobileAppWizardMaemoOptionsPage();

    QString pngIcon() const;
    void setPngIcon(const QString &icon);

private slots:
    void openPngIcon();

private:
    class MobileAppWizardMaemoOptionsPagePrivate *m_d;
};

} // end of namespace Internal
} // end of namespace Qt4ProjectManager

#endif // MOBILEAPPWIZARDPAGES_H
