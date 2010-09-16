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

namespace Qt4ProjectManager {
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
    void setModulesError(const QString &error);
    QStringList moduleUris() const;
    QStringList moduleImportPaths() const;

private slots:
    void on_addModuleUriButton_clicked();
    void on_removeModuleUriButton_clicked();
    void on_addImportPathButton_clicked();
    void on_removeImportPathButton_clicked();
    void handleModulesChanged();

signals:
    void externalModulesChanged(const QStringList &uris, const QStringList &importPaths) const;

private:
    class QmlStandaloneAppWizardSourcesPagePrivate *m_d;
};

} // end of namespace Internal
} // end of namespace Qt4ProjectManager

#endif // QMLSTANDALONEAPPWIZARDPAGES_H
