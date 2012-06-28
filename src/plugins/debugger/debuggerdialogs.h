/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_DIALOGS_H
#define DEBUGGER_DIALOGS_H

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
namespace ProjectExplorer { class Profile; }

namespace Debugger {
class DebuggerStartParameters;

namespace Internal {

namespace Ui {
class AttachCoreDialog;
class AttachExternalDialog;
class StartExternalDialog;
class StartRemoteDialog;
class StartRemoteEngineDialog;
} // namespace Ui

class ProcessListFilterModel;
class StartExternalParameters;
class StartRemoteParameters;
class AttachToQmlPortDialogPrivate;

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

    int profileIndex() const;
    void setProfileIndex(int);
    Core::Id profileId() const;

    QString overrideStartScript() const;
    void setOverrideStartScript(const QString &scriptName);

private slots:
    void changed();

private:
    bool isValid() const;

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

    int profileIndex() const;
    void setProfileIndex(int);
    Core::Id profileId() const;

    void accept();

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

class StartExternalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartExternalDialog(QWidget *parent);
    ~StartExternalDialog();

    static bool run(QWidget *parent, QSettings *settings, DebuggerStartParameters *sp);

private slots:
    void changed();
    void historyIndexChanged(int);

private:
    StartExternalParameters parameters() const;
    void setParameters(const StartExternalParameters &p);
    void setHistory(const QList<StartExternalParameters> l);

    QString executableFile() const;
    void setExecutableFile(const QString &executable);

    Core::Id profileId() const;
    bool isValid() const;

    Ui::StartExternalDialog *m_ui;
};

class StartRemoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartRemoteDialog(QWidget *parent, bool enableStartScript);
    ~StartRemoteDialog();

    static bool run(QWidget *parent, QSettings *settings,
                    bool useScript, DebuggerStartParameters *sp);

private slots:
    void updateState();
    void historyIndexChanged(int);

private:
    StartRemoteParameters parameters() const;
    void setParameters(const StartRemoteParameters &);
    void setHistory(const QList<StartRemoteParameters> &);

    void setRemoteArchitectures(const QStringList &list);

    Core::Id profileId() const;

    Ui::StartRemoteDialog *m_ui;
};

class AttachToQmlPortDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachToQmlPortDialog(QWidget *parent);
    ~AttachToQmlPortDialog();

    QString host() const;
    void setHost(const QString &host);

    int port() const;
    void setPort(const int port);

    Core::Id profileId() const;
    void setProfileId(const Core::Id &id);

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
    Ui::StartRemoteEngineDialog *m_ui;
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
