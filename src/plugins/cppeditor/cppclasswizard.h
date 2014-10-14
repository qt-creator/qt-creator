/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPCLASSWIZARD_H
#define CPPCLASSWIZARD_H

#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>

#include <utils/wizard.h>

#include <QWizardPage>

namespace Utils { class NewClassWidget; }

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

class CppClassWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    explicit CppClassWizardDialog(QWidget *parent = 0);

    void setPath(const QString &path);
    CppClassWizardParameters parameters() const;

private:
    ClassNamePage *m_classNamePage;
};


class CppClassWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    CppClassWizard();

private:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const;

    Core::GeneratedFiles generateFiles(const QWizard *w,
                                               QString *errorMessage) const;
    QString sourceSuffix() const;
    QString headerSuffix() const;

    static bool generateHeaderAndSource(const CppClassWizardParameters &params,
                                        QString *header, QString *source);
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPCLASSWIZARD_H
