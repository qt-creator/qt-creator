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

    class SelectLibraryVersionCommand : public QUndoCommand
    {
    public:
        SelectLibraryVersionCommand(LibrarySelectionAspect *aspect,
                                    int libraryIndex,
                                    const QVariant &versionId,
                                    const QVariant &oldVersionId = QVariant())
            : m_aspect(aspect)
            , m_libraryIndex(libraryIndex)
            , m_versionId(versionId)
            , m_oldVersionId(oldVersionId)
        {}

        void undo() override
        {
            m_aspect->m_model->setData(m_aspect->m_model->index(m_libraryIndex, 0),
                                       m_oldVersionId,
                                       LibrarySelectionAspect::SelectedVersion);
            m_aspect->handleGuiChanged();
            emit m_aspect->returnToDisplay();
        }

        void redo() override
        {
            m_aspect->m_model->setData(m_aspect->m_model->index(m_libraryIndex, 0),
                                       m_versionId,
                                       LibrarySelectionAspect::SelectedVersion);
            if (!m_firstTime) {
                emit m_aspect->returnToDisplay();
                m_aspect->handleGuiChanged();
            }
            m_firstTime = false;
        }

    private:
        LibrarySelectionAspect *m_aspect;
        int m_libraryIndex;
        QVariant m_versionId;
        QVariant m_oldVersionId;
        bool m_firstTime{true};
    };

    LibrarySelectionAspect(Utils::AspectContainer *container = nullptr);

    void addToLayout(Layouting::LayoutItem &parent) override;

    using ResultCallback = std::function<void(QList<QStandardItem *>)>;
    using FillCallback = std::function<void(ResultCallback)>;
    void setFillCallback(FillCallback callback) { m_fillCallback = callback; }
    void refill() { emit refillRequested(); }

    void bufferToGui() override;
    bool guiToBuffer() override;

    QVariant variantValue() const override;
    QVariant volatileVariantValue() const override;

    void setVariantValue(const QVariant &value, Announcement howToAnnounce = DoEmit) override;

signals:
    void refillRequested();
    void returnToDisplay();

private:
    FillCallback m_fillCallback;
    QStandardItemModel *m_model{nullptr};
};

} // namespace CompilerExplorer
