/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "ui_remotehelpfilter.h"

#include <coreplugin/locator/ilocatorfilter.h>

#include <QIcon>
#include <QMutex>

namespace Help {
namespace Internal {

class RemoteHelpFilter : public Core::ILocatorFilter
{
    Q_OBJECT
public:
    RemoteHelpFilter();
    ~RemoteHelpFilter() final;

    // ILocatorFilter
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(Core::LocatorFilterEntry selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;
    void refresh(QFutureInterface<void> &future) override;
    QByteArray saveState() const override;
    void restoreState(const QByteArray &state) override;
    bool openConfigDialog(QWidget *parent, bool &needsRefresh) override;

    QStringList remoteUrls() const;

signals:
    void linkActivated(const QUrl &url) const;

private:
    QIcon m_icon;
    QStringList m_remoteUrls;
    mutable QMutex m_mutex;
};

class RemoteFilterOptions : public QDialog
{
    Q_OBJECT
    friend class RemoteHelpFilter;

public:
    explicit RemoteFilterOptions(RemoteHelpFilter *filter, QWidget *parent = nullptr);

private:
    void addNewItem();
    void removeItem();
    void moveItemUp();
    void moveItemDown();
    void updateActionButtons();

    RemoteHelpFilter *m_filter = nullptr;
    Ui::RemoteFilterOptions m_ui;
};

} // namespace Internal
} // namespace Help
