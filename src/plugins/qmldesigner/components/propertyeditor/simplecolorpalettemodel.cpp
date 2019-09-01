/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "simplecolorpalettemodel.h"
#include "simplecolorpalette.h"
#include "simplecolorpalettesingleton.h"

#include "designersettings.h"

#include <coreplugin/icore.h>

#include <QColorDialog>
#include <QHash>
#include <QByteArray>
#include <QDebug>
#include <QSettings>
#include <QTimer>

namespace QmlDesigner {

SimpleColorPaletteModel::SimpleColorPaletteModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&SimpleColorPaletteSingleton::getInstance(),
            &SimpleColorPaletteSingleton::paletteChanged,
            this,
            &SimpleColorPaletteModel::setPalette);
    m_roleNames = {{static_cast<int>(PaletteColor::Property::objectNameRole), "objectName"},
                   {static_cast<int>(PaletteColor::Property::colorRole), "color"},
                   {static_cast<int>(PaletteColor::Property::colorCodeRole), "colorCode"},
                   {static_cast<int>(PaletteColor::Property::isFavoriteRole), "isFavorite"}};

    setPalette();
}

SimpleColorPaletteModel::~SimpleColorPaletteModel()
{
    clearItems();
}

int SimpleColorPaletteModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_items.count();
}

QVariant SimpleColorPaletteModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && (index.row() >= 0) && (index.row() < m_items.count())) {
        if (m_roleNames.contains(role)) {
            QVariant value = m_items.at(index.row())
                                 .getProperty(static_cast<PaletteColor::Property>(role));
            if (auto model = qobject_cast<SimpleColorPaletteModel *>(value.value<QObject *>()))
                return QVariant::fromValue(model);

            return value;
        }

        qWarning() << Q_FUNC_INFO << "invalid role requested";
        return QVariant();
    }

    qWarning() << Q_FUNC_INFO << "invalid index requested";
    return QVariant();
}

QHash<int, QByteArray> SimpleColorPaletteModel::roleNames() const
{
    return m_roleNames;
}

void SimpleColorPaletteModel::clearItems()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

void SimpleColorPaletteModel::addItem(const QString &item)
{
    PaletteColor palette(item);
    addItem(palette);
}

void SimpleColorPaletteModel::addItem(const PaletteColor &item)
{
    SimpleColorPaletteSingleton::getInstance().addItem(item);
}

const QList<PaletteColor> &SimpleColorPaletteModel::items() const
{
    return m_items;
}

void SimpleColorPaletteModel::sortItems()
{
    SimpleColorPaletteSingleton::getInstance().sortItems();
}

void SimpleColorPaletteModel::registerDeclarativeType()
{
    qmlRegisterType<SimpleColorPaletteModel>("HelperWidgets", 2, 0, "SimpleColorPaletteModel");
}

void SimpleColorPaletteModel::toggleFavorite(int id)
{
    SimpleColorPaletteSingleton::getInstance().toggleFavorite(id);
}

void SimpleColorPaletteModel::setPalette()
{
    beginResetModel();
    m_items = SimpleColorPaletteSingleton::getInstance().getItems();
    m_favoriteOffset = SimpleColorPaletteSingleton::getInstance().getFavoriteOffset();
    m_paletteSize = SimpleColorPaletteSingleton::getInstance().getPaletteSize();
    endResetModel();
}

bool SimpleColorPaletteModel::read()
{
    return SimpleColorPaletteSingleton::getInstance().readPalette();
}

void SimpleColorPaletteModel::write()
{
    SimpleColorPaletteSingleton::getInstance().writePalette();
}

void SimpleColorPaletteModel::showDialog(QColor color)
{
    auto colorDialog = new QColorDialog(Core::ICore::dialogParent());
    colorDialog->setCurrentColor(color);
    colorDialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(colorDialog, &QDialog::rejected, this, &SimpleColorPaletteModel::colorDialogRejected);
    connect(colorDialog, &QColorDialog::currentColorChanged, this, &SimpleColorPaletteModel::currentColorChanged);

    QTimer::singleShot(0, [colorDialog](){ colorDialog->exec(); });
}

} // namespace QmlDesigner
