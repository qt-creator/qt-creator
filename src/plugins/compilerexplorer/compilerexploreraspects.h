// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

#include <QComboBox>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardItemModel>

namespace CompilerExplorer {

// QMap<Library.Id, Library.Version.Id>
class LibrarySelectionAspect : public Utils::TypedAspect<QMap<QString, QString>>
{
    Q_OBJECT
public:
    enum Roles {
        LibraryData = Qt::UserRole + 1,
        SelectedVersion,
    };

    LibrarySelectionAspect(Utils::AspectContainer *container = nullptr);

    void addToLayout(Layouting::LayoutItem &parent) override;

    using ResultCallback = std::function<void(QList<QStandardItem *>)>;
    using FillCallback = std::function<void(ResultCallback)>;
    void setFillCallback(FillCallback callback) { m_fillCallback = callback; }
    void refill() { emit refillRequested(); }

    void bufferToGui() override;
    bool guiToBuffer() override;

signals:
    void refillRequested();

private:
    FillCallback m_fillCallback;
    QStandardItemModel *m_model{nullptr};
};

} // namespace CompilerExplorer
