/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved
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

#ifndef BLACKBERRYNDKSETTINGSWIDGET_H
#define BLACKBERRYNDKSETTINGSWIDGET_H

#include "blackberryinstallwizard.h"

#include <QWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

class BlackBerryApiLevelConfiguration;
class BlackBerryConfigurationManager;
class Ui_BlackBerryNDKSettingsWidget;

class BlackBerryNDKSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BlackBerryNDKSettingsWidget(QWidget *parent = 0);

   bool hasActiveNdk() const;

   QList<BlackBerryApiLevelConfiguration *> activatedApiLevels();
   QList<BlackBerryApiLevelConfiguration *> deactivatedApiLevels();

   BlackBerryApiLevelConfiguration *defaultApiLevel() const;

signals:
    void configurationsUpdated();

public slots:
    void updateInfoTable(QTreeWidgetItem* currentItem);
    void updateConfigurationList();
    void addConfiguration();
    void removeConfiguration();
    void activateApiLevel();
    void deactivateApiLevel();
    void uninstallConfiguration(BlackBerryInstallerDataHandler::Target target);
    void cleanUp();
    void handleInstallationFinished();
    void handleUninstallationFinished();
    void updateUi(QTreeWidgetItem* item);
    void populateDefaultConfigurationCombo();
    void updatePage();

private:
    void launchBlackBerryInstallerWizard(BlackBerryInstallerDataHandler::Mode mode,
                                         BlackBerryInstallerDataHandler::Target target,
                                         const QString& tagetVersion = QString());

    Ui_BlackBerryNDKSettingsWidget *m_ui;
    BlackBerryConfigurationManager &m_bbConfigManager;

    QTreeWidgetItem *m_apiLevels;
    QTreeWidgetItem *m_runtimes;

    QTreeWidgetItem *m_autoDetectedNdks;
    QTreeWidgetItem *m_manualApiLevel;

    QList<BlackBerryApiLevelConfiguration *> m_activatedApiLevel;
    QList<BlackBerryApiLevelConfiguration *> m_deactivatedApiLevel;

    QTimer m_timer;
};

} // namespace Internal
} // namespeace Qnx

#endif // BLACKBERRYNDKSETTINGSWIDGET_H
