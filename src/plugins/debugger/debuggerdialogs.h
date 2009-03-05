/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_DIALOGS_H
#define DEBUGGER_DIALOGS_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE

class QModelIndex;
class QStandardItemModel;
class QSortFilterProxyModel;
class QPushButton;

namespace Ui {
class AttachCoreDialog;
class AttachExternalDialog;
class AttachRemoteDialog;
class StartExternalDialog;
} // namespace Ui

QT_END_NAMESPACE


namespace Debugger {
namespace Internal {

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

    int attachPID() const;

private slots:
    void rebuildProcessList();
    void procSelected(const QModelIndex &);
    void pidChanged(const QString &);

private:
    inline QPushButton *okButton() const;
    const QString m_selfPid;

    Ui::AttachExternalDialog *m_ui;
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
};


class AttachRemoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AttachRemoteDialog(QWidget *parent, const QString &pid);
    ~AttachRemoteDialog();

    int attachPID() const;

private slots:
    void rebuildProcessList();
    void procSelected(const QModelIndex &);

private:
    Ui::AttachRemoteDialog *m_ui;
    QString m_defaultPID;
    QStandardItemModel *m_model;
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

private:
    Ui::StartExternalDialog *m_ui;
};

} // namespace Debugger
} // namespace Internal

#endif // DEBUGGER_DIALOGS_H
