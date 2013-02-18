/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BLACKBERRYREGISTERKEYDIALOG_H
#define QNX_INTERNAL_BLACKBERRYREGISTERKEYDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
}

namespace Qnx {
namespace Internal {

class Ui_BlackBerryRegisterKeyDialog;
class BlackBerryCsjRegistrar;
class BlackBerryCertificate;

class BlackBerryRegisterKeyDialog : public QDialog
{
Q_OBJECT

public:
    explicit BlackBerryRegisterKeyDialog(QWidget *parent = 0,
            Qt::WindowFlags f = 0);

    QString pbdtPath() const;
    QString rdkPath() const;
    QString csjPin() const;
    QString cskPin() const;
    QString keystorePassword() const;
    QString keystorePath() const;

    BlackBerryCertificate *certificate() const;

private slots:
    void csjAutoComplete(const QString &path);
    void validate();
    void createKey();
    void pinCheckBoxChanged(int state);
    void certCheckBoxChanged(int state);
    void registrarFinished(int status, const QString &errorString);
    void certificateCreated(int status);

private:
    void setupCsjPathChooser(Utils::PathChooser *chooser);
    void generateDeveloperCertificate();
    void cleanup() const;
    void setBusy(bool busy);

    QString getCsjAuthor(const QString &fileName) const;

    Ui_BlackBerryRegisterKeyDialog *m_ui;

    BlackBerryCsjRegistrar *m_registrar;

    BlackBerryCertificate *m_certificate;

    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYREGISTERKEYDIALOG_H
