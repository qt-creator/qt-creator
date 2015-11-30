/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PASTEVIEW_H
#define PASTEVIEW_H

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

private slots:
    void contentChanged();
    void protocolChanged(int);

private:
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
#endif // VIEW_H
