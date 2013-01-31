/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDCREATEKEYSTORECERTIFICATE_H
#define ANDROIDCREATEKEYSTORECERTIFICATE_H

#include <utils/fileutils.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class AndroidCreateKeystoreCertificate; }
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidCreateKeystoreCertificate : public QDialog
{
    Q_OBJECT
    enum PasswordStatus
    {
        Invalid,
        NoMatch,
        Match
    };

public:
    explicit AndroidCreateKeystoreCertificate(QWidget *parent = 0);
    ~AndroidCreateKeystoreCertificate();
    Utils::FileName keystoreFilePath();
    QString keystorePassword();
    QString certificateAlias();
    QString certificatePassword();

private slots:
    PasswordStatus checkKeystorePassword();
    PasswordStatus checkCertificatePassword();
    void on_keystoreShowPassCheckBox_stateChanged(int state);
    void on_certificateShowPassCheckBox_stateChanged(int state);
    void on_buttonBox_accepted();

private:
    Ui::AndroidCreateKeystoreCertificate *ui;
    Utils::FileName m_keystoreFilePath;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDCREATEKEYSTORECERTIFICATE_H
