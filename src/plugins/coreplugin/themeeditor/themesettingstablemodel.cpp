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

#include "themesettingstablemodel.h"
#include "colorvariable.h"
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QApplication>
#include <QImage>
#include <QMetaEnum>
#include <QPainter>
#include <QPalette>

using namespace Utils;

namespace Core {
namespace Internal {
namespace ThemeEditor {

ThemeSettingsTableModel::ThemeSettingsTableModel(QObject *parent)
    : SectionedTableModel(parent),
      m_colors(new ThemeColors),
      m_hasChanges(false)
{
}

int ThemeSettingsTableModel::columnCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return 2;
}

int ThemeSettingsTableModel::sectionRowCount(int section) const
{
    switch (static_cast<Section>(section)) {
        case SectionWidgetStyle:  return 1;
        case SectionColors:       return m_colors->numColorRoles();
        case SectionFlags:        return m_flags.size();
        default:                  return 0;
    }
}

QVariant ThemeSettingsTableModel::sectionBodyData(int section, int row, int column, int role) const
{
    auto makeDecoration = [](const QColor &c) -> QImage {
        QImage img(QSize(32,32), QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        QPainter p(&img);
        p.fillRect(QRect(4,4,24,24), c);
        return img;
    };

    switch (static_cast<Section>(section)) {
    case SectionWidgetStyle: {
        if (role != Qt::DisplayRole)
            return QVariant();
        if (column == 0)
            return QLatin1String("WidgetStyle");
        else
            return m_widgetStyle == Theme::StyleFlat ? QLatin1String("StyleFlat") : QLatin1String("StyleDefault");
    }
    case SectionColors: {
        ColorRole::Ptr colorRole = m_colors->colorRole(row);
        if (column == 0 && role == Qt::DecorationRole)
            return QVariant::fromValue(makeDecoration(colorRole->colorVariable()->color()));
        if (role == Qt::DisplayRole) {
            if (column == 0)
                return colorRole->roleName();
            else
                return colorRole->colorVariable()->variableName();
        }
        return QVariant();
    }
    case SectionFlags: {
        if (column == 0 && role == Qt::DisplayRole)
            return m_flags[row].first;
        else if (column == 1 && role == Qt::CheckStateRole)
            return m_flags[row].second ? Qt::Checked : Qt::Unchecked;
        else if (column == 0 && role == Qt::DecorationRole)
            return QVariant::fromValue(makeDecoration(Qt::transparent));
        return QVariant();
    }
    default:
        return QVariant();
    }
}

QVariant ThemeSettingsTableModel::sectionHeaderData(int section, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (static_cast<Section>(section)) {
        case SectionWidgetStyle:  return tr("Widget Style");
        case SectionColors:       return tr("Colors");
        case SectionFlags:        return tr("Flags");
        default:                  return QString();
        }
    }
    if (role == Qt::FontRole) {
        QFont font;
        font.setPointSizeF(font.pointSizeF() * 1.25);
        font.setBold(true);
        return font;
    }
    if (role == Qt::SizeHintRole)
        return QSize(50, 50);
    return QVariant();
}

Qt::ItemFlags ThemeSettingsTableModel::sectionBodyFlags(int section, int row, int column) const
{
    Q_UNUSED(row);
    switch (static_cast<Section>(section)) {
    case SectionWidgetStyle:
        return (column == 0) ? Qt::ItemIsEnabled
                             : Qt::ItemIsEnabled | Qt::ItemIsEditable;
    case SectionColors:
        return (column == 0) ? Qt::ItemIsEnabled
                             : Qt::ItemIsEnabled | Qt::ItemIsEditable;
    case SectionFlags:
        return (column == 0) ? Qt::ItemIsEnabled
                             : Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    default: return Qt::ItemIsEnabled;
    }
}

bool ThemeSettingsTableModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    m_hasChanges = true;
    Q_UNUSED(role);

    int section = inSectionBody(idx.row());
    int row = modelToSectionRow(idx.row());
    switch (static_cast<Section>(section)) {
    case SectionFlags: {
        Qt::CheckState checkState = static_cast<Qt::CheckState>(value.toInt());
        bool checked = checkState == Qt::Checked;
        m_flags[row].second = checked;
        emit dataChanged(idx, idx);
        return true;
    }
    default: {
        // don't bother tracking changes, just mark the whole table as changed
        markEverythingChanged();
        return true;
    }
    } // switch
}

void ThemeSettingsTableModel::markEverythingChanged()
{
    m_hasChanges = true;
    QModelIndex i;
    emit dataChanged(index(0, 0, i), index(rowCount(i), columnCount(i), i));
}

void ThemeSettingsTableModel::initFrom(Theme *theme)
{
    const QMetaObject &metaObject = Theme::staticMetaObject;
    // Colors
    {
        QMetaEnum e = metaObject.enumerator(metaObject.indexOfEnumerator("Color"));
        QMap<QString, ColorVariable::Ptr> varLookup;
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            QPair<QColor, QString> c = theme->d->colors[static_cast<Theme::Color>(i)];
            if (c.second.isEmpty()) {
                ColorVariable::Ptr v = colors()->createVariable(c.first);
                colors()->createRole(key, v);
            } else if (varLookup.contains(c.second)) {
                colors()->createRole(key, varLookup[c.second]);
            } else {
                ColorVariable::Ptr v = colors()->createVariable(c.first, c.second);
                colors()->createRole(key, v);
                varLookup[c.second] = v;
            }
        }
    }
    // Flags
    {
        QMetaEnum e = metaObject.enumerator(metaObject.indexOfEnumerator("Flag"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            m_flags.append(qMakePair(key, theme->flag(static_cast<Theme::Flag>(i))));
        }
    }
    // ImageFiles
    {
        QMetaEnum e = metaObject.enumerator(metaObject.indexOfEnumerator("ImageFile"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            const QString key = QLatin1String(e.key(i));
            m_imageFiles.append(qMakePair(key, theme->imageFile(static_cast<Theme::ImageFile>(i), QString())));
        }
    }

    m_widgetStyle = theme->widgetStyle();
    m_name = theme->d->name;
    m_preferredStyles = theme->d->preferredStyles;
}

void ThemeSettingsTableModel::toTheme(Theme *t) const
{
    ThemePrivate *theme = t->d;
    // Colors
    {
        QMetaEnum e = Theme::staticMetaObject.enumerator(Theme::staticMetaObject.indexOfEnumerator("Color"));
        for (int i = 0, total = e.keyCount(); i < total; ++i) {
            ColorRole::Ptr role = colors()->colorRole(i);
            ColorVariable::Ptr var = role->colorVariable();
            theme->colors[i] = qMakePair(var->color(), var->variableName());
        }
    }
    // Flags
    {
        QTC_ASSERT(theme->flags.size() == m_flags.size(), return);
        for (int i = 0; i < theme->flags.size(); ++i)
            theme->flags[i] = m_flags[i].second;
    }
    // ImageFiles
    {
        const int nImageFiles = theme->imageFiles.size();
        for (int i = 0; i < nImageFiles; ++i)
            theme->imageFiles[i] = m_imageFiles.at(i).second;
    }

    theme->widgetStyle = m_widgetStyle;
    theme->name = m_name;
    theme->preferredStyles = m_preferredStyles;
}

Qt::ItemFlags ThemeSettingsTableModel::sectionHeaderFlags(int section) const
{
    Q_UNUSED(section);
    return Qt::ItemIsEnabled;
}

int ThemeSettingsTableModel::sectionCount() const
{
    return SectionInvalid;
}

QVariant ThemeSettingsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            if (section == 0)
                return tr("Role");
            return tr("Value");
        }
    }
    return QVariant();
}

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core
