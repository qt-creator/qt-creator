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

#ifndef BLACKBERRYKEYSWIDGET_H_H
#define BLACKBERRYKEYSWIDGET_H_H

#include <QWidget>
#include <QString>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QModelIndex;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

class BlackBerryCertificate;
class BlackBerrySigningUtils;
class BlackBerryDebugTokenRequester;
class Ui_BlackBerryKeysWidget;

class BlackBerryKeysWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BlackBerryKeysWidget(QWidget *parent = 0);
    void saveSettings();

private slots:
    void certificateLoaded(int status);
    void createCertificate();
    void clearCertificate();
    void loadDefaultCertificate();
    void updateDebugTokenList();

    void requestDebugToken();
    void importDebugToken();
    void editDebugToken();
    void removeDebugToken();
    void updateDebugToken(const QStringList &pins);
    void requestFinished(int status);
    void updateUi(const QModelIndex &index);

protected:
    void showEvent(QShowEvent *event);

private:
    void updateKeysSection();
    void updateCertificateSection();
    void setCertificateError(const QString &error);
    void setCreateCertificateVisible(bool show);
    void initModel();

    BlackBerrySigningUtils &m_utils;

    Ui_BlackBerryKeysWidget *m_ui;
    QStandardItemModel *m_dtModel;
    BlackBerryDebugTokenRequester *m_requester;
};

} // namespace Internal
} // namespeace Qnx

#endif // BLACKBERRYKEYSWIDGET_H_H
