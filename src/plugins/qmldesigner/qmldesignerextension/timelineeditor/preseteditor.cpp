/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "preseteditor.h"

#include "canvas.h"
#include "easingcurve.h"
#include "timelineicons.h"

#include <QAbstractButton>
#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QStandardItemModel>
#include <QString>

#include <coreplugin/icore.h>
#include <theme.h>

namespace QmlDesigner {

constexpr int iconWidth = 86;
constexpr int iconHeight = 86;
constexpr int itemFrame = 3;
constexpr int itemWidth = iconWidth + 2 * itemFrame;
constexpr int unsavedMarkSize = 18;

constexpr int spacingg = 5;

const QColor background = Qt::white;

const QColor labelBackground = qRgb(0x70, 0x70, 0x70);
const QColor canvasBackground = qRgb(0x46, 0x46, 0x46);
const QColor curveLine = qRgb(0xe6, 0xe7, 0xe8);

PresetItemDelegate::PresetItemDelegate() = default;

void PresetItemDelegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &opt,
                               const QModelIndex &index) const
{
    QStyleOptionViewItem option = opt;
    initStyleOption(&option, index);

    auto *w = option.widget;
    auto *style = w == nullptr ? qApp->style() : w->style();

    QSize textSize = QSize(option.rect.width(),
                           style->subElementRect(QStyle::SE_ItemViewItemText, &option, w).height());

    auto textRect = QRect(option.rect.topLeft(), textSize);
    textRect.moveBottom(option.rect.bottom());

    option.font.setPixelSize(Theme::instance()->smallFontPixelSize());

    painter->save();
    painter->fillRect(option.rect, canvasBackground);

    if (option.text.isEmpty())
        painter->fillRect(textRect, canvasBackground);
    else
        painter->fillRect(textRect, Theme::instance()->qmlDesignerButtonColor());

    style->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);

    QVariant dirty = option.index.data(PresetList::ItemRole_Dirty);
    if (dirty.isValid()) {
        if (dirty.toBool()) {
            QRect asteriskRect(option.rect.right() - unsavedMarkSize,
                               itemFrame,
                               unsavedMarkSize,
                               unsavedMarkSize);

            QFont font = painter->font();
            font.setPixelSize(unsavedMarkSize);
            painter->setFont(font);

            auto pen = painter->pen();
            pen.setColor(Qt::white);
            painter->setPen(pen);

            painter->drawText(asteriskRect, Qt::AlignTop | Qt::AlignRight, "*");
        }
    }
    painter->restore();
}

QSize PresetItemDelegate::sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(opt, index);
    size.rwidth() = itemWidth;
    return size;
}

QIcon paintPreview()
{
    QPixmap pm(iconWidth, iconHeight);
    pm.fill(canvasBackground);
    return QIcon(pm);
}

QIcon paintPreview(const EasingCurve &curve)
{
    QPixmap pm(iconWidth, iconHeight);
    pm.fill(canvasBackground);

    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);

    Canvas canvas(iconWidth, iconHeight, 2, 2, 9, 6, 0, 1);
    canvas.paintCurve(&painter, curve, curveLine);

    return QIcon(pm);
}

namespace Internal {

static const char settingsKey[] = "EasingCurveList";
static const char settingsFileName[] = "/EasingCurves.ini";

QString settingsFullFilePath(const QSettings::Scope &scope)
{
    if (scope == QSettings::SystemScope)
        return Core::ICore::installerResourcePath() + settingsFileName;

    return Core::ICore::userResourcePath() + settingsFileName;
}

} // namespace Internal

PresetList::PresetList(QSettings::Scope scope, QWidget *parent)
    : QListView(parent)
    , m_scope(scope)
    , m_index(-1)
    , m_filename(Internal::settingsFullFilePath(scope))
{
    int magic = 4;
    int scrollBarWidth = this->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    const int width = 3 * itemWidth + 4 * spacingg + scrollBarWidth + magic;

    setFixedWidth(width);

    setModel(new QStandardItemModel);

    setItemDelegate(new PresetItemDelegate);

    setSpacing(spacingg);

    setUniformItemSizes(true);

    setIconSize(QSize(iconWidth, iconHeight));

    setSelectionMode(QAbstractItemView::SingleSelection);

    setViewMode(QListView::IconMode);

    setFlow(QListView::LeftToRight);

    setMovement(QListView::Static);

    setWrapping(true);

    setTextElideMode(Qt::ElideMiddle);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

void PresetList::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    for (const QModelIndex &index : deselected.indexes()) {
        if (dirty(index)) {
            QMessageBox msgBox;
            msgBox.setText("The preset has been modified.");
            msgBox.setInformativeText("Do you want to save your changes?");
            msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard
                                      | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Save);

            if (QAbstractButton *button = msgBox.button(QMessageBox::Discard))
                button->setText("Discard Changes");

            if (QAbstractButton *button = msgBox.button(QMessageBox::Cancel))
                button->setText("Cancel Selection");

            int ret = msgBox.exec();

            switch (ret) {
            case QMessageBox::Save:
                // Save the preset and continue selection.
                writePresets();
                break;
            case QMessageBox::Discard:
                // Discard changes to the curve and continue selection.
                revert(index);
                break;

            case QMessageBox::Cancel:
                // Cancel selection operation and leave the curve untouched.
                selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
                return;

            default:
                // should never be reachedDiscard
                break;
            }
        }
    }

    for (const auto &index : selected.indexes()) {
        QVariant curveData = model()->data(index, ItemRole_Data);
        if (curveData.isValid())
            emit presetChanged(curveData.value<EasingCurve>());
    }
}

bool PresetList::hasSelection() const
{
    return selectionModel()->hasSelection();
}

bool PresetList::dirty(const QModelIndex &index) const
{
    return model()->data(index, ItemRole_Dirty).toBool();
}

int PresetList::index() const
{
    return m_index;
}

bool PresetList::isEditable(const QModelIndex &index) const
{
    QFlags<Qt::ItemFlag> flags(model()->flags(index));
    return flags.testFlag(Qt::ItemIsEditable);
}

void PresetList::initialize(int index)
{
    m_index = index;

    readPresets();
}

void PresetList::readPresets()
{
    auto *simodel = qobject_cast<QStandardItemModel *>(model());

    simodel->clear();

    QList<NamedEasingCurve> curves = storedCurves();

    for (int i = 0; i < curves.size(); ++i) {
        QVariant curveData = QVariant::fromValue(curves[i].curve());

        auto *item = new QStandardItem(paintPreview(curves[i].curve()), curves[i].name());
        item->setData(curveData, ItemRole_Data);
        item->setEditable(m_scope == QSettings::UserScope);
        item->setToolTip(curves[i].name());

        simodel->setItem(i, item);
    }
}

void PresetList::writePresets()
{
    QList<QVariant> presets;
    for (int i = 0; i < model()->rowCount(); ++i) {
        QModelIndex index = model()->index(i, 0);

        QVariant nameData = model()->data(index, Qt::DisplayRole);
        QVariant curveData = model()->data(index, ItemRole_Data);

        if (nameData.isValid() && curveData.isValid()) {
            NamedEasingCurve curve(nameData.toString(), curveData.value<QmlDesigner::EasingCurve>());

            presets << QVariant::fromValue(curve);
        }

        model()->setData(index, false, ItemRole_Dirty);
    }

    QSettings settings(m_filename, QSettings::IniFormat);
    settings.clear();
    settings.setValue(Internal::settingsKey, QVariant::fromValue(presets));
}

void PresetList::revert(const QModelIndex &index)
{
    auto *simodel = qobject_cast<QStandardItemModel *>(model());
    if (auto *item = simodel->itemFromIndex(index)) {
        QString name = item->data(Qt::DisplayRole).toString();
        QList<NamedEasingCurve> curves = storedCurves();

        for (const auto &curve : curves) {
            if (curve.name() == name) {
                item->setData(false, ItemRole_Dirty);
                item->setData(paintPreview(curve.curve()), Qt::DecorationRole);
                item->setData(QVariant::fromValue(curve.curve()), ItemRole_Data);
                item->setToolTip(name);
                return;
            }
        }
    }
}

void PresetList::updateCurve(const EasingCurve &curve)
{
    if (!selectionModel()->hasSelection())
        return;

    QVariant icon = QVariant::fromValue(paintPreview(curve));
    QVariant curveData = QVariant::fromValue(curve);

    for (const auto &index : selectionModel()->selectedIndexes())
        setItemData(index, curveData, icon);
}

void PresetList::contextMenuEvent(QContextMenuEvent *event)
{
    event->accept();

    if (m_scope == QSettings::SystemScope)
        return;

    QMenu menu;

    QAction *addAction = menu.addAction(tr("Add Preset"));

    connect(addAction, &QAction::triggered, [&]() { createItem(); });

    if (selectionModel()->hasSelection()) {
        QAction *removeAction = menu.addAction(tr("Delete Selected Preset"));
        connect(removeAction, &QAction::triggered, [&]() { removeSelectedItem(); });
    }

    menu.exec(event->globalPos());
}

void PresetList::dataChanged(const QModelIndex &topLeft,
                             const QModelIndex &bottomRight,
                             const QVector<int> &roles)
{
    if (topLeft == bottomRight && roles.contains(0)) {
        const QVariant name = model()->data(topLeft, 0);
        model()->setData(topLeft, name, Qt::ToolTipRole);
    }
}

void PresetList::createItem()
{
    EasingCurve curve;
    curve.makeDefault();
    createItem(createUniqueName(), curve);
}

void PresetList::createItem(const QString &name, const EasingCurve &curve)
{
    auto *item = new QStandardItem(paintPreview(curve), name);
    item->setData(QVariant::fromValue(curve), ItemRole_Data);
    item->setToolTip(name);

    int row = model()->rowCount();
    qobject_cast<QStandardItemModel *>(model())->setItem(row, item);

    QModelIndex index = model()->index(row, 0);

    // Why is that needed? SingleSelection is specified.
    selectionModel()->clear();
    selectionModel()->select(index, QItemSelectionModel::Select);
}

void PresetList::removeSelectedItem()
{
    for (const auto &index : selectionModel()->selectedIndexes())
        model()->removeRow(index.row());

    writePresets();
}

void PresetList::setItemData(const QModelIndex &index, const QVariant &curve, const QVariant &icon)
{
    if (isEditable(index)) {
        model()->setData(index, curve, PresetList::ItemRole_Data);
        model()->setData(index, true, PresetList::ItemRole_Dirty);
        model()->setData(index, icon, Qt::DecorationRole);
    }
}

QString PresetList::createUniqueName() const
{
    QStringList names = allNames();
    auto nameIsUnique = [&](const QString &name) {
        auto iter = std::find(names.begin(), names.end(), name);
        if (iter == names.end())
            return true;
        else
            return false;
    };

    int counter = 0;
    QString tmp("Default");
    QString name = tmp;

    while (!nameIsUnique(name))
        name = tmp + QString(" %1").arg(counter++);

    return name;
}

QStringList PresetList::allNames() const
{
    QStringList names;
    for (int i = 0; i < model()->rowCount(); ++i) {
        QModelIndex index = model()->index(i, 0);
        QVariant nameData = model()->data(index, Qt::DisplayRole);
        if (nameData.isValid())
            names << nameData.toString();
    }

    return names;
}

QList<NamedEasingCurve> PresetList::storedCurves() const
{
    QSettings settings(m_filename, QSettings::IniFormat);
    QVariant presetSettings = settings.value(Internal::settingsKey);

    if (!presetSettings.isValid())
        return QList<NamedEasingCurve>();

    QList<QVariant> presets = presetSettings.toList();

    QList<NamedEasingCurve> out;
    for (const QVariant &preset : presets)
        if (preset.isValid())
            out << preset.value<NamedEasingCurve>();

    return out;
}

PresetEditor::PresetEditor(QWidget *parent)
    : QStackedWidget(parent)
    , m_presets(new PresetList(QSettings::SystemScope))
    , m_customs(new PresetList(QSettings::UserScope))
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    addWidget(m_presets);
    addWidget(m_customs);

    connect(m_presets, &PresetList::presetChanged, this, &PresetEditor::presetChanged);
    connect(m_customs, &PresetList::presetChanged, this, &PresetEditor::presetChanged);
}

void PresetEditor::initialize(QTabBar *bar)
{
    m_presets->initialize(bar->addTab("Presets"));
    m_customs->initialize(bar->addTab("Custom"));

    connect(bar, &QTabBar::currentChanged, this, &PresetEditor::activate);
    connect(this, &PresetEditor::currentChanged, bar, &QTabBar::setCurrentIndex);

    m_presets->selectionModel()->clear();
    m_customs->selectionModel()->clear();

    activate(m_presets->index());
}

void PresetEditor::activate(int id)
{
    if (id == m_presets->index())
        setCurrentWidget(m_presets);
    else
        setCurrentWidget(m_customs);
}

void PresetEditor::update(const EasingCurve &curve)
{
    if (isCurrent(m_presets))
        m_presets->selectionModel()->clear();
    else {
        if (m_customs->selectionModel()->hasSelection()) {
            QVariant icon = QVariant::fromValue(paintPreview(curve));
            QVariant curveData = QVariant::fromValue(curve);
            for (const QModelIndex &index : m_customs->selectionModel()->selectedIndexes())
                m_customs->setItemData(index, curveData, icon);
        }
    }
}

bool PresetEditor::writePresets(const EasingCurve &curve)
{
    if (!curve.isLegal()) {
        QMessageBox msgBox;
        msgBox.setText("Attempting to save invalid curve");
        msgBox.setInformativeText("Please solve the issue before proceeding.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        return false;
    }

    if (auto current = qobject_cast<const PresetList *>(currentWidget())) {
        if (current->index() == m_presets->index()
            || (current->index() == m_customs->index() && !m_customs->hasSelection())) {
            bool ok;
            QString name = QInputDialog::getText(this,
                                                 tr("Save Preset"),
                                                 tr("Name"),
                                                 QLineEdit::Normal,
                                                 QString(),
                                                 &ok);

            if (ok && !name.isEmpty()) {
                activate(m_customs->index());
                m_customs->createItem(name, curve);
            }
        }

        m_customs->writePresets();
        return true;
    }

    return false;
}

bool PresetEditor::isCurrent(PresetList *list)
{
    if (auto current = qobject_cast<const PresetList *>(currentWidget()))
        return list->index() == current->index();

    return false;
}

} // namespace QmlDesigner
