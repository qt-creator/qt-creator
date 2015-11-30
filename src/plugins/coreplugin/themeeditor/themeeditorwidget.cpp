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

#include "themeeditorwidget.h"
#include "ui_themeeditorwidget.h"

#include "colorvariable.h"
#include "colorrole.h"
#include "themecolors.h"
#include "themesettingstablemodel.h"
#include "themesettingsitemdelegate.h"

#include <QAbstractButton>
#include <QAbstractItemModel>
#include <QColorDialog>
#include <QDir>
#include <QFileInfo>
#include <QMetaEnum>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QSharedPointer>
#include <QWeakPointer>

namespace Core {
namespace Internal {
namespace ThemeEditor {

ThemeEditorWidget::ThemeEditorWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::ThemeEditorWidget),
    m_readOnly(false),
    m_model(0)
{
    m_ui->setupUi(this);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(0);

    m_ui->tableView->setModel(m_proxyModel);
    ThemeSettingsItemDelegate *cbid = new ThemeSettingsItemDelegate(this);
    m_ui->tableView->setItemDelegate(cbid);
    connect(m_ui->filter, &QLineEdit::textChanged, m_proxyModel,
            static_cast<void (QSortFilterProxyModel:: *)(const QString &)>(&QSortFilterProxyModel::setFilterRegExp));
    connect(m_ui->tableView, &QAbstractItemView::doubleClicked, this, &ThemeEditorWidget::changeColor);
}

ThemeEditorWidget::~ThemeEditorWidget()
{
    delete m_ui;
}

void ThemeEditorWidget::changeColor(const QModelIndex &index)
{
    if (!(m_ui->tableView->editTriggers() & QAbstractItemView::DoubleClicked))
        return;
    if (m_model->inSectionBody(index.row()) != ThemeSettingsTableModel::SectionColors)
        return;
    if (index.column() == 1)
        return;

    int row = m_model->modelToSectionRow(index.row());
    ColorRole::Ptr themeColor = m_model->colors()->colorRole(row);

    QColor currentColor = themeColor->colorVariable()->color();

    // FIXME: 'currentColor' is correct, but QColorDialog won't show
    //        it as the correct initial color. Why?

    QColorDialog dlg(this);
    dlg.setOption(QColorDialog::ShowAlphaChannel);
    dlg.setCurrentColor(currentColor);

    const int customCount = QColorDialog::customCount();
    for (int i = 0; i < customCount; ++i)
        QColorDialog::setCustomColor(i, Qt::transparent); // invalid

    int i = 0;
    foreach (ColorVariable::Ptr namedColor, m_model->colors()->colorVariables())
        QColorDialog::setCustomColor(i++, namedColor->color().toRgb());

    int ret = dlg.exec();
    if (ret == QDialog::Accepted) {
        themeColor->colorVariable()->setColor(dlg.currentColor());
        m_model->markEverythingChanged();
    }
}

void ThemeEditorWidget::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    m_ui->tableView->setEditTriggers(
                readOnly ? QAbstractItemView::NoEditTriggers
                         : QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_ui->filter->setEnabled(!readOnly);
}

void ThemeEditorWidget::initFrom(Utils::Theme *theme)
{
    if (m_model) {
        m_model->setParent(0);
        delete m_model;
    }
    m_model = new ThemeSettingsTableModel(this);
    m_model->initFrom(theme);
    m_proxyModel->setSourceModel(m_model);

    m_ui->tableView->setColumnWidth(0, 400);
    m_ui->tableView->setColumnWidth(1, 300);
}

ThemeSettingsTableModel *ThemeEditorWidget::model()
{
    return m_model;
}

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core
