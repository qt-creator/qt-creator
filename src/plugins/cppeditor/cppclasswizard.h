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

#ifndef CPPCLASSWIZARD_H
#define CPPCLASSWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <utils/wizard.h>

#include <QStringList>
#include <QWizardPage>

namespace Utils {

class NewClassWidget;

} // namespace Utils

namespace CppEditor {
namespace Internal {

class ClassNamePage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ClassNamePage(QWidget *parent = 0);

    bool isComplete() const { return m_isValid; }
    Utils::NewClassWidget *newClassWidget() const { return m_newClassWidget; }

private slots:
    void slotValidChanged();

private:
    void initParameters();

    Utils::NewClassWidget *m_newClassWidget;
    bool m_isValid;
};


struct CppClassWizardParameters
{
    QString className;
    QString headerFile;
    QString sourceFile;
    QString baseClass;
    QString path;
    int classType;
};

class CppClassWizardDialog : public Utils::Wizard
{
    Q_OBJECT

public:
    explicit CppClassWizardDialog(QWidget *parent = 0);

    void setPath(const QString &path);
    CppClassWizardParameters parameters() const;

private:
    ClassNamePage *m_classNamePage;
};


class CppClassWizard : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    explicit CppClassWizard(const Core::BaseFileWizardParameters &parameters,
                            QObject *parent = 0);

    virtual Core::FeatureSet requiredFeatures() const;

protected:
    virtual QWizard *createWizardDialog(QWidget *parent,
                                        const Core::WizardDialogParameters &wizardDialogParameters) const;


    virtual Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;
    QString sourceSuffix() const;
    QString headerSuffix() const;

private:
    static bool generateHeaderAndSource(const CppClassWizardParameters &params,
                                        QString *header, QString *source);

};

} // namespace Internal
} // namespace CppEditor

#endif // CPPCLASSWIZARD_H
