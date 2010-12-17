/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

namespace Locator {
namespace Internal {

class DirectoryFilter : public BaseFileFilter
{
    Q_OBJECT

public:
    DirectoryFilter();
    QString displayName() const { return m_name; }
    QString id() const { return m_name; }
    Locator::ILocatorFilter::Priority priority() const { return Locator::ILocatorFilter::Medium; }
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
} // namespace Locator

#endif // DIRECTORYFILTER_H
