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

#ifndef QMLSTANDALONEAPPWIZARD_H
#define QMLSTANDALONEAPPWIZARD_H

#include "abstractmobileappwizard.h"

namespace Qt4ProjectManager {
namespace Internal {

class QmlStandaloneAppWizard : public AbstractMobileAppWizard
{
    Q_OBJECT

public:
    enum WizardType {
        NewQmlFile,
        ImportQmlFile
    };

    QmlStandaloneAppWizard(WizardType type);
    virtual ~QmlStandaloneAppWizard();

private slots:
    void handleModulesChange(const QStringList &uris, const QStringList &paths);

private:
    static Core::BaseFileWizardParameters parameters(WizardType type);

    virtual AbstractMobileApp *app() const;
    virtual AbstractMobileAppWizardDialog *wizardDialog() const;
    virtual AbstractMobileAppWizardDialog *createWizardDialogInternal(QWidget *parent) const;
    virtual void prepareGenerateFiles(const QWizard *wizard,
        QString *errorMessage) const;
    virtual bool postGenerateFilesInternal(const Core::GeneratedFiles &l,
        QString *errorMessage);

    class QmlStandaloneAppWizardPrivate *m_d;
};

} // end of namespace Internal
} // end of namespace Qt4ProjectManager

#endif // QMLSTANDALONEAPPWIZARD_H
