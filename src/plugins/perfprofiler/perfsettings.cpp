// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfsettings.h"

#include "perfconfigeventsmodel.h"
#include "perfprofilerconstants.h"
#include "perfprofilertr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <debugger/analyzer/analyzericons.h>
#include <debugger/debuggertr.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/target.h>

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QMetaEnum>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableView>

using namespace ProjectExplorer;
using namespace Utils;

using namespace PerfProfiler::Internal;

namespace PerfProfiler {

class PerfConfigWidget : public QWidget
{
public:
    PerfConfigWidget(PerfSettings *settings, Target *target);

private:
    void readTracePoints();
    void handleProcessDone();

    PerfSettings *m_settings;
    std::unique_ptr<Utils::Process> m_process;

    QTableView *eventsView;
    QPushButton *useTracePointsButton;
};

class SettingsDelegate : public QStyledItemDelegate
{
public:
    SettingsDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }
};

PerfConfigWidget::PerfConfigWidget(PerfSettings *settings, Target *target)
    : m_settings(settings)
{
    eventsView = new QTableView(this);
    eventsView->setMinimumSize(QSize(0, 300));
    eventsView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    eventsView->setSelectionMode(QAbstractItemView::SingleSelection);
    eventsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    eventsView->setModel(new PerfConfigEventsModel(m_settings, this));
    eventsView->setItemDelegate(new SettingsDelegate(this));
    eventsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    useTracePointsButton = new QPushButton(this);
    useTracePointsButton->setText(Tr::tr("Use Trace Points"));
    useTracePointsButton->setVisible(target != nullptr);
    connect(useTracePointsButton, &QPushButton::pressed,
            this, &PerfConfigWidget::readTracePoints);

    auto addEventButton = new QPushButton(Tr::tr("Add Event"), this);
    connect(addEventButton, &QPushButton::pressed, this, [this]() {
        auto model = eventsView->model();
        model->insertRow(model->rowCount());
    });

    auto removeEventButton = new QPushButton(Tr::tr("Remove Event"), this);
    connect(removeEventButton, &QPushButton::pressed, this, [this] {
        QModelIndex index = eventsView->currentIndex();
        if (index.isValid())
            eventsView->model()->removeRow(index.row());
    });

    auto resetButton = new QPushButton(Tr::tr("Reset"), this);
    connect(resetButton, &QPushButton::pressed, m_settings, &PerfSettings::resetToDefault);

    using namespace Layouting;
    Column {
        Row { st, useTracePointsButton, addEventButton, removeEventButton, resetButton },

        eventsView,

        Grid {
            m_settings->callgraphMode, m_settings->stackSize, br,
            m_settings->sampleMode, m_settings->period, br,
            m_settings->extraArguments,
        },

        st
    }.attachTo(this);

    IDevice::ConstPtr device;
    if (target)
        device = DeviceKitAspect::device(target->kit());

    if (device.isNull()) {
        useTracePointsButton->setEnabled(false);
        return;
    }

    QTC_ASSERT(device, return);
    QTC_CHECK(!m_process || m_process->state() == QProcess::NotRunning);

    m_process.reset(new Process);
    m_process->setCommand({device->filePath("perf"), {"probe", "-l"}});
    connect(m_process.get(), &Process::done,
            this, &PerfConfigWidget::handleProcessDone);

    useTracePointsButton->setEnabled(true);
}

void PerfConfigWidget::readTracePoints()
{
    QMessageBox messageBox;
    messageBox.setWindowTitle(Tr::tr("Use Trace Points"));
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setText(Tr::tr("Replace events with trace points read from the device?"));
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    if (messageBox.exec() == QMessageBox::Yes) {
        m_process->start();
        useTracePointsButton->setEnabled(false);
    }
}

void PerfConfigWidget::handleProcessDone()
{
    if (m_process->error() == QProcess::FailedToStart) {
        Core::AsynchronousMessageBox::warning(
                    Tr::tr("Cannot List Trace Points"),
                    Tr::tr("\"perf probe -l\" failed to start. Is perf installed?"));
        useTracePointsButton->setEnabled(true);
        return;
    }
    const QList<QByteArray> lines =
            m_process->readAllRawStandardOutput().append(m_process->readAllRawStandardError())
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
                    Tr::tr("No Trace Points Found"),
                    Tr::tr("Trace points can be defined with \"perf probe -a\"."));
    } else {
        for (const QByteArray &event : std::as_const(tracePoints)) {
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

// PerfSettingsPage

PerfSettings &globalSettings()
{
    static PerfSettings theSettings(nullptr);
    return theSettings;
}

PerfSettings::PerfSettings(ProjectExplorer::Target *target)
{
    setAutoApply(false);
    period.setSettingsKey("Analyzer.Perf.Frequency");
    period.setRange(250, 2147483647);
    period.setDefaultValue(250);
    period.setLabelText(Tr::tr("Sample period:"));

    stackSize.setSettingsKey("Analyzer.Perf.StackSize");
    stackSize.setRange(4096, 65536);
    stackSize.setDefaultValue(4096);
    stackSize.setLabelText(Tr::tr("Stack snapshot size (kB):"));

    sampleMode.setSettingsKey("Analyzer.Perf.SampleMode");
    sampleMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    sampleMode.setLabelText(Tr::tr("Sample mode:"));
    sampleMode.addOption({Tr::tr("frequency (Hz)"), {}, QString("-F")});
    sampleMode.addOption({Tr::tr("event count"), {}, QString("-c")});
    sampleMode.setDefaultValue(0);

    callgraphMode.setSettingsKey("Analyzer.Perf.CallgraphMode");
    callgraphMode.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    callgraphMode.setLabelText(Tr::tr("Call graph mode:"));
    callgraphMode.addOption({Tr::tr("dwarf"), {}, QString(Constants::PerfCallgraphDwarf)});
    callgraphMode.addOption({Tr::tr("frame pointer"), {}, QString("fp")});
    callgraphMode.addOption({Tr::tr("last branch record"), {}, QString("lbr")});
    callgraphMode.setDefaultValue(0);

    events.setSettingsKey("Analyzer.Perf.Events");
    events.setDefaultValue({"cpu-cycles"});

    extraArguments.setSettingsKey("Analyzer.Perf.ExtraArguments");
    extraArguments.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
    extraArguments.setLabelText(Tr::tr("Additional arguments:"));
    extraArguments.setSpan(4);

    connect(&callgraphMode, &SelectionAspect::volatileValueChanged, this, [this] {
        stackSize.setEnabled(callgraphMode.volatileValue() == 0);
    });

    setLayouter([this, target] {
        using namespace Layouting;
        auto widget = new PerfConfigWidget(this, target);
        return Column { widget };
    });

    readGlobalSettings();
    readSettings();
}

PerfSettings::~PerfSettings()
{
}

void PerfSettings::readGlobalSettings()
{
    Store defaults;

    // Read stored values
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::AnalyzerSettingsGroupId);
    Store map = defaults;
    for (Store::ConstIterator it = defaults.constBegin(); it != defaults.constEnd(); ++it)
        map.insert(it.key(), settings->value(it.key(), it.value()));
    settings->endGroup();

    fromMap(map);
}

void PerfSettings::writeGlobalSettings() const
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::AnalyzerSettingsGroupId);
    Store map;
    toMap(map);
    for (Store::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
        settings->setValue(it.key(), it.value());
    settings->endGroup();
}

void PerfSettings::addPerfRecordArguments(CommandLine *cmd) const
{
    QString callgraphArg = callgraphMode.itemValue().toString();
    if (callgraphArg == Constants::PerfCallgraphDwarf)
        callgraphArg += "," + QString::number(stackSize());

    QString events;
    for (const QString &event : this->events()) {
        if (!event.isEmpty()) {
            if (!events.isEmpty())
                events.append(',');
            events.append(event);
        }
    }

    cmd->addArgs({"-e", events,
                  "--call-graph", callgraphArg,
                  sampleMode.itemValue().toString(),
                  QString::number(period())});
    cmd->addArgs(extraArguments(), CommandLine::Raw);
}

void PerfSettings::resetToDefault()
{
    PerfSettings defaults;
    Store map;
    defaults.toMap(map);
    fromMap(map);
}

QWidget *PerfSettings::createPerfConfigWidget(Target *target)
{
    return new PerfConfigWidget(this, target);
}

// PerfSettingsPage

class PerfSettingsPage final : public Core::IOptionsPage
{
public:
    PerfSettingsPage()
    {
        setId(Constants::PerfSettingsId);
        setDisplayName(Tr::tr("CPU Usage"));
        setCategory("T.Analyzer");
        setDisplayCategory(::Debugger::Tr::tr("Analyzer"));
        setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
        setSettingsProvider([] { return &globalSettings(); });
    }
};

const PerfSettingsPage settingsPage;

} // namespace PerfProfiler
