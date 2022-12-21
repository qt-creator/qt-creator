// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "kit.h"

#include <QWidget>

#include <functional>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {

// Let the user pick a kit.
class PROJECTEXPLORER_EXPORT KitChooser : public QWidget
{
    Q_OBJECT

public:
    explicit KitChooser(QWidget *parent = nullptr);

    void setCurrentKitId(Utils::Id id);
    Utils::Id currentKitId() const;

    void setKitPredicate(const Kit::Predicate &predicate);
    void setShowIcons(bool showIcons);

    Kit *currentKit() const;
    bool hasStartupKit() const { return m_hasStartupKit; }

signals:
    void currentIndexChanged();
    void activated();

public slots:
    void populate();

protected:
    virtual QString kitText(const Kit *k) const;
    virtual QString kitToolTip(Kit *k) const;

private:
    void onActivated();
    void onCurrentIndexChanged();
    void onManageButtonClicked();

    Kit::Predicate m_kitPredicate;
    QComboBox *m_chooser;
    QPushButton *m_manageButton;
    bool m_hasStartupKit = false;
    bool m_showIcons = false;
};

} // namespace ProjectExplorer
