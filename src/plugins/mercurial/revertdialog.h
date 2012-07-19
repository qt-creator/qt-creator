/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef MERCURIAL_REVERTDIALOG_H
#define MERCURIAL_REVERTDIALOG_H

#include "ui_revertdialog.h"

#include <QDialog>

namespace Mercurial {
namespace Internal {

class RevertDialog : public QDialog
{
    Q_OBJECT

public:
    RevertDialog(QWidget *parent = 0);
    ~RevertDialog();

    QString revision() const;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::RevertDialog *m_ui;
};

} // namespace Internal
} // namespace Mercurial

#endif // MERCURIAL_REVERTDIALOG_H
