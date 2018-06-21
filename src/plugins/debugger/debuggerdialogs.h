/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "watchhandler.h"

#include <projectexplorer/kitchooser.h>
#include <projectexplorer/abi.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class QLineEdit;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class AttachToQmlPortDialogPrivate;
class DebuggerRunParameters;
class StartApplicationParameters;
class StartApplicationDialogPrivate;
class StartRemoteEngineDialogPrivate;

class DebuggerKitChooser : public ProjectExplorer::KitChooser
{
    Q_OBJECT

public:
    enum Mode { AnyDebugging, LocalDebugging };

    explicit DebuggerKitChooser(Mode mode = AnyDebugging, QWidget *parent = nullptr);

protected:
    QString kitToolTip(ProjectExplorer::Kit *k) const final;

private:
    const ProjectExplorer::Abi m_hostAbi;
    const Mode m_mode;
};

class StartApplicationDialog : public QDialog
{
    Q_OBJECT

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
    Q_OBJECT

public:
    explicit AttachToQmlPortDialog(QWidget *parent);
    ~AttachToQmlPortDialog() override;

    int port() const;
    void setPort(const int port);

    ProjectExplorer::Kit *kit() const;
    void setKitId(Core::Id id);

private:
    AttachToQmlPortDialogPrivate *d;
};

class StartRemoteCdbDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartRemoteCdbDialog(QWidget *parent);
    ~StartRemoteCdbDialog() override;

    QString connection() const;
    void setConnection(const QString &);

private:
    void textChanged(const QString &);
    void accept() override;

    QPushButton *m_okButton;
    QLineEdit *m_lineEdit;
};

class AddressDialog : public QDialog
{
    Q_OBJECT

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
    Q_OBJECT

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
    Q_OBJECT

public:
    explicit TypeFormatsDialog(QWidget *parent);
    ~TypeFormatsDialog() override;

    void addTypeFormats(const QString &type, const DisplayFormats &formats,
        int currentFormat);

private:
    TypeFormatsDialogUi *m_ui;
};

} // namespace Debugger
} // namespace Internal
