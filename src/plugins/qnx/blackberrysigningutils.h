/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_BLACKBERRYSIGNINGUTILS_H
#define QNX_INTERNAL_BLACKBERRYSIGNINGUTILS_H

#include <QtGlobal>
#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

class BlackBerryCertificate;

class BlackBerrySigningUtils : public QObject
{
    Q_OBJECT

public:
    enum Status {
        NotOpened,
        Opening,
        Opened
    };

    static BlackBerrySigningUtils &instance();

    bool hasRegisteredKeys();
    bool hasLegacyKeys();
    bool hasDefaultCertificate();

    QString cskPassword(QWidget *passwordPromptParent = 0, bool *ok = 0);
    QString certificatePassword(QWidget *passwordPromptParent = 0, bool *ok = 0);

    const BlackBerryCertificate *defaultCertificate() const;
    Status defaultCertificateOpeningStatus() const;

    void openDefaultCertificate(QWidget *passwordPromptParent = 0);
    void setDefaultCertificate(BlackBerryCertificate *certificate);
    void clearCskPassword();
    void clearCertificatePassword();
    void deleteDefaultCertificate();
    bool createCertificate();
    void addDebugToken(const QString &dt);
    void removeDebugToken(const QString &dt);

    QStringList debugTokens() const;

signals:
    void defaultCertificateLoaded(int status);
    void debugTokenListChanged();

public slots:
    void saveDebugTokens();

private slots:
    void certificateLoaded(int status);
    void loadDebugTokens();

private:
    Q_DISABLE_COPY(BlackBerrySigningUtils)

    BlackBerrySigningUtils(QObject *parent = 0);

    QString promptPassword(const QString &message, QWidget *dialogParent = 0, bool *ok = 0) const;

    BlackBerryCertificate *m_defaultCertificate;
    Status m_defaultCertificateStatus;

    QString m_cskPassword;
    QString m_certificatePassword;

    QStringList m_debugTokens;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYSIGNINGUTILS_H
