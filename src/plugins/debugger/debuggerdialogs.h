/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_DIALOGS_H
#define DEBUGGER_DIALOGS_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE

class QModelIndex;
class QPushButton;
class QLineEdit;
class QDialogButtonBox;

namespace Ui {
class AttachCoreDialog;
class AttachExternalDialog;
class AttachTcfDialog;
class StartExternalDialog;
class StartRemoteDialog;
class StartRemoteEngineDialog;
} // namespace Ui

QT_END_NAMESPACE


namespace Debugger {
namespace Internal {

class ProcessListFilterModel;

struct ProcData
{
    QString ppid;
    QString name;
    QString image;
    QString state;
};

class AttachCoreDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachCoreDialog(QWidget *parent);
    ~AttachCoreDialog();

    void setExecutableFile(const QString &executable);
    void setCoreFile(const QString &core);

    QString executableFile() const;
    QString coreFile() const;

private:
    Ui::AttachCoreDialog *m_ui;
};

class AttachExternalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachExternalDialog(QWidget *parent);
    ~AttachExternalDialog();

    qint64 attachPID() const;
    QString executable() const;

    virtual void accept();

private slots:
    void rebuildProcessList();
    void procSelected(const QModelIndex &index);
    void procClicked(const QModelIndex &index);
    void pidChanged(const QString &index);
    void setFilterString(const QString &filter);

private:
    inline QPushButton *okButton() const;
    inline QString attachPIDText() const;

    const QString m_selfPid;
    Ui::AttachExternalDialog *m_ui;
    ProcessListFilterModel *m_model;
};

class AttachTcfDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachTcfDialog(QWidget *parent);
    ~AttachTcfDialog();

    QString remoteChannel() const;
    void setRemoteChannel(const QString &host);

    QString remoteArchitecture() const;
    void setRemoteArchitecture(const QString &arch);
    void setRemoteArchitectures(const QStringList &arches);

    QString serverStartScript() const;
    bool useServerStartScript() const;
    void setUseServerStartScript(bool on);
    void setServerStartScript(const QString &scriptName);

private slots:
    void updateState();

private:
    Ui::AttachTcfDialog *m_ui;
};

class StartExternalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartExternalDialog(QWidget *parent);
    ~StartExternalDialog();

    QString executableFile() const;
    void setExecutableFile(const QString &executable);

    QString executableArguments() const;
    void setExecutableArguments(const QString &args);

    QString workingDirectory() const;
    void setWorkingDirectory(const QString &str);

    bool breakAtMain() const;

private:
    Ui::StartExternalDialog *m_ui;
};

class StartRemoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartRemoteDialog(QWidget *parent);
    ~StartRemoteDialog();

    QString localExecutable() const;
    void setLocalExecutable(const QString &executable);

    QString remoteChannel() const;
    void setRemoteChannel(const QString &host);

    QString remoteArchitecture() const;
    void setRemoteArchitecture(const QString &arch);
    void setRemoteArchitectures(const QStringList &arches);

    QString gnuTarget() const;
    void setGnuTarget(const QString &gnuTarget);
    void setGnuTargets(const QStringList &gnuTargets);

    bool useServerStartScript() const;
    void setUseServerStartScript(bool on);
    QString serverStartScript() const;
    void setServerStartScript(const QString &scriptName);

    QString sysRoot() const;
    void setSysRoot(const QString &sysRoot);

    QString debugger() const;
    void setDebugger(const QString &debugger);

private slots:
    void updateState();

private:
    Ui::StartRemoteDialog *m_ui;
};

class StartRemoteCdbDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartRemoteCdbDialog(QWidget *parent);
    ~StartRemoteCdbDialog();

    QString connection() const;
    void setConnection(const QString &);

    virtual void accept();

private slots:
    void textChanged(const QString &);

private:
    QPushButton *m_okButton;
    QLineEdit *m_lineEdit;
};

class AddressDialog : public QDialog {
    Q_OBJECT
public:
     explicit AddressDialog(QWidget *parent = 0);
     quint64 address() const;

     virtual void accept();

private slots:
     void textChanged();

private:
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
    ~StartRemoteEngineDialog();
    QString username() const;
    QString host() const;
    QString password() const;
    QString enginePath() const;
    QString inferiorPath() const;

private:
    Ui::StartRemoteEngineDialog *m_ui;
};

} // namespace Debugger
} // namespace Internal

#endif // DEBUGGER_DIALOGS_H
