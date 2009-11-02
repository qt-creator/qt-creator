/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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

#ifndef REVERTDIALOG_H
#define REVERTDIALOG_H

#include "ui_revertdialog.h"

#include <QtGui/QDialog>


namespace Mercurial {
namespace Internal {

class mercurialPlugin;

class RevertDialog : public QDialog
{
    Q_OBJECT
public:
    RevertDialog(QWidget *parent = 0);
    ~RevertDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::RevertDialog *m_ui;
    friend class MercurialPlugin;
};

} // namespace Internal
} // namespace Mercurial
#endif // REVERTDIALOG_H
