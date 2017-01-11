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

#include "projectexplorer_export.h"

#include "kit.h"

#include <QWidget>

#include <functional>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace Core { class Id; }

namespace ProjectExplorer {

// Let the user pick a kit.
class PROJECTEXPLORER_EXPORT KitChooser : public QWidget
{
    Q_OBJECT

public:
    explicit KitChooser(QWidget *parent = nullptr);

    void setCurrentKitId(Core::Id id);
    Core::Id currentKitId() const;

    void setKitPredicate(const Kit::Predicate &predicate);

    Kit *currentKit() const;

signals:
    void currentIndexChanged(int);
    void activated(int);

public slots:
    void populate();

protected:
    virtual QString kitText(const Kit *k) const;
    virtual QString kitToolTip(Kit *k) const;

private:
    void onCurrentIndexChanged(int index);
    void onManageButtonClicked();
    Kit *kitAt(int index) const;

    Kit::Predicate m_kitPredicate;
    QComboBox *m_chooser;
    QPushButton *m_manageButton;
};

} // namespace ProjectExplorer
