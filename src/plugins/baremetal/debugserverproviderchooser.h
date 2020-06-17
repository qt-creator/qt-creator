/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {

class IDebugServerProvider;

// DebugServerProviderChooser

class DebugServerProviderChooser final : public QWidget
{
    Q_OBJECT

public:
    explicit DebugServerProviderChooser(
            bool useManageButton = true, QWidget *parent = nullptr);

    QString currentProviderId() const;
    void setCurrentProviderId(const QString &id);
    void populate();

signals:
    void providerChanged();

private:
    void currentIndexChanged(int index);
    void manageButtonClicked();
    bool providerMatches(const IDebugServerProvider *) const;
    QString providerText(const IDebugServerProvider *) const;

    QComboBox *m_chooser = nullptr;
    QPushButton *m_manageButton = nullptr;
};

} // namespace Internal
} // namespace BareMetal
