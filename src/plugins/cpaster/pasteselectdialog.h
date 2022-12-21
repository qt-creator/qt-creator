// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>
#include <QList>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QListWidget;
class QPushButton;
QT_END_NAMESPACE

namespace CodePaster {

class Protocol;

class PasteSelectDialog : public QDialog
{
public:
    explicit PasteSelectDialog(const QList<Protocol*> &protocols,
                               QWidget *parent = nullptr);
    ~PasteSelectDialog() override;

    QString pasteId() const;

    int protocol() const;
    void setProtocol(const QString &);

private:
    QString protocolName() const;
    void protocolChanged(int);
    void list();
    void listDone(const QString &name, const QStringList &items);

    const QList<Protocol*> m_protocols;

    QComboBox *m_protocolBox;
    QListWidget *m_listWidget;
    QPushButton *m_refreshButton;
    QLineEdit *m_pasteEdit;
};

} // CodePaster
