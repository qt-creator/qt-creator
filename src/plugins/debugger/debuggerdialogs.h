/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef DEBUGGER_DIALOGS_H
#define DEBUGGER_DIALOGS_H

#include <projectexplorer/kitchooser.h>
#include <projectexplorer/abi.h>

#include <QDialog>
#include <QHash>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QPushButton;
class QLineEdit;
class QDialogButtonBox;
class QSettings;
QT_END_NAMESPACE

namespace Core { class Id; }
namespace ProjectExplorer { class Kit; }

namespace Debugger {
class DebuggerStartParameters;

namespace Internal {

class AttachCoreDialogPrivate;
class AttachToQmlPortDialogPrivate;
class ProcessListFilterModel;
class StartApplicationParameters;
class StartApplicationDialogPrivate;
class StartRemoteEngineDialogPrivate;

class DebuggerKitChooser : public ProjectExplorer::KitChooser {
    Q_OBJECT
public:
    enum Mode { RemoteDebugging, LocalDebugging };

    explicit DebuggerKitChooser(Mode mode = RemoteDebugging, QWidget *parent = 0);

protected:
    bool kitMatches(const ProjectExplorer::Kit *k) const;
    QString kitToolTip(ProjectExplorer::Kit *k) const;

private:
    const ProjectExplorer::Abi m_hostAbi;
    const Mode m_mode;
};

class StartApplicationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartApplicationDialog(QWidget *parent);
    ~StartApplicationDialog();

    static bool run(QWidget *parent, QSettings *settings,
                    DebuggerStartParameters *sp);

private slots:
    void historyIndexChanged(int);
    void updateState();

private:
    StartApplicationParameters parameters() const;
    void setParameters(const StartApplicationParameters &p);
    void setHistory(const QList<StartApplicationParameters> &l);

    StartApplicationDialogPrivate *d;
};

class AttachToQmlPortDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachToQmlPortDialog(QWidget *parent);
    ~AttachToQmlPortDialog();

    int port() const;
    void setPort(const int port);

    ProjectExplorer::Kit *kit() const;
    void setKitId(const Core::Id &id);

private:
    AttachToQmlPortDialogPrivate *d;
};

class StartRemoteCdbDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartRemoteCdbDialog(QWidget *parent);
    ~StartRemoteCdbDialog();

    QString connection() const;
    void setConnection(const QString &);

private slots:
    void textChanged(const QString &);

private:
    void accept();

    QPushButton *m_okButton;
    QLineEdit *m_lineEdit;
};

class AddressDialog : public QDialog
{
    Q_OBJECT
public:
     explicit AddressDialog(QWidget *parent = 0);

     void setAddress(quint64 a);
     quint64 address() const;

private slots:
     void textChanged();

private:
     void accept();

     void setOkButtonEnabled(bool v);
     bool isOkButtonEnabled() const;

     bool isValid() const;

     QLineEdit *m_lineEdit;
     QDialogButtonBox *m_box;
};

typedef QHash<QString, QStringList> TypeFormats;

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
    StartRemoteEngineDialogPrivate *d;
};

class TypeFormatsDialogUi;

class TypeFormatsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TypeFormatsDialog(QWidget *parent);
    ~TypeFormatsDialog();

    void addTypeFormats(const QString &type, const QStringList &formats,
        int currentFormat);
    TypeFormats typeFormats() const;

private:
    TypeFormatsDialogUi *m_ui;
};

} // namespace Debugger
} // namespace Internal

#endif // DEBUGGER_DIALOGS_H
