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

#ifndef PYTHONEDITOR_CLASSWIZARDDIALOG_H
#define PYTHONEDITOR_CLASSWIZARDDIALOG_H

#include <utils/wizard.h>
#include <utils/newclasswidget.h>
#include <QScopedPointer>
#include <QVariantMap>

namespace PythonEditor {

class ClassNamePage;

class ClassWizardParameters
{
public:
    QString className;
    QString fileName;
    QString path;
    QString baseClass;
    Utils::NewClassWidget::ClassType classType;
};

class ClassWizardDialog : public Utils::Wizard
{
    Q_OBJECT
public:
    explicit ClassWizardDialog(QWidget *parent = 0);
    virtual ~ClassWizardDialog();

    void setPath(const QString &path);
    ClassWizardParameters parameters() const;

    void setExtraValues(const QVariantMap &extraValues);
    const QVariantMap &extraValues() const;

private:
    QScopedPointer<ClassNamePage> m_classNamePage;
    QVariantMap m_extraValues;
};

} // namespace PythonEditor

#endif // PYTHONEDITOR_CLASSWIZARDDIALOG_H
