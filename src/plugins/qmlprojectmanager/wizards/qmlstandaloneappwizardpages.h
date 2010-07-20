/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLSTANDALONEAPPWIZARDPAGES_H
#define QMLSTANDALONEAPPWIZARDPAGES_H

#include <QtGui/QWizardPage>
#include "qmlstandaloneapp.h"

namespace QmlProjectManager {
namespace Internal {

class QmlStandaloneAppWizardSourcesPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(QmlStandaloneAppWizardSourcesPage)

public:
    explicit QmlStandaloneAppWizardSourcesPage(QWidget *parent = 0);
    virtual ~QmlStandaloneAppWizardSourcesPage();

    QString mainQmlFile() const;
    virtual bool isComplete() const;
    void setMainQmlFileChooserVisible(bool visible);

private:
    class QmlStandaloneAppWizardSourcesPagePrivate *m_d;
};

class QmlStandaloneAppWizardOptionsPage : public QWizardPage
{
    Q_OBJECT
    Q_DISABLE_COPY(QmlStandaloneAppWizardOptionsPage)

public:
    explicit QmlStandaloneAppWizardOptionsPage(QWidget *parent = 0);
    virtual ~QmlStandaloneAppWizardOptionsPage();

    void setOrientation(QmlStandaloneApp::Orientation orientation);
    QmlStandaloneApp::Orientation orientation() const;
    QString symbianSvgIcon() const;
    void setSymbianSvgIcon(const QString &icon);
    QString symbianUid() const;
    void setLoadDummyData(bool loadIt);
    bool loadDummyData() const;
    void setNetworkEnabled(bool enableIt);
    bool networkEnabled() const;
    void setSymbianUid(const QString &uid);

private slots:
    void openSymbianSvgIcon(); // Via file open dialog

private:
    class QmlStandaloneAppWizardOptionsPagePrivate *m_d;
};

} // end of namespace Internal
} // end of namespace QmlProjectManager

#endif // QMLSTANDALONEAPPWIZARDPAGES_H
