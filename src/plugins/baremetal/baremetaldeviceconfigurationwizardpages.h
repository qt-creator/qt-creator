/****************************************************************************
**
** Copyright (C) 2014 Tim Sander <tim@krieglstein.org>
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

#ifndef BAREMETALDEVICECONFIGURATIONWIZARDPAGES_H
#define BAREMETALDEVICECONFIGURATIONWIZARDPAGES_H

#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSpinBox;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {

class BareMetalDeviceConfigurationWizardSetupPage : public QWizardPage
{
public:
    explicit BareMetalDeviceConfigurationWizardSetupPage(QWidget *parent = 0);

    void initializePage();
    bool isComplete() const;
    QString configurationName() const;
    QString gdbHostname() const;
    quint16 gdbPort() const;
    QString gdbResetCommands() const;
    QString gdbInitCommands() const;

    virtual QString defaultConfigurationName() const;

private:
    QLineEdit *m_nameLineEdit;
    QLineEdit *m_hostNameLineEdit;
    QSpinBox *m_portSpinBox;
    QPlainTextEdit *m_gdbResetCommandsTextEdit;
    QPlainTextEdit *m_gdbInitCommandsPlainTextEdit;
};

} // namespace Internal
} // namespace BareMetal

#endif // BAREMETALDEVICECONFIGURATIONWIZARDPAGES_H
