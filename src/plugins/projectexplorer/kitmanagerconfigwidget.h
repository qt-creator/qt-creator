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

#include "kitconfigwidget.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QGridLayout;
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
    explicit KitManagerConfigWidget(Kit *k);
    ~KitManagerConfigWidget() override;

    QString displayName() const;
    QIcon icon() const;

    void apply();
    void discard();
    bool isDirty() const;
    bool isValid() const;
    bool hasWarning() const;
    QString validityMessage() const;
    void addConfigWidget(KitConfigWidget *widget);
    void makeStickySubWidgetsReadOnly();

    Kit *workingCopy() const;
    bool configures(Kit *k) const;
    void setIsDefaultKit(bool d);
    bool isDefaultKit() const;
    void removeKit();
    void updateVisibility();

    void setHasUniqueName(bool unique);

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
    QLabel *createLabel(const QString &name, const QString &toolTip);

    QGridLayout *m_layout;
    QToolButton *m_iconButton;
    QLineEdit *m_nameEdit;
    QLineEdit *m_fileSystemFriendlyNameLineEdit;
    QList<KitConfigWidget *> m_widgets;
    QList<QLabel *> m_labels;
    Kit *m_kit;
    Kit *m_modifiedKit;
    bool m_isDefaultKit = false;
    bool m_fixingKit = false;
    bool m_hasUniqueName = true;
    QList<QAction *> m_actions;
    mutable QString m_cachedDisplayName;
};

} // namespace Internal
} // namespace ProjectExplorer
