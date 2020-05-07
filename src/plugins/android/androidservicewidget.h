/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QList>
#include <QString>
#include <QWidget>

class QPushButton;
class QTableView;

namespace Android {
namespace Internal {

struct AndroidServiceData
{
public:
    bool isValid() const;
    void setClassName(const QString &className);
    QString className() const;
    void setRunInExternalProcess(bool isRunInExternalProcess);
    bool isRunInExternalProcess() const;
    void setExternalProcessName(const QString &externalProcessName);
    QString externalProcessName() const;
    void setRunInExternalLibrary(bool isRunInExternalLibrary);
    bool isRunInExternalLibrary() const ;
    void setExternalLibraryName(const QString &externalLibraryName);
    QString externalLibraryName() const;
    void setServiceArguments(const QString &serviceArguments);
    QString serviceArguments() const;
    void setNewService(bool isNewService);
    bool isNewService() const;
private:
    QString m_className;
    bool m_isRunInExternalProcess = false;
    QString m_externalProcessName;
    bool m_isRunInExternalLibrary = false;
    QString m_externalLibName;
    QString m_serviceArguments;
    bool m_isNewService = false;
};

class AndroidServiceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AndroidServiceWidget(QWidget *parent = nullptr);
    ~AndroidServiceWidget();
    void setServices(const QList<AndroidServiceData> &androidServices);
    const QList<AndroidServiceData> &services();
    void servicesSaved();
private:
    void addService();
    void removeService();
signals:
    void servicesModified();
    void servicesInvalid();
private:
    class AndroidServiceModel;
    QScopedPointer<AndroidServiceModel> m_model;
    QTableView *m_tableView;
    QPushButton *m_removeButton;
};

} // namespace Internal
} // namespace Android
