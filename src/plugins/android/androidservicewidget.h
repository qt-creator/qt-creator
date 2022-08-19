// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTableView;
QT_END_NAMESPACE

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
