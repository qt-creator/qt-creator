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

#include "perfconfigeventsmodel.h"
#include "perfconfigwidget.h"
#include "perfprofilerconstants.h"

#include <coreplugin/messagebox.h>

#include <projectexplorer/kit.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>

#include <QComboBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QMetaEnum>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableView>

using namespace Utils;

namespace PerfProfiler {
namespace Internal {

class SettingsDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    SettingsDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
};

PerfConfigWidget::PerfConfigWidget(PerfSettings *settings, QWidget *parent)
    : m_settings(settings)
{
    setParent(parent);

    eventsView = new QTableView(this);
    eventsView->setMinimumSize(QSize(0, 300));
    eventsView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    eventsView->setSelectionMode(QAbstractItemView::SingleSelection);
    eventsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    eventsView->setModel(new PerfConfigEventsModel(m_settings, this));
    eventsView->setItemDelegate(new SettingsDelegate(this));
    eventsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    useTracePointsButton = new QPushButton(this);
    useTracePointsButton->setText(tr("Use Trace Points"));
    useTracePointsButton->setVisible(false);
    connect(useTracePointsButton, &QPushButton::pressed,
            this, &PerfConfigWidget::readTracePoints);

    addEventButton = new QPushButton(this);
    addEventButton->setText(tr("Add Event"));
    connect(addEventButton, &QPushButton::pressed, this, [this]() {
        auto model = eventsView->model();
        model->insertRow(model->rowCount());
    });

    removeEventButton = new QPushButton(this);
    removeEventButton->setText(tr("Remove Event"));
    connect(removeEventButton, &QPushButton::pressed, this, [this]() {
        QModelIndex index = eventsView->currentIndex();
        if (index.isValid())
            eventsView->model()->removeRow(index.row());
    });

    resetButton = new QPushButton(this);
    resetButton->setText(tr("Reset"));
    connect(resetButton, &QPushButton::pressed, m_settings, &PerfSettings::resetToDefault);

    using namespace Layouting;
    const Break nl;

    Column {
        Row { Stretch(), useTracePointsButton, addEventButton, removeEventButton, resetButton },

        eventsView,

        Grid {
            m_settings->callgraphMode, m_settings->stackSize, nl,
            m_settings->sampleMode, m_settings->period, nl,
            m_settings->extraArguments,
        },

        Stretch()
    }.attachTo(this);
}

PerfConfigWidget::~PerfConfigWidget() = default;

void PerfConfigWidget::setTarget(ProjectExplorer::Target *target)
{
    ProjectExplorer::IDevice::ConstPtr device;
    if (target) {
        if (ProjectExplorer::Kit *kit = target->kit())
            device = ProjectExplorer::DeviceKitAspect::device(kit);
    }

    if (device.isNull()) {
        useTracePointsButton->setEnabled(false);
        return;
    }

    QTC_ASSERT(device, return);
    QTC_CHECK(!m_process || m_process->state() == QProcess::NotRunning);

    m_process.reset(device->createProcess(nullptr));
    if (!m_process) {
        useTracePointsButton->setEnabled(false);
        return;
    }

    connect(m_process.get(), &QtcProcess::finished,
            this, &PerfConfigWidget::handleProcessFinished);

    connect(m_process.get(), &QtcProcess::errorOccurred,
            this, &PerfConfigWidget::handleProcessError);

    useTracePointsButton->setEnabled(true);
}

void PerfConfigWidget::setTracePointsButtonVisible(bool visible)
{
    useTracePointsButton->setVisible(visible);
}

void PerfConfigWidget::apply()
{
    m_settings->writeGlobalSettings();
}

void PerfConfigWidget::readTracePoints()
{
    QMessageBox messageBox;
    messageBox.setWindowTitle(tr("Use Trace Points"));
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setText(tr("Replace events with trace points read from the device?"));
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (messageBox.exec() == QMessageBox::Yes) {
        m_process->setCommand({"perf", {"probe", "-l"}});
        m_process->start();
        useTracePointsButton->setEnabled(false);
    }
}

void PerfConfigWidget::handleProcessFinished()
{
    const QList<QByteArray> lines =
            m_process->readAllStandardOutput().append(m_process->readAllStandardError())
            .split('\n');
    auto model = eventsView->model();
    const int previousRows = model->rowCount();
    QHash<QByteArray, QByteArray> tracePoints;
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        const int space = trimmed.indexOf(' ');
        if (space < 0)
            continue;

        // If the whole "on ..." string is the same, the trace points are redundant
        tracePoints[trimmed.mid(space + 1)] = trimmed.left(space);
    }

    if (tracePoints.isEmpty()) {
        Core::AsynchronousMessageBox::warning(
                    tr("No Trace Points Found"),
                    tr("Trace points can be defined with \"perf probe -a\"."));
    } else {
        for (const QByteArray &event : qAsConst(tracePoints)) {
            int row = model->rowCount();
            model->insertRow(row);
            model->setData(model->index(row, PerfConfigEventsModel::ColumnEventType),
                           PerfConfigEventsModel::EventTypeCustom);
            model->setData(model->index(row, PerfConfigEventsModel::ColumnSubType),
                           QString::fromUtf8(event));
        }
        model->removeRows(0, previousRows);
        m_settings->sampleMode.setVolatileValue(1);
        m_settings->period.setVolatileValue(1);
    }
    useTracePointsButton->setEnabled(true);
}

void PerfConfigWidget::handleProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        Core::AsynchronousMessageBox::warning(
                    tr("Cannot List Trace Points"),
                    tr("\"perf probe -l\" failed to start. Is perf installed?"));
        useTracePointsButton->setEnabled(true);
    }
}

QWidget *SettingsDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(option)
    const int row = index.row();
    const int column = index.column();
    const PerfConfigEventsModel *model = qobject_cast<const PerfConfigEventsModel *>(index.model());

    auto getRowEventType = [&]() {
        return qvariant_cast<PerfConfigEventsModel::EventType>(
                    model->data(model->index(row, PerfConfigEventsModel::ColumnEventType),
                                Qt::EditRole));
    };

    switch (column) {
    case PerfConfigEventsModel::ColumnEventType: {
        QComboBox *editor = new QComboBox(parent);
        QMetaEnum meta = QMetaEnum::fromType<PerfConfigEventsModel::EventType>();
        for (int i = 0; i < PerfConfigEventsModel::EventTypeInvalid; ++i) {
            editor->addItem(QString::fromLatin1(meta.valueToKey(i)).mid(
                                static_cast<int>(strlen("EventType"))).toLower(), i);
        }
        return editor;
    }
    case PerfConfigEventsModel::ColumnSubType: {
        PerfConfigEventsModel::EventType eventType = getRowEventType();
        switch (eventType) {
        case PerfConfigEventsModel::EventTypeHardware: {
            QComboBox *editor = new QComboBox(parent);
            for (int i = PerfConfigEventsModel::SubTypeEventTypeHardware;
                 i < PerfConfigEventsModel::SubTypeEventTypeSoftware; ++i) {
                editor->addItem(PerfConfigEventsModel::subTypeString(PerfConfigEventsModel::EventTypeHardware,
                                                     PerfConfigEventsModel::SubType(i)), i);
            }
            return editor;
        }
        case PerfConfigEventsModel::EventTypeSoftware: {
            QComboBox *editor = new QComboBox(parent);
            for (int i = PerfConfigEventsModel::SubTypeEventTypeSoftware;
                 i < PerfConfigEventsModel::SubTypeEventTypeCache; ++i) {
                editor->addItem(PerfConfigEventsModel::subTypeString(PerfConfigEventsModel::EventTypeSoftware,
                                                     PerfConfigEventsModel::SubType(i)), i);
            }
            return editor;
        }
        case PerfConfigEventsModel::EventTypeCache: {
            QComboBox *editor = new QComboBox(parent);
            for (int i = PerfConfigEventsModel::SubTypeEventTypeCache;
                 i < PerfConfigEventsModel::SubTypeInvalid; ++i) {
                editor->addItem(PerfConfigEventsModel::subTypeString(PerfConfigEventsModel::EventTypeCache,
                                                     PerfConfigEventsModel::SubType(i)), i);
            }
            return editor;
        }
        case PerfConfigEventsModel::EventTypeBreakpoint: {
            QLineEdit *editor = new QLineEdit(parent);
            editor->setText("0x0000000000000000");
            editor->setValidator(new QRegularExpressionValidator(
                                     QRegularExpression("0x[0-9a-f]{16}"), parent));
            return editor;
        }
        case PerfConfigEventsModel::EventTypeCustom: {
            QLineEdit *editor = new QLineEdit(parent);
            return editor;
        }
        case PerfConfigEventsModel::EventTypeRaw: {
            QLineEdit *editor = new QLineEdit(parent);
            editor->setText("r000");
            editor->setValidator(new QRegularExpressionValidator(
                                     QRegularExpression("r[0-9a-f]{3}"), parent));
            return editor;
        }
        case PerfConfigEventsModel::EventTypeInvalid:
            return nullptr;
        }
        return nullptr; // Will never be reached, but GCC cannot figure this out.
    }
    case PerfConfigEventsModel::ColumnOperation: {
        QComboBox *editor = new QComboBox(parent);
        PerfConfigEventsModel::EventType eventType = getRowEventType();
        if (eventType == PerfConfigEventsModel::EventTypeCache) {
            editor->addItem("load", PerfConfigEventsModel::OperationLoad);
            editor->addItem("store", PerfConfigEventsModel::OperationStore);
            editor->addItem("prefetch", PerfConfigEventsModel::OperationPrefetch);
        } else if (eventType == PerfConfigEventsModel::EventTypeBreakpoint) {
            editor->addItem("r", PerfConfigEventsModel::OperationLoad);
            editor->addItem("rw", PerfConfigEventsModel::OperationLoad
                            | PerfConfigEventsModel::OperationStore);
            editor->addItem("rwx", PerfConfigEventsModel::OperationLoad
                            | PerfConfigEventsModel::OperationStore
                            | PerfConfigEventsModel::OperationExecute);
            editor->addItem("rx", PerfConfigEventsModel::OperationLoad
                            | PerfConfigEventsModel::OperationExecute);
            editor->addItem("w", PerfConfigEventsModel::OperationStore);
            editor->addItem("wx", PerfConfigEventsModel::OperationStore
                            | PerfConfigEventsModel::OperationExecute);
            editor->addItem("x", PerfConfigEventsModel::OperationExecute);
        } else {
            editor->setEnabled(false);
        }
        return editor;
    }
    case PerfConfigEventsModel::ColumnResult: {
        QComboBox *editor = new QComboBox(parent);
        PerfConfigEventsModel::EventType eventType = getRowEventType();
        if (eventType != PerfConfigEventsModel::EventTypeCache) {
            editor->setEnabled(false);
        } else {
            editor->addItem("refs", PerfConfigEventsModel::ResultRefs);
            editor->addItem("misses", PerfConfigEventsModel::ResultMisses);
        }
        return editor;
    }
    default:
        return nullptr;
    }
}

void SettingsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (QComboBox *combo = qobject_cast<QComboBox *>(editor)) {
        QVariant data = index.model()->data(index, Qt::EditRole);
        for (int i = 0, end = combo->count(); i != end; ++i) {
            if (combo->itemData(i) == data) {
                combo->setCurrentIndex(i);
                return;
            }
        }
    } else if (QLineEdit *lineedit = qobject_cast<QLineEdit *>(editor)) {
        lineedit->setText(index.model()->data(index, Qt::DisplayRole).toString());
    }
}

void SettingsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    if (QComboBox *combo = qobject_cast<QComboBox *>(editor)) {
        model->setData(index, combo->currentData());
    } else if (QLineEdit *lineedit = qobject_cast<QLineEdit *>(editor)) {
        QString text = lineedit->text();
        QVariant type = model->data(model->index(index.row(),
                                                 PerfConfigEventsModel::ColumnEventType),
                                    Qt::EditRole);
        switch (qvariant_cast<PerfConfigEventsModel::EventType>(type)) {
        case PerfConfigEventsModel::EventTypeRaw:
            model->setData(index, text.mid(static_cast<int>(strlen("r"))).toULongLong(nullptr, 16));
            break;
        case PerfConfigEventsModel::EventTypeBreakpoint:
            model->setData(index,
                           text.mid(static_cast<int>(strlen("0x"))).toULongLong(nullptr, 16));
            break;
        case PerfConfigEventsModel::EventTypeCustom:
            model->setData(index, text);
            break;
        default:
            break;
        }
    }
}

void SettingsDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

} // namespace Internal
} // namespace PerfProfiler

#include "perfconfigwidget.moc"
