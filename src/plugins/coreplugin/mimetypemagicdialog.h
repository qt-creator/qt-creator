/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef MIMETYPEMAGICDIALOG_H
#define MIMETYPEMAGICDIALOG_H

#include "ui_mimetypemagicdialog.h"

namespace Core {
namespace Internal {

struct MagicData
{
    MagicData() {}
    MagicData(const QString &value, const QString &type, int start, int end, int p)
        : m_value(value)
        , m_type(type)
        , m_start(start)
        , m_end(end)
        , m_priority(p) {}

    QString m_value;
    QString m_type;
    int m_start;
    int m_end;
    int m_priority;
};

class MimeTypeMagicDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MimeTypeMagicDialog(QWidget *parent = 0);

    void setMagicData(const MagicData &data);
    MagicData magicData() const;

protected:
    void changeEvent(QEvent *e);

private slots:
    void applyRecommended(bool checked);
    void validateAccept();

private:
    Ui::MimeTypeMagicDialog ui;
};

} // Internal
} // Core

#endif // MIMETYPEMAGICDIALOG_H
