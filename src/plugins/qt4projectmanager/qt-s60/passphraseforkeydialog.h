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

#ifndef PASSPHRASEFORKEYDIALOG_H
#define PASSPHRASEFORKEYDIALOG_H

#include <QtGui/QDialog>

QT_FORWARD_DECLARE_CLASS(QCheckBox);
QT_FORWARD_DECLARE_CLASS(QDialogButtonBox);
QT_FORWARD_DECLARE_CLASS(QLineEdit);

namespace Qt4ProjectManager {

class PassphraseForKeyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PassphraseForKeyDialog(const QString &keyName, QWidget *parent = 0);

    QString passphrase() const;
    bool savePassphrase() const;

protected slots:
    void passphraseChanged();

private:
    QDialogButtonBox *m_buttonBox;
    QCheckBox *m_saveCheckBox;
    QLineEdit *m_passphraseEdit;
};

} // namespace Qt4ProjectManager

#endif // PASSPHRASEFORKEYDIALOG_H
