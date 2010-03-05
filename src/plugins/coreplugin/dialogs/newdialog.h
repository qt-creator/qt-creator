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

#ifndef NEWDIALOG_H
#define NEWDIALOG_H

#include "iwizard.h"

#include <QtGui/QDialog>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTreeWidgetItem;
class QStringList;
QT_END_NAMESPACE

namespace Core {

namespace Internal {

namespace Ui {
    class NewDialog;
}

class NewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewDialog(QWidget *parent);
    virtual ~NewDialog();

    void setWizards(QList<IWizard*> wizards);
    void setPreferredWizardKinds(IWizard::WizardKinds kinds);

    Core::IWizard *showDialog();

private slots:
    void currentItemChanged(QTreeWidgetItem *cat);
    void okButtonClicked();
    void updateOkButton();

private:
    Core::IWizard *currentWizard() const;

    Ui::NewDialog *m_ui;
    QPushButton *m_okButton;
    IWizard::WizardKinds m_preferredWizardKinds;
};

} // namespace Internal
} // namespace Core

#endif // NEWDIALOG_H
