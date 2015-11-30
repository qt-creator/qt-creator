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

#include "themesettingsitemdelegate.h"

#include "colorvariable.h"
#include "themesettingstablemodel.h"
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QAbstractProxyModel>
#include <QComboBox>
#include <QEvent>
#include <QInputDialog>
#include <QMetaEnum>

using namespace Utils;

static QAbstractItemModel *sourceModel(QAbstractItemModel *model)
{
    if (QAbstractProxyModel *m = qobject_cast<QAbstractProxyModel *>(model))
        return m->sourceModel();
    return model;
}

static const QAbstractItemModel *sourceModel(const QAbstractItemModel *model)
{
    if (const QAbstractProxyModel *m = qobject_cast<const QAbstractProxyModel *>(model))
        return m->sourceModel();
    return model;
}

static QIcon makeIcon(const QColor &color)
{
    QImage img(QSize(24,24), QImage::Format_ARGB32);
    img.fill(color.rgba());
    QIcon ico = QIcon(QPixmap::fromImage(img));
    return ico;
}

namespace Core {
namespace Internal {
namespace ThemeEditor {

ThemeSettingsItemDelegate::ThemeSettingsItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent),
      m_comboBox(0)
{
}

QWidget *ThemeSettingsItemDelegate::createColorEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const ThemeSettingsTableModel *model = qobject_cast<const ThemeSettingsTableModel*>(sourceModel(index.model()));

    Q_UNUSED(option);
    const int row = model->modelToSectionRow(index.row());
    QComboBox *cb = new QComboBox(parent);
    ColorRole::Ptr colorRole = model->m_colors->colorRole(row);

    const bool   isUnnamed    = colorRole->colorVariable()->variableName().isEmpty();
    const QColor currentColor = colorRole->colorVariable()->color();

    cb->addItem(makeIcon(currentColor),
                isUnnamed ? tr("<Unnamed> (Current)")
                          : colorRole->colorVariable()->variableName() + tr(" (Current)"));
    int k = 1;

    foreach (ColorVariable::Ptr namedColor, model->m_colors->colorVariables()) {
        if (namedColor->variableName().isEmpty())
            continue;
        if (colorRole->colorVariable() == namedColor) {
            continue;
        } else {
            cb->addItem(makeIcon(namedColor->color()), namedColor->variableName());
            m_actions[k++] = qMakePair(Action_ChooseNamedColor, namedColor);
        }
    }

    if (!isUnnamed) {
        cb->addItem(tr("Remove Variable Name"));
        m_actions[k++] = qMakePair(Action_MakeUnnamed, QSharedPointer<ColorVariable>(0));
    }
    cb->addItem(tr("Add Variable Name..."));
    m_actions[k++] = qMakePair(Action_CreateNew, QSharedPointer<ColorVariable>(0));

    connect(cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, [this, cb]() {
        ThemeSettingsItemDelegate *me = const_cast<ThemeSettingsItemDelegate *>(this);
        emit me->commitData(cb);
        emit me->closeEditor(cb);
    });

    m_comboBox = cb;
    return cb;
}

QWidget *ThemeSettingsItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const ThemeSettingsTableModel *model = qobject_cast<const ThemeSettingsTableModel*>(sourceModel(index.model()));

    const int section = model->inSectionBody(index.row());
    QTC_ASSERT(section >= 0, return 0);

    switch (section) {
    case ThemeSettingsTableModel::SectionWidgetStyle: {
        QComboBox *cb = new QComboBox(parent);
        QMetaEnum e = Theme::staticMetaObject.enumerator(Theme::staticMetaObject.indexOfEnumerator("WidgetStyle"));
        for (int i = 0, total = e.keyCount(); i < total; ++i)
            cb->addItem(QLatin1String(e.key(i)));
        cb->setCurrentIndex(model->m_widgetStyle);
        connect(cb, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
                this, [this, cb]() {
            ThemeSettingsItemDelegate *me = const_cast<ThemeSettingsItemDelegate *>(this);
            emit me->commitData(cb);
            emit me->closeEditor(cb);
        });
        m_comboBox = cb;
        return cb;
    }
    case ThemeSettingsTableModel::SectionColors:
        return createColorEditor(parent, option, index);
    case ThemeSettingsTableModel::SectionFlags:
        return QStyledItemDelegate::createEditor(parent, option, index);
    default:
        qWarning("unhandled section");
        return 0;
    } // switch
}

void ThemeSettingsItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QStyledItemDelegate::setEditorData(editor, index);
}

void ThemeSettingsItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ThemeSettingsTableModel *themeSettingsModel = qobject_cast<ThemeSettingsTableModel *>(sourceModel(model));

    const int row = themeSettingsModel->modelToSectionRow(index.row());
    const int section = themeSettingsModel->inSectionBody(index.row());

    switch (section) {
    case ThemeSettingsTableModel::SectionWidgetStyle:
        if (QComboBox *cb = qobject_cast<QComboBox *>(editor))
            themeSettingsModel->m_widgetStyle = static_cast<Theme::WidgetStyle>(cb->currentIndex());
        return;
    case ThemeSettingsTableModel::SectionColors: {
        if (QComboBox *cb = qobject_cast<QComboBox *>(editor)) {
            ColorRole::Ptr themeColor = themeSettingsModel->m_colors->colorRole(row);

            Action act = m_actions[cb->currentIndex()].first;
            ColorVariable::Ptr previousVariable = themeColor->colorVariable();
            ColorVariable::Ptr newVariable = m_actions[cb->currentIndex()].second;

            if (act == Action_NoAction) {
                return;
            } else if (act == Action_ChooseNamedColor) {
                previousVariable->removeReference(themeColor.data());
                QTC_ASSERT(newVariable, return);
                themeColor->assignColorVariable(newVariable);
            } else if (act == Action_MakeUnnamed) {
                previousVariable->removeReference(themeColor.data());
                if (previousVariable->references().size() == 0)
                    themeSettingsModel->m_colors->removeVariable(previousVariable);
                ColorVariable::Ptr anonymousColor = themeSettingsModel->m_colors->createVariable(previousVariable->color());
                themeColor->assignColorVariable(anonymousColor);
            } else if (act == Action_CreateNew) {
                QString name = QInputDialog::getText(editor, tr("Add Variable Name"), tr("Variable name:"));
                if (!name.isEmpty()) {
                    previousVariable->removeReference(themeColor.data());

                    // TODO: check for name collision
                    ColorVariable::Ptr newVariable = themeSettingsModel->m_colors->createVariable(previousVariable->color(), name);

                    themeColor->assignColorVariable(newVariable);
                }
            }
        }
        return;
    }
    default:
        return QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void ThemeSettingsItemDelegate::popupMenu()
{
    m_comboBox->showPopup();
}

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core
