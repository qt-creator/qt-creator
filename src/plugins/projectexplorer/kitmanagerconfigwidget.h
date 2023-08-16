// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "kitmanager.h"

#include <QWidget>

#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QToolButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;

namespace Internal {

class KitManagerConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KitManagerConfigWidget(Kit *k, bool &isDefaultKit, bool &hasUniqueName);
    ~KitManagerConfigWidget() override;

    QString displayName() const;
    QIcon displayIcon() const;

    void apply();
    void discard();
    bool isDirty() const;
    QString validityMessage() const;
    void addAspectToWorkingCopy(Layouting::LayoutItem &parent, KitAspectFactory *factory);
    void makeStickySubWidgetsReadOnly();

    Kit *workingCopy() const;
    bool isRegistering() const { return m_isRegistering; }
    bool isDefaultKit() const;
    void updateVisibility();

signals:
    void dirty();
    void isAutoDetectedChanged();

private:
    void setIcon();
    void resetIcon();
    void setDisplayName();
    void setFileSystemFriendlyName();
    void workingCopyWasUpdated(ProjectExplorer::Kit *k);
    void kitWasUpdated(ProjectExplorer::Kit *k);

    enum LayoutColumns {
        LabelColumn,
        WidgetColumn,
        ButtonColumn
    };

    void showEvent(QShowEvent *event) override;

    QToolButton *m_iconButton;
    QLineEdit *m_nameEdit;
    QLineEdit *m_fileSystemFriendlyNameLineEdit;
    QList<KitAspect *> m_kitAspects;
    Kit *m_kit;
    std::unique_ptr<Kit> m_modifiedKit;
    bool &m_isDefaultKit;
    bool m_fixingKit = false;
    bool &m_hasUniqueName;
    bool m_isRegistering = false;
    mutable QString m_cachedDisplayName;
};

} // namespace Internal
} // namespace ProjectExplorer
