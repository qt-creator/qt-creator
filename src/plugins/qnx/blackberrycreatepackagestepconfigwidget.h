/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
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

#ifndef QNX_INTERNAL_BLACKBERRYCREATEPACKAGESTEPCONFIGWIDGET_H
#define QNX_INTERNAL_BLACKBERRYCREATEPACKAGESTEPCONFIGWIDGET_H

#include <projectexplorer/buildstep.h>

namespace Qnx {
namespace Internal {

namespace Ui { class BlackBerryCreatePackageStepConfigWidget; }

class BlackBerryCreatePackageStep;

class BlackBerryCreatePackageStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    explicit BlackBerryCreatePackageStepConfigWidget(BlackBerryCreatePackageStep *step);
    ~BlackBerryCreatePackageStepConfigWidget();

    QString displayName() const;
    QString summaryText() const;

    bool showWidget() const;

private slots:
    void setPackageMode(bool signPackagesChecked);
    void showPasswords(bool show);
    void setBundleMode(int qtLibraryIndex);
    void updateDeployWidgetsState();
    void deployLibraries();

signals:
    void bundleModeChanged();

private:
    BlackBerryCreatePackageStep *m_step;
    Ui::BlackBerryCreatePackageStepConfigWidget *m_ui;

    QMap<int, QString> m_qtLibraryExplanations;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYCREATEPACKAGESTEPCONFIGWIDGET_H
