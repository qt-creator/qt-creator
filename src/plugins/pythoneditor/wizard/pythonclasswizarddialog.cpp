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

#include "pythonclasswizarddialog.h"
#include "pythonclassnamepage.h"

#include <utils/newclasswidget.h>
#include <coreplugin/basefilewizard.h>

namespace PythonEditor {

ClassWizardDialog::ClassWizardDialog(QWidget *parent)
    : Utils::Wizard(parent)
    , m_classNamePage(new ClassNamePage(this))
{
    setWindowTitle(tr("Python Class Wizard"));
    Core::BaseFileWizard::setupWizard(this);
    const int classNameId = addPage(m_classNamePage.data());
    wizardProgress()->item(classNameId)->setTitle(tr("Details"));
}

ClassWizardDialog::~ClassWizardDialog()
{
}

ClassWizardParameters ClassWizardDialog::parameters() const
{
    ClassWizardParameters p;
    const Utils::NewClassWidget *ncw = m_classNamePage->newClassWidget();
    p.className = ncw->className();
    p.fileName = ncw->sourceFileName();
    p.baseClass = ncw->baseClassName();
    p.path = ncw->path();
    p.classType = ncw->classType();

    return p;
}

void ClassWizardDialog::setExtraValues(const QVariantMap &extraValues)
{
    m_extraValues = extraValues;
}

const QVariantMap &ClassWizardDialog::extraValues() const
{
    return m_extraValues;
}

void ClassWizardDialog::setPath(const QString &path)
{
    m_classNamePage->newClassWidget()->setPath(path);
}

} // namespace PythonEditor
