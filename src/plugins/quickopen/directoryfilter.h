/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DIRECTORYFILTER_H
#define DIRECTORYFILTER_H

#include "ui_directoryfilter.h"
#include "basefilefilter.h"

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QByteArray>
#include <QtCore/QFutureInterface>
#include <QtCore/QMutex>
#include <QtGui/QWidget>
#include <QtGui/QDialog>

namespace QuickOpen {
namespace Internal {

class DirectoryFilter : public BaseFileFilter
{
    Q_OBJECT

public:
    DirectoryFilter();
    QString trName() const { return m_name; }
    QString name() const { return m_name; }
    QuickOpen::IQuickOpenFilter::Priority priority() const { return QuickOpen::IQuickOpenFilter::Medium; }
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    bool openConfigDialog(QWidget *parent, bool &needsRefresh);
    void refresh(QFutureInterface<void> &future);

private slots:
    void addDirectory();
    void editDirectory();
    void removeDirectory();
    void updateOptionButtons();

private:
    QString m_name;
    QStringList m_directories;
    QStringList m_filters;
    // Our config dialog, uses in addDirectory and editDirectory
    // to give their dialogs the right parent
    QDialog *m_dialog;
    Ui::DirectoryFilterOptions m_ui;
    mutable QMutex m_lock;
};

} // namespace Internal
} // namespace QuickOpen

#endif // DIRECTORYFILTER_H
