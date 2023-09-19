// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zoomaction.h"
#include "formeditorwidget.h"
#include <algorithm>
#include <iterator>
#include <utility>

#include <utils/stylehelper.h>

#include <QAbstractItemView>
#include <QComboBox>
#include <QToolBar>

#include <cmath>

namespace QmlDesigner {

// Order matters!
std::array<double, 27> ZoomAction::m_zooms = {
    0.01, 0.02, 0.05, 0.0625, 0.1, 0.125, 0.2, 0.25, 0.33, 0.5,  0.66, 0.75, 0.9,
    1.0, 1.1, 1.25, 1.33, 1.5, 1.66, 1.75, 2.0, 3.0, 4.0, 6.0, 8.0, 10.0, 16.0
};

bool isValidIndex(int index)
{
    if (index >= 0 && index < static_cast<int>(ZoomAction::zoomLevels().size()))
        return true;

    return false;
}

ZoomAction::ZoomAction(QObject *parent)
    : QWidgetAction(parent)
    , m_combo(nullptr)
{
    m_index = indexOf(1.0);
}

std::array<double, 27> ZoomAction::zoomLevels()
{
    return m_zooms;
}

int ZoomAction::indexOf(double zoom)
{
    auto finder = [zoom](double val) { return qFuzzyCompare(val, zoom); };
    if (auto iter = std::find_if(m_zooms.begin(), m_zooms.end(), finder); iter != m_zooms.end())
        return static_cast<int>(std::distance(m_zooms.begin(), iter));

    return -1;
}

void ZoomAction::setZoomFactor(double zoom)
{
    if (int index = indexOf(zoom); index >= 0) {
        emitZoomLevelChanged(index);
        if (m_combo) {
            m_combo->setCurrentIndex(index);
            m_combo->setToolTip(m_combo->currentText());
        }
        m_index = index;
        return;
    }
    if (m_combo) {
        int rounded = static_cast<int>(std::round(zoom * 100));
        m_combo->setEditable(true);
        m_combo->setEditText(QString::number(rounded) + " %");
        m_combo->setToolTip(m_combo->currentText());
    }
}

double ZoomAction::setNextZoomFactor(double zoom)
{
    if (zoom >= m_zooms.back())
        return zoom;

    auto greater = [zoom](double val) { return val > zoom; };
    if (auto iter = std::find_if(m_zooms.begin(), m_zooms.end(), greater); iter != m_zooms.end()) {
        auto index = std::distance(m_zooms.begin(), iter);
        m_combo->setCurrentIndex(static_cast<int>(index));
        m_combo->setToolTip(m_combo->currentText());
        return *iter;
    }
    return zoom;
}

double ZoomAction::setPreviousZoomFactor(double zoom)
{
    if (zoom <= m_zooms.front())
        return zoom;

    auto smaller = [zoom](double val) { return val < zoom; };
    if (auto iter = std::find_if(m_zooms.rbegin(), m_zooms.rend(), smaller); iter != m_zooms.rend()) {
        auto index = std::distance(iter, m_zooms.rend() - 1);
        m_combo->setCurrentIndex(static_cast<int>(index));
        m_combo->setToolTip(m_combo->currentText());
        return *iter;
    }
    return zoom;
}

int ZoomAction::currentIndex() const
{
    return m_index;
}

bool parentIsToolBar(QWidget *parent)
{
    return qobject_cast<QToolBar *>(parent) != nullptr;
}

QComboBox *createZoomComboBox(QWidget *parent)
{
    auto *combo = new QComboBox(parent);
    for (double z : ZoomAction::zoomLevels()) {
        const QString name = QString::number(z * 100., 'g', 4) + " %";
        combo->addItem(name, z);
    }
    return combo;
}

QWidget *ZoomAction::createWidget(QWidget *parent)
{
    if (!m_combo && parentIsToolBar(parent)) {
        m_combo = createZoomComboBox(parent);
        m_combo->setProperty(Utils::StyleHelper::C_HIDE_BORDER, true);
        m_combo->setProperty(Utils::StyleHelper::C_TOOLBAR_ACTIONWIDGET, true);
        m_combo->setCurrentIndex(m_index);
        m_combo->setToolTip(m_combo->currentText());

        connect(m_combo, &QComboBox::currentIndexChanged, this, &ZoomAction::emitZoomLevelChanged);

        return m_combo.data();
    }
    return nullptr;
}

void ZoomAction::emitZoomLevelChanged(int index)
{
    if (index >= 0 && index < static_cast<int>(m_zooms.size()))
        emit zoomLevelChanged(m_zooms[static_cast<size_t>(index)]);
}

} // namespace QmlDesigner
