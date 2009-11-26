/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLNEWPROJECTWIZARD_H
#define QMLNEWPROJECTWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <projectexplorer/baseprojectwizarddialog.h>

namespace Utils {

class FileWizardPage;
class ProjectIntroPage;

} // namespace Utils


namespace QmlProjectManager {
namespace Internal {

class QmlNewProjectWizardDialog : public ProjectExplorer::BaseProjectWizardDialog
{
    Q_OBJECT
public:
    explicit QmlNewProjectWizardDialog(QWidget *parent = 0);
};

class QmlNewProjectWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    QmlNewProjectWizard();
    virtual ~QmlNewProjectWizard();

    static Core::BaseFileWizardParameters parameters();

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const QString &defaultPath,
                                        const WizardPageList &extensionPages) const;

    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;

    virtual bool postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage);
};

} // end of namespace Internal
} // end of namespace QmlProjectManager

#endif // QMLNEWPROJECTWIZARD_H
