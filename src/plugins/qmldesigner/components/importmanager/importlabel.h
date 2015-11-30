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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef IMPORTLABEL_H
#define IMPORTLABEL_H

#include <QLabel>
#include <QPushButton>

#include <import.h>

namespace QmlDesigner {

class ImportLabel : public QWidget
{
    Q_OBJECT
public:
    explicit ImportLabel(QWidget *parent = 0);

    void setImport(const Import &import);
    const Import import() const;
    void setReadOnly(bool) const;

signals:
    void removeImport(const Import &import);

private slots:
    void emitRemoveImport();

private:
    Import m_import;
    QLabel *m_importLabel;
    QPushButton *m_removeButton;
};

}
#endif // IMPORTLABEL_H
