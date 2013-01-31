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

#include "filespage.h"

#include <utils/newclasswidget.h>

#include <QLabel>
#include <QLayout>

namespace Qt4ProjectManager {
namespace Internal {

FilesPage::FilesPage(QWidget *parent) :
    QWizardPage(parent),
    m_newClassWidget(new Utils::NewClassWidget)
{
    m_newClassWidget->setPathInputVisible(false);
    setTitle(tr("Class Information"));

    QLabel *label = new QLabel(tr("Specify basic information about the classes "
        "for which you want to generate skeleton source code files."));
    label->setWordWrap(true);

    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(label);
    vlayout->addItem(new QSpacerItem(0, 20));

    vlayout->addWidget(m_newClassWidget);

    vlayout->addItem(new QSpacerItem(0, 20));
    m_errorLabel = new QLabel;
    m_errorLabel->setStyleSheet(QLatin1String("color: red;"));
    vlayout->addWidget(m_errorLabel);
    setLayout(vlayout);

    connect(m_newClassWidget, SIGNAL(validChanged()), this, SIGNAL(completeChanged()));
}

void FilesPage::setSuffixes(const QString &header, const QString &source,  const QString &form)
{
    m_newClassWidget->setSourceExtension(source);
    m_newClassWidget->setHeaderExtension(header);
    if (!form.isEmpty())
        m_newClassWidget->setFormExtension(form);
}

void FilesPage::setClassName(const QString &suggestedClassName)
{
    m_newClassWidget->setClassName(suggestedClassName);
}


bool FilesPage::isComplete() const
{
    QString error;
    const bool complete = m_newClassWidget->isValid(&error);
    m_errorLabel->setText(error);
    return complete;
}

QString FilesPage::className() const
{
    return m_newClassWidget->className();
}

QString FilesPage::baseClassName() const
{
    return m_newClassWidget->baseClassName();
}

void FilesPage::setBaseClassName(const QString &b)
{
    m_newClassWidget->setBaseClassName(b);
}

QString FilesPage::sourceFileName() const
{
    return m_newClassWidget->sourceFileName();
}

QString FilesPage::headerFileName() const
{
    return m_newClassWidget->headerFileName();
}

QString FilesPage::formFileName() const
{
    return m_newClassWidget->formFileName();
}

bool FilesPage::namespacesEnabled() const
{
    return m_newClassWidget->namespacesEnabled();
}

void FilesPage::setNamespacesEnabled(bool b)
{
    m_newClassWidget->setNamespacesEnabled(b);
}

void FilesPage::setBaseClassInputVisible(bool visible)
{
    m_newClassWidget->setBaseClassInputVisible(visible);
}

bool FilesPage::isBaseClassInputVisible() const
{
    return m_newClassWidget->isBaseClassInputVisible();
}

QStringList FilesPage::baseClassChoices() const
{
    return m_newClassWidget->baseClassChoices();
}

void FilesPage::setBaseClassChoices(const QStringList &choices)
{
    m_newClassWidget->setBaseClassChoices(choices);
}

void FilesPage::setFormFileInputVisible(bool visible)
{
    m_newClassWidget->setFormInputVisible(visible);
}

bool FilesPage::isFormInputVisible() const
{
    return m_newClassWidget->isFormInputVisible();
}

bool FilesPage::formInputCheckable() const
{
    return m_newClassWidget->formInputCheckable();
}

bool FilesPage::formInputChecked() const
{
    return m_newClassWidget->formInputChecked();
}

void FilesPage::setFormInputCheckable(bool checkable)
{
    m_newClassWidget->setFormInputCheckable(checkable);
}

void FilesPage::setFormInputChecked(bool checked)
{
    m_newClassWidget->setFormInputChecked(checked);
}

bool FilesPage::lowerCaseFiles() const
{
    return m_newClassWidget->lowerCaseFiles();
}

void FilesPage::setLowerCaseFiles(bool l)
{
    m_newClassWidget->setLowerCaseFiles(l);
}

bool FilesPage::isClassTypeComboVisible() const
{
    return m_newClassWidget->isClassTypeComboVisible();
}

void FilesPage::setClassTypeComboVisible(bool v)
{
    m_newClassWidget->setClassTypeComboVisible(v);
}

} // namespace Internal
} // namespace Qt4ProjectManager
