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

#ifndef QTQUICKAPPWIZARD_H
#define QTQUICKAPPWIZARD_H

#include "abstractmobileappwizard.h"

namespace Qt4ProjectManager {
namespace Internal {

class QtQuickAppWizard : public AbstractMobileAppWizard
{
    Q_OBJECT
public:

    enum Kind {
        QtQuick1_1 = 0,
        QtQuick2_0 = 1,
        MeegoComponents = 2,
        ImportQml = 3
    };

    QtQuickAppWizard();
    explicit QtQuickAppWizard(const Core::BaseFileWizardParameters &params, QObject *parent = 0);
    virtual ~QtQuickAppWizard();
    static void createInstances(ExtensionSystem::IPlugin *plugin);

protected:
    QString fileToOpenPostGeneration() const;

private:
    static Core::BaseFileWizardParameters baseParameters();

    virtual AbstractMobileApp *app() const;
    virtual AbstractMobileAppWizardDialog *wizardDialog() const;
    virtual AbstractMobileAppWizardDialog *createWizardDialogInternal(QWidget *parent,
                                                                      const Core::WizardDialogParameters &baseParameters) const;
    virtual void projectPathChanged(const QString &path) const;
    virtual void prepareGenerateFiles(const QWizard *wizard,
        QString *errorMessage) const;
    void setQtQuickKind(Kind kind);
    Kind qtQuickKind() const;

    class QtQuickAppWizardPrivate *d;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QTQUICKAPPWIZARD_H
