/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
    void addConfigWidget(ProjectExplorer::KitConfigWidget *widget);
    void makeStickySubWidgetsReadOnly();

    Kit *workingCopy() const;
    bool configures(ProjectExplorer::Kit *k) const;
    void setIsDefaultKit(bool d);
    bool isDefaultKit() const;
    void removeKit();
    void updateVisibility();

signals:
    void dirty();

private slots:
    void setIcon();
    void setDisplayName();
    void workingCopyWasUpdated(ProjectExplorer::Kit *k);
    void kitWasUpdated(ProjectExplorer::Kit *k);

private:
    enum LayoutColumns {
        LabelColumn,
        WidgetColumn,
        ButtonColumn
    };

    QLabel *createLabel(const QString &name, const QString &toolTip);
    void paintEvent(QPaintEvent *ev);

    QGridLayout *m_layout;
    QToolButton *m_iconButton;
    QLineEdit *m_nameEdit;
    QList<KitConfigWidget *> m_widgets;
    QList<QLabel *> m_labels;
    Kit *m_kit;
    Kit *m_modifiedKit;
    bool m_isDefaultKit;
    bool m_fixingKit;
    QPixmap m_background;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // KITMANAGERCONFIGWIDGET_H
