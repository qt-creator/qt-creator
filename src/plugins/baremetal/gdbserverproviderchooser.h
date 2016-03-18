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

namespace Core { class Id; }

namespace BareMetal {
namespace Internal {

class GdbServerProvider;

class GdbServerProviderChooser : public QWidget
{
    Q_OBJECT

public:
    explicit GdbServerProviderChooser(
            bool useManageButton = true, QWidget *parent = 0);

    QString currentProviderId() const;
    void setCurrentProviderId(const QString &id);

signals:
    void providerChanged();

public slots:
    void populate();

private slots:
    void currentIndexChanged(int index);
    void manageButtonClicked();

protected:
    bool providerMatches(const GdbServerProvider *) const;
    QString providerText(const GdbServerProvider *) const;

private:
    QComboBox *m_chooser;
    QPushButton *m_manageButton;
};

} // namespace Internal
} // namespace BareMetal
