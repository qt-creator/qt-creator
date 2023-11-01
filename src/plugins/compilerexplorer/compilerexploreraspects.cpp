// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "compilerexploreraspects.h"
#include "compilerexplorertr.h"

#include "api/library.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QComboBox>
#include <QCompleter>
#include <QFutureWatcher>
#include <QPushButton>
#include <QStackedWidget>

using namespace Utils;

namespace CompilerExplorer {

LibrarySelectionAspect::LibrarySelectionAspect(AspectContainer *container)
    : TypedAspect<QMap<QString, QString>>(container)
{}

void LibrarySelectionAspect::bufferToGui()
{
    if (!m_model)
        return;

    for (int i = 0; i < m_model->rowCount(); i++) {
        QModelIndex idx = m_model->index(i, 0);
        if (m_buffer.contains(qvariant_cast<Api::Library>(idx.data(LibraryData)).id))
            m_model->setData(idx,
                             m_buffer[qvariant_cast<Api::Library>(idx.data(LibraryData)).id],
                             SelectedVersion);
        else
            m_model->setData(idx, QVariant(), SelectedVersion);
    }

    handleGuiChanged();
}

bool LibrarySelectionAspect::guiToBuffer()
{
    if (!m_model)
        return false;

    auto oldBuffer = m_buffer;

    m_buffer.clear();

    for (int i = 0; i < m_model->rowCount(); i++) {
        if (m_model->item(i)->data(SelectedVersion).isValid()) {
            m_buffer.insert(qvariant_cast<Api::Library>(m_model->item(i)->data(LibraryData)).id,
                            m_model->item(i)->data(SelectedVersion).toString());
        }
    }
    return oldBuffer != m_buffer;
}

QVariantMap toVariantMap(const QMap<QString, QString> &map)
{
    QVariantMap variant;
    for (const auto &key : map.keys())
        variant.insert(key, map[key]);

    return variant;
}

QVariant LibrarySelectionAspect::variantValue() const
{
    return toVariantMap(m_internal);
}

QVariant LibrarySelectionAspect::volatileVariantValue() const
{
    return toVariantMap(m_buffer);
}

void LibrarySelectionAspect::setVariantValue(const QVariant &value, Announcement howToAnnounce)
{
    QMap<QString, QString> map;
    Store store = storeFromVariant(value);
    for (const auto &key : store.keys())
        map[stringFromKey(key)] = store[key].toString();

    setValue(map, howToAnnounce);
}

void LibrarySelectionAspect::addToLayout(Layouting::LayoutItem &parent)
{
    using namespace Layouting;

    QTC_ASSERT(m_fillCallback, return);

    auto cb = [this](const QList<QStandardItem *> &items) {
        for (QStandardItem *item : items)
            m_model->appendRow(item);

        bufferToGui();
    };

    if (!m_model) {
        m_model = new QStandardItemModel(this);

        connect(this, &LibrarySelectionAspect::refillRequested, this, [this, cb] {
            m_model->clear();
            m_fillCallback(cb);
        });

        m_fillCallback(cb);
    }

    QComboBox *nameCombo = new QComboBox();
    nameCombo->setInsertPolicy(QComboBox::InsertPolicy::NoInsert);
    nameCombo->setEditable(true);
    nameCombo->completer()->setCompletionMode(QCompleter::PopupCompletion);
    nameCombo->completer()->setFilterMode(Qt::MatchContains);

    nameCombo->setModel(m_model);

    QComboBox *versionCombo = new QComboBox();
    versionCombo->addItem("--");

    auto refreshVersionCombo = [nameCombo, versionCombo] {
        versionCombo->clear();
        versionCombo->addItem("--");
        QString selected = nameCombo->currentData(SelectedVersion).toString();
        Api::Library lib = qvariant_cast<Api::Library>(nameCombo->currentData(LibraryData));
        for (const auto &version : lib.versions) {
            versionCombo->addItem(version.version, version.id);
            if (version.id == selected)
                versionCombo->setCurrentIndex(versionCombo->count() - 1);
        }
    };

    refreshVersionCombo();

    connect(nameCombo, &QComboBox::currentIndexChanged, this, refreshVersionCombo);

    connect(versionCombo, &QComboBox::activated, this, [this, nameCombo, versionCombo] {
        if (undoStack()) {
            QVariant old = m_model->data(m_model->index(nameCombo->currentIndex(), 0),
                                         SelectedVersion);
            undoStack()->push(new SelectLibraryVersionCommand(this,
                                                              nameCombo->currentIndex(),
                                                              versionCombo->currentData(),
                                                              old));

            handleGuiChanged();
            return;
        }

        m_model->setData(m_model->index(nameCombo->currentIndex(), 0),
                         versionCombo->currentData(),
                         SelectedVersion);
        handleGuiChanged();
    });

    QPushButton *clearBtn = new QPushButton("Clear All");
    connect(clearBtn, &QPushButton::clicked, clearBtn, [this, refreshVersionCombo] {
        if (undoStack()) {
            undoStack()->beginMacro(Tr::tr("Reset used libraries"));
            for (int i = 0; i < m_model->rowCount(); i++) {
                QModelIndex idx = m_model->index(i, 0);
                if (idx.data(SelectedVersion).isValid())
                    undoStack()->push(new SelectLibraryVersionCommand(this,
                                                                      i,
                                                                      QVariant(),
                                                                      idx.data(SelectedVersion)));
            }
            undoStack()->endMacro();

            handleGuiChanged();
            refreshVersionCombo();
            return;
        }

        for (int i = 0; i < m_model->rowCount(); i++)
            m_model->setData(m_model->index(i, 0), QVariant(), SelectedVersion);
        handleGuiChanged();
        refreshVersionCombo();
    });

    ElidingLabel *displayLabel = new ElidingLabel();

    auto updateLabel = [displayLabel, this] {
        QStringList libs;
        for (int i = 0; i < m_model->rowCount(); i++) {
            QModelIndex idx = m_model->index(i, 0);
            if (idx.data(LibraryData).isValid() && idx.data(SelectedVersion).isValid()) {
                auto libData = idx.data(LibraryData).value<Api::Library>();
                auto id = idx.data(SelectedVersion).toString();

                auto versionIt = std::find_if(libData.versions.begin(),
                                              libData.versions.end(),
                                              [id](const Api::Library::Version &v) {
                                                  return v.id == id;
                                              });
                const QString versionName = versionIt == libData.versions.end()
                                                ? id
                                                : versionIt->version;

                libs.append(QString("%1 %2").arg(libData.name).arg(versionName));
            }
        }
        if (libs.empty())
            displayLabel->setText(Tr::tr("No libraries selected"));
        else
            displayLabel->setText(libs.join(", "));
    };

    connect(m_model, &QStandardItemModel::itemChanged, displayLabel, updateLabel);

    updateLabel();

    QPushButton *editBtn = new QPushButton(Tr::tr("Edit"));

    // clang-format off
    QStackedWidget *stack = static_cast<QStackedWidget*>(
        Stack {
            noMargin,
            Row { noMargin, displayLabel, editBtn },
            Row { noMargin, nameCombo, versionCombo, clearBtn }
        }.emerge()
    );
    // clang-format on
    connect(editBtn, &QPushButton::clicked, stack, [stack] { stack->setCurrentIndex(1); });
    connect(this, &LibrarySelectionAspect::returnToDisplay, stack, [stack] {
        stack->setCurrentIndex(0);
    });

    addLabeledItem(parent, stack);
}

} // namespace CompilerExplorer
