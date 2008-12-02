/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
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

#include <coreplugin/icore.h>
#include <qtconcurrent/QtConcurrentTools>

namespace QuickOpen {
namespace Internal {

class DirectoryFilter : public BaseFileFilter
{
    Q_OBJECT

public:
    DirectoryFilter(Core::ICore *core);
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
