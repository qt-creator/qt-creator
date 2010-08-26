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

#ifndef REMOTEHELPFILTER_H
#define REMOTEHELPFILTER_H

#include "ui_remotehelpfilter.h"

#include <locator/ilocatorfilter.h>

#include <QtGui/QIcon>

namespace Help {
    namespace Internal {

class RemoteHelpFilter : public Locator::ILocatorFilter
{
    Q_OBJECT
public:
    RemoteHelpFilter();
    ~RemoteHelpFilter();

    // ILocatorFilter
    QString displayName() const;
    QString id() const;
    Priority priority() const;
    QList<Locator::FilterEntry> matchesFor(const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);
    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);
    bool openConfigDialog(QWidget *parent, bool &needsRefresh);

    QStringList remoteUrls() const { return m_remoteUrls; }

signals:
    void linkActivated(const QUrl &url) const;

private:
    QIcon m_icon;
    QStringList m_remoteUrls;
};

class RemoteFilterOptions : public QDialog
{
    Q_OBJECT
    friend class RemoteHelpFilter;

public:
    explicit RemoteFilterOptions(RemoteHelpFilter *filter, QWidget *parent = 0);

private slots:
    void addNewItem();
    void removeItem();

private:
    RemoteHelpFilter *m_filter;
    Ui::RemoteFilterOptions m_ui;
};

    } // namespace Internal
} // namespace Help

#endif // REMOTEHELPFILTER_H
