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

#include "simplecolorpalettesingleton.h"
#include "simplecolorpalette.h"

#include "designersettings.h"

#include <QDebug>
#include <QSettings>

namespace QmlDesigner {

SimpleColorPaletteSingleton::SimpleColorPaletteSingleton()
    : m_items()
    , m_favoriteOffset(0)
{
    if (!readPalette()) {
        for (int i = 0; i < m_paletteSize; i++)
            m_items.append(PaletteColor());
    }
}

SimpleColorPaletteSingleton &SimpleColorPaletteSingleton::getInstance()
{
    static SimpleColorPaletteSingleton singleton;

    return singleton;
}

void SimpleColorPaletteSingleton::addItem(const PaletteColor &item)
{
    if (m_favoriteOffset >= m_paletteSize)
        return;

    if (item.isFavorite()) {
        int contains = m_items.indexOf(item);
        if (contains != -1) {
            if (m_items.at(contains).isFavorite())
                return;
            else
                m_items.removeAt(contains);
        }
        m_items.insert(0, item);
        m_favoriteOffset++;
    } else if (m_items.contains(item))
        return;
    else
        m_items.insert(m_favoriteOffset, item);

    while (m_items.size() > m_paletteSize) {
        m_items.removeLast();
    }

    writePalette();

    emit paletteChanged();
}

QList<PaletteColor> SimpleColorPaletteSingleton::getItems() const
{
    return m_items;
}

int SimpleColorPaletteSingleton::getPaletteSize() const
{
    return m_paletteSize;
}

int SimpleColorPaletteSingleton::getFavoriteOffset() const
{
    return m_favoriteOffset;
}

void SimpleColorPaletteSingleton::sortItems()
{
    auto itemSort = [](const PaletteColor &first, const PaletteColor &second) {
        return (static_cast<int>(first.isFavorite()) < static_cast<int>(second.isFavorite()));
    };

    std::sort(m_items.begin(), m_items.end(), itemSort);

    emit paletteChanged();
}

void SimpleColorPaletteSingleton::toggleFavorite(int id)
{
    bool toggleResult = m_items[id].toggleFavorite();

    if (toggleResult) {
        m_favoriteOffset++;
        m_items.move(id, 0);
    } else {
        m_favoriteOffset--;
        m_items.move(id, m_favoriteOffset);
    }

    if (m_favoriteOffset < 0)
        m_favoriteOffset = 0;
    else if (m_favoriteOffset > m_paletteSize)
        m_favoriteOffset = m_paletteSize;

    emit paletteChanged();
}

bool SimpleColorPaletteSingleton::readPalette()
{
    QList<PaletteColor> proxy;
    const QStringList stringData = QmlDesigner::DesignerSettings::getValue(
                                       QmlDesigner::DesignerSettingsKey::SIMPLE_COLOR_PALETTE_CONTENT)
                                       .toStringList();

    int favCounter = 0;

    for (int i = 0; i < stringData.size(); i++) {
        const QStringList strsep = stringData.at(i).split(";");
        if (strsep.size() != 2) {
            continue;
        }
        PaletteColor colorItem(strsep.at(0));
        bool isFav = static_cast<bool>(strsep.at(1).toInt());
        colorItem.setFavorite(isFav);
        if (isFav)
            favCounter++;
        proxy.append(colorItem);
    }

    if (proxy.size() == 0) {
        return false;
    }

    while (proxy.size() > m_paletteSize) {
        proxy.removeLast();
    }
    while (proxy.size() < m_paletteSize) {
        proxy.append(PaletteColor());
    }

    m_items.clear();
    m_items = proxy;
    m_favoriteOffset = favCounter;

    return true;
}

void SimpleColorPaletteSingleton::writePalette()
{
    QStringList output;
    QString subres;

    for (int i = 0; i < m_items.size(); i++) {
        subres = m_items.at(i).color().name(QColor::HexArgb);
        subres += ";";
        subres += QString::number(static_cast<int>(m_items.at(i).isFavorite()));
        output.push_back(subres);
        subres.clear();
    }

    QmlDesigner::DesignerSettings::setValue(
        QmlDesigner::DesignerSettingsKey::SIMPLE_COLOR_PALETTE_CONTENT, output);
}

} // namespace QmlDesigner
