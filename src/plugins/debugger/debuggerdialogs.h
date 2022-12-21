// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "watchhandler.h"

#include <projectexplorer/kitchooser.h>
#include <projectexplorer/abi.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class QLineEdit;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Debugger::Internal {

class AttachToQmlPortDialogPrivate;
class DebuggerRunParameters;
class StartApplicationParameters;
class StartApplicationDialogPrivate;
class StartRemoteEngineDialogPrivate;

class StartApplicationDialog : public QDialog
{
public:
    explicit StartApplicationDialog(QWidget *parent);
    ~StartApplicationDialog() override;

    static void attachToRemoteServer();
    static void startAndDebugApplication();

private:
    void historyIndexChanged(int);
    void updateState();
    StartApplicationParameters parameters() const;
    void setParameters(const StartApplicationParameters &p);
    void setHistory(const QList<StartApplicationParameters> &l);
    void onChannelOverrideChanged(const QString &channel);
    static void run(bool);

    StartApplicationDialogPrivate *d;
};

class AttachToQmlPortDialog : public QDialog
{
public:
    explicit AttachToQmlPortDialog(QWidget *parent);
    ~AttachToQmlPortDialog() override;

    int port() const;
    void setPort(const int port);

    ProjectExplorer::Kit *kit() const;
    void setKitId(Utils::Id id);

private:
    AttachToQmlPortDialogPrivate *d;
};

class StartRemoteCdbDialog : public QDialog
{
public:
    explicit StartRemoteCdbDialog(QWidget *parent);
    ~StartRemoteCdbDialog() override;

    QString connection() const;
    void setConnection(const QString &);

private:
    void textChanged(const QString &);
    void accept() override;

    QPushButton *m_okButton = nullptr;
    QLineEdit *m_lineEdit;
};

class AddressDialog : public QDialog
{
public:
     explicit AddressDialog(QWidget *parent = nullptr);

     void setAddress(quint64 a);
     quint64 address() const;

private:
     void textChanged();
     void accept() override;

     void setOkButtonEnabled(bool v);
     bool isOkButtonEnabled() const;

     bool isValid() const;

     QLineEdit *m_lineEdit;
     QDialogButtonBox *m_box;
};

class StartRemoteEngineDialog : public QDialog
{
public:
    explicit StartRemoteEngineDialog(QWidget *parent);
    ~StartRemoteEngineDialog() override;
    QString username() const;
    QString host() const;
    QString password() const;
    QString enginePath() const;
    QString inferiorPath() const;

private:
    StartRemoteEngineDialogPrivate *d;
};

class TypeFormatsDialogUi;

class TypeFormatsDialog : public QDialog
{
public:
    explicit TypeFormatsDialog(QWidget *parent);
    ~TypeFormatsDialog() override;

    void addTypeFormats(const QString &type, const DisplayFormats &formats,
        int currentFormat);

private:
    TypeFormatsDialogUi *m_ui;
};

} // Debugger::Internal
