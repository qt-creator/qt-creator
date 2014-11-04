/****************************************************************************
**
** Copyright (C) 2015 Thorben Kroeger <thorbenkroeger@gmail.com>.
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

#ifndef THEMESETTINGSTABLEMODEL_H
#define THEMESETTINGSTABLEMODEL_H

#include <QAbstractTableModel>

#include "themecolors.h"
#include "sectionedtablemodel.h"
#include <utils/theme/theme.h>

namespace Core {
namespace Internal {
namespace ThemeEditor {

class ThemeSettingsTableModel : public SectionedTableModel
{
    Q_OBJECT

public:
    friend class ThemeSettingsItemDelegate;

    enum Section {
        SectionWidgetStyle,
        SectionColors,
        SectionFlags,
        SectionInvalid // end
    };

    ThemeSettingsTableModel(QObject *parent = 0);

    bool setData(const QModelIndex &index, const QVariant &value, int role) Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int sectionRowCount(int section) const Q_DECL_OVERRIDE;
    QVariant sectionBodyData(int section, int row, int column, int role) const Q_DECL_OVERRIDE;
    QVariant sectionHeaderData(int section, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags sectionBodyFlags(int section, int row, int column) const Q_DECL_OVERRIDE;
    Qt::ItemFlags sectionHeaderFlags(int section) const Q_DECL_OVERRIDE;
    int sectionCount() const Q_DECL_OVERRIDE;

    ThemeColors::Ptr colors() const { return m_colors; }

    bool hasChanges() const { return m_hasChanges; }

    void markEverythingChanged();

    void initFrom(Utils::Theme *theme);
    void toTheme(Utils::Theme *theme) const;

    QString m_name;
    QStringList m_preferredStyles;

public:
    ThemeColors::Ptr m_colors;
    QList<QPair<QString, bool> > m_flags;
    QList<QPair<QString, QString> > m_imageFiles;
    Utils::Theme::WidgetStyle m_widgetStyle;
    bool m_hasChanges;
};

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core

#endif // THEMESETTINGSTABLEMODEL_H
