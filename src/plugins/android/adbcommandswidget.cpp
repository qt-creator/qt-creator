/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "adbcommandswidget.h"
#include "ui_adbcommandswidget.h"

#include "utils/utilsicons.h"

#include <QGroupBox>
#include <QItemSelectionModel>
#include <QShortcut>
#include <QStringListModel>

#include <functional>

namespace Android {
namespace Internal {

using namespace std::placeholders;

const char defaultCommand[] = "echo \"shell command\"";

static void swapData(QStringListModel *model, const QModelIndex &srcIndex,
                     const QModelIndex &destIndex)
{
    if (model) {
        QVariant data = model->data(destIndex, Qt::EditRole); // QTBUG-55078
        model->setData(destIndex, model->data(srcIndex, Qt::EditRole));
        model->setData(srcIndex, data);
    }
}

class AdbCommandsWidgetPrivate
{
public:
    AdbCommandsWidgetPrivate(const AdbCommandsWidget &parent, QWidget *parentWidget);
    ~AdbCommandsWidgetPrivate();

private:
    void addString(const QString &str);
    void onAddButton();
    void onMoveUpButton();
    void onMoveDownButton();
    void onRemove();
    void onCurrentIndexChanged(const QModelIndex &newIndex, const QModelIndex &prevIndex);

    const AdbCommandsWidget &m_parent;
    QGroupBox *m_rootWidget = nullptr;
    Ui::AdbCommandsWidget *m_ui = nullptr;
    QStringListModel *m_stringModel = nullptr;
    friend class AdbCommandsWidget;
};

AdbCommandsWidget::AdbCommandsWidget(QWidget *parent) :
    QObject(parent),
    d(new AdbCommandsWidgetPrivate(*this, parent))
{
}

AdbCommandsWidget::~AdbCommandsWidget()
{
}

QStringList AdbCommandsWidget::commandsList() const
{
    return d->m_stringModel->stringList();
}

QWidget *AdbCommandsWidget::widget() const
{
    return d->m_rootWidget;
}

void AdbCommandsWidget::setTitleText(const QString &title)
{
    d->m_rootWidget->setTitle(title);
}

void AdbCommandsWidget::setCommandList(const QStringList &commands)
{
    d->m_stringModel->setStringList(commands);
}

AdbCommandsWidgetPrivate::AdbCommandsWidgetPrivate(const AdbCommandsWidget &parent,
                                                   QWidget *parentWidget):
    m_parent(parent),
    m_rootWidget(new QGroupBox(parentWidget)),
    m_ui(new Ui::AdbCommandsWidget),
    m_stringModel(new QStringListModel)
{
    m_ui->setupUi(m_rootWidget);
    m_ui->addButton->setIcon(Utils::Icons::PLUS.icon());
    m_ui->removeButton->setIcon(Utils::Icons::MINUS.icon());
    m_ui->moveUpButton->setIcon(Utils::Icons::ARROW_UP.icon());
    m_ui->moveDownButton->setIcon(Utils::Icons::ARROW_DOWN.icon());

    auto deleteShortcut = new QShortcut(QKeySequence(QKeySequence::Delete), m_ui->commandsView);
    deleteShortcut->setContext(Qt::WidgetShortcut);
    QObject::connect(deleteShortcut, &QShortcut::activated,
                     std::bind(&AdbCommandsWidgetPrivate::onRemove, this));

    QObject::connect(m_ui->addButton, &QToolButton::clicked,
                     std::bind(&AdbCommandsWidgetPrivate::onAddButton, this));
    QObject::connect(m_ui->removeButton, &QToolButton::clicked,
                     std::bind(&AdbCommandsWidgetPrivate::onRemove, this));
    QObject::connect(m_ui->moveUpButton, &QToolButton::clicked,
                     std::bind(&AdbCommandsWidgetPrivate::onMoveUpButton, this));
    QObject::connect(m_ui->moveDownButton, &QToolButton::clicked,
                     std::bind(&AdbCommandsWidgetPrivate::onMoveDownButton, this));

    m_ui->commandsView->setModel(m_stringModel);
    QObject::connect(m_stringModel, &QStringListModel::dataChanged,
                     &m_parent, &AdbCommandsWidget::commandsChanged);
    QObject::connect(m_stringModel, &QStringListModel::rowsRemoved,
                     &m_parent, &AdbCommandsWidget::commandsChanged);
    QObject::connect(m_ui->commandsView->selectionModel(), &QItemSelectionModel::currentChanged,
                     std::bind(&AdbCommandsWidgetPrivate::onCurrentIndexChanged, this, _1, _2));
}

AdbCommandsWidgetPrivate::~AdbCommandsWidgetPrivate()
{
    delete m_ui;
    delete m_stringModel;
}

void AdbCommandsWidgetPrivate::addString(const QString &str)
{
    if (!str.isEmpty()) {
        m_stringModel->insertRows(m_stringModel->rowCount(), 1);
        const QModelIndex lastItemIndex = m_stringModel->index(m_stringModel->rowCount() - 1);
        m_stringModel->setData(lastItemIndex, str);
    }
}

void AdbCommandsWidgetPrivate::onAddButton()
{
    addString(defaultCommand);
    const QModelIndex index = m_stringModel->index(m_stringModel->rowCount() - 1);
    m_ui->commandsView->setCurrentIndex(index);
    m_ui->commandsView->edit(index);
}

void AdbCommandsWidgetPrivate::onMoveUpButton()
{
    QModelIndex index = m_ui->commandsView->currentIndex();
    if (index.row() > 0) {
        const QModelIndex newIndex = m_stringModel->index(index.row() - 1, 0);
        swapData(m_stringModel, index, newIndex);
        m_ui->commandsView->setCurrentIndex(newIndex);
    }
}

void AdbCommandsWidgetPrivate::onMoveDownButton()
{
    QModelIndex index = m_ui->commandsView->currentIndex();
    if (index.row() < m_stringModel->rowCount() - 1) {
        const QModelIndex newIndex = m_stringModel->index(index.row() + 1, 0);
        swapData(m_stringModel, index, newIndex);
        m_ui->commandsView->setCurrentIndex(newIndex);
    }
}

void AdbCommandsWidgetPrivate::onRemove()
{
    const QModelIndex &index = m_ui->commandsView->currentIndex();
    if (index.isValid())
        m_stringModel->removeRow(index.row());
}

void AdbCommandsWidgetPrivate::onCurrentIndexChanged(const QModelIndex &newIndex, const QModelIndex &prevIndex)
{
    Q_UNUSED(prevIndex)
    m_ui->moveUpButton->setEnabled(newIndex.row() != 0);
    m_ui->moveDownButton->setEnabled(newIndex.row() < m_stringModel->rowCount() - 1);
    m_ui->removeButton->setEnabled(newIndex.isValid());
}

} // Internal
} // Android
