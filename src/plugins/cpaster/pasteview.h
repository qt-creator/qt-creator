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

#include "ui_pasteview.h"

#include <splitter.h>

#include <QDialog>

namespace CodePaster {
class Protocol;
class PasteView : public QDialog
{
    Q_OBJECT
public:
    enum Mode
    {
        // Present a list of read-only diff chunks which the user can check for inclusion
        DiffChunkMode,
        // Present plain, editable text.
        PlainTextMode
    };

    explicit PasteView(const QList<Protocol *> &protocols,
                       const QString &mimeType,
                       QWidget *parent);
    ~PasteView() override;

    // Show up with checkable list of diff chunks.
    int show(const QString &user, const QString &description, const QString &comment,
             int expiryDays, const FileDataList &parts);
    // Show up with editable plain text.
    int show(const QString &user, const QString &description, const QString &comment,
             int expiryDays, const QString &content);

    void setProtocol(const QString &protocol);

    QString user() const;
    QString description() const;
    QString comment() const;
    QString content() const;
    QString protocol() const;
    void setExpiryDays(int d);
    int expiryDays() const;

    void accept() override;

private:
    void contentChanged();
    void protocolChanged(int);

    void setupDialog(const QString &user, const QString &description, const QString &comment);
    int showDialog();

    const QList<Protocol *> m_protocols;
    const QString m_commentPlaceHolder;
    const QString m_mimeType;

    Internal::Ui::ViewDialog m_ui;
    FileDataList m_parts;
    Mode m_mode = DiffChunkMode;
};

} // namespace CodePaster
