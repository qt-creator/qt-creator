/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef KITMANAGERCONFIGWIDGET_H
#define KITMANAGERCONFIGWIDGET_H

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
    ~KitManagerConfigWidget();

    QString displayName() const;

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

private slots:
    void setIcon();
    void setDisplayName();
    void setFileSystemFriendlyName();
    void workingCopyWasUpdated(ProjectExplorer::Kit *k);
    void kitWasUpdated(ProjectExplorer::Kit *k);

private:
    enum LayoutColumns {
        LabelColumn,
        WidgetColumn,
        ButtonColumn
    };

    void showEvent(QShowEvent *event);
    QLabel *createLabel(const QString &name, const QString &toolTip);
    void paintEvent(QPaintEvent *ev);

    QGridLayout *m_layout;
    QToolButton *m_iconButton;
    QLineEdit *m_nameEdit;
    QLineEdit *m_fileSystemFriendlyNameLineEdit;
    QList<KitConfigWidget *> m_widgets;
    QList<QLabel *> m_labels;
    Kit *m_kit;
    Kit *m_modifiedKit;
    bool m_isDefaultKit;
    bool m_fixingKit;
    bool m_hasUniqueName;
    QPixmap m_background;
    QList<QAction *> m_actions;
    mutable QString m_cachedDisplayName;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // KITMANAGERCONFIGWIDGET_H
