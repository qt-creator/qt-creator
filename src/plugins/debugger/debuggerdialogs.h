// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitchooser.h>
#include <projectexplorer/abi.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
class QLineEdit;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace ProjectExplorer { class Kit; }

namespace Debugger::Internal {

void runAttachToRemoteServerDialog();
void runStartAndDebugApplicationDialog();
void runStartRemoteCdbSessionDialog(ProjectExplorer::Kit *kit);

class AttachToQmlPortDialog : public QDialog
{
public:
    AttachToQmlPortDialog();
    ~AttachToQmlPortDialog() override;

    int port() const;
    void setPort(const int port);

    ProjectExplorer::Kit *kit() const;
    void setKitId(Utils::Id id);

private:
    class AttachToQmlPortDialogPrivate *d;
};

class AddressDialog : public QDialog
{
public:
     AddressDialog();

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

} // Debugger::Internal
