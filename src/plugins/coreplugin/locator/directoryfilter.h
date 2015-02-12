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

#ifndef DIRECTORYFILTER_H
#define DIRECTORYFILTER_H

#include "ui_directoryfilter.h"
#include "basefilefilter.h"

#include <QString>
#include <QByteArray>
#include <QFutureInterface>
#include <QMutex>

namespace Core {
namespace Internal {

class DirectoryFilter : public BaseFileFilter
{
    Q_OBJECT

public:
    DirectoryFilter(Id id);
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    bool openConfigDialog(QWidget *parent, bool &needsRefresh);
    void refresh(QFutureInterface<void> &future);

private slots:
    void addDirectory();
    void editDirectory();
    void removeDirectory();
    void updateOptionButtons();
    void updateFileIterator();

private:
    QStringList m_directories;
    QStringList m_filters;
    // Our config dialog, uses in addDirectory and editDirectory
    // to give their dialogs the right parent
    QDialog *m_dialog;
    Ui::DirectoryFilterOptions m_ui;
    mutable QMutex m_lock;
    QStringList m_files;
};

} // namespace Internal
} // namespace Core

#endif // DIRECTORYFILTER_H
