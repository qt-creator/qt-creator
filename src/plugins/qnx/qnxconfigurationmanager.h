/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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

#ifndef QNXCONFIGURATIONMANAGER_H
#define QNXCONFIGURATIONMANAGER_H

#include <utils/fileutils.h>

namespace Utils { class PersistentSettingsWriter; }
namespace Qnx {
namespace Internal {

class QnxConfiguration;
class QnxPlugin;

class QnxConfigurationManager: public QObject
{
    Q_OBJECT
public:
    static QnxConfigurationManager *instance();
    ~QnxConfigurationManager();
    QList<QnxConfiguration*> configurations() const;
    void removeConfiguration(QnxConfiguration *config);
    bool addConfiguration(QnxConfiguration *config);
    QnxConfiguration* configurationFromEnvFile(const Utils::FileName &envFile) const;

protected slots:
    void saveConfigs();

signals:
    void configurationsListUpdated();

private:
    QnxConfigurationManager(QObject *parent = 0);
    static QnxConfigurationManager *m_instance;
    QList<QnxConfiguration*> m_configurations;
    Utils::PersistentSettingsWriter *m_writer;
    void restoreConfigurations();

    friend class QnxPlugin;
};

}
}

#endif // QNXCONFIGURATIONMANAGER_H
