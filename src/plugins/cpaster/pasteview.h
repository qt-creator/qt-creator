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

#ifndef PASTEVIEW_H
#define PASTEVIEW_H

#include "ui_pasteview.h"
#include "splitter.h"

#include <QtGui/QDialog>

class PasteView : public QDialog
{
    Q_OBJECT
public:
    explicit PasteView(QWidget *parent);
    ~PasteView();

    int show(const QString &user, const QString &description, const QString &comment,
             const FileDataList &parts);

    void addProtocol(const QString &protocol, bool defaultProtocol = false);

    QString user() const;
    QString description() const;
    QString comment() const;
    QByteArray content() const;
    QString protocol() const;

private slots:
    void contentChanged();

private:
    Ui::ViewDialog m_ui;
    FileDataList m_parts;
};

#endif // VIEW_H
