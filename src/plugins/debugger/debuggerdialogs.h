/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
class StartExternalDialog;
class StartRemoteDialog;
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

private slots:
    void rebuildProcessList();
    void procSelected(const QModelIndex &);
    void pidChanged(const QString &);
    void setFilterString(const QString &filter);

private:
    inline QPushButton *okButton() const;
    const QString m_selfPid;

    Ui::AttachExternalDialog *m_ui;
    ProcessListFilterModel *m_model;
};

class StartExternalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartExternalDialog(QWidget *parent);
    ~StartExternalDialog();

    void setExecutableFile(const QString &executable);
    void setExecutableArguments(const QString &args);

    QString executableFile() const;
    QString executableArguments() const;
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

    void setRemoteChannel(const QString &host);
    void setRemoteArchitecture(const QString &arch);
    void setRemoteArchitectures(const QStringList &arches);
    QString remoteChannel() const;
    QString remoteArchitecture() const;
    void setServerStartScript(const QString &scriptName);
    QString serverStartScript() const;
    void setUseServerStartScript(bool on);
    bool useServerStartScript() const;

private slots:
    void updateState();

private:
    Ui::StartRemoteDialog *m_ui;
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

} // namespace Debugger
} // namespace Internal

#endif // DEBUGGER_DIALOGS_H
