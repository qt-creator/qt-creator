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

#include "qtquickappwizardpages.h"
#include "ui_qtquickcomponentsetoptionspage.h"
#include <coreplugin/coreconstants.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QFileDialog>
#include <QMessageBox>

namespace Qt4ProjectManager {
namespace Internal {

class QtQuickComponentSetOptionsPagePrivate
{
    Ui::QtQuickComponentSetOptionsPage ui;
    friend class QtQuickComponentSetOptionsPage;
};

QtQuickComponentSetOptionsPage::QtQuickComponentSetOptionsPage(QWidget *parent)
    : QWizardPage(parent)
    , d(new QtQuickComponentSetOptionsPagePrivate)
{
    d->ui.setupUi(this);

    d->ui.importLineEdit->setExpectedKind(Utils::PathChooser::File);
    d->ui.importLineEdit->setPromptDialogFilter(QLatin1String("*.qml"));
    d->ui.importLineEdit->setPromptDialogTitle(tr("Select QML File"));
    connect(d->ui.importLineEdit, SIGNAL(changed(QString)), SIGNAL(completeChanged()));

    setTitle(tr("Select Existing QML file"));
}

QtQuickComponentSetOptionsPage::~QtQuickComponentSetOptionsPage()
{
    delete d;
}

QString QtQuickComponentSetOptionsPage::mainQmlFile() const
{
    return d->ui.importLineEdit->path();
}

bool QtQuickComponentSetOptionsPage::isComplete() const
{
    return d->ui.importLineEdit->isValid();
}

} // namespace Internal
} // namespace Qt4ProjectManager
