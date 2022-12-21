// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "bineditor_global.h"

#include <QColor>
#include <QObject>
#include <QString>

#include <functional>

namespace Core { class IEditor; }

namespace BinEditor {

class EditorService
{
public:
    virtual ~EditorService() = default;

    virtual QWidget *widget() = 0;
    virtual Core::IEditor *editor() = 0;

    // "Slots"
    virtual void setSizes(quint64 address, qint64 range, int blockSize) = 0;
    virtual void setReadOnly(bool on) = 0;
    virtual void setFinished() = 0;
    virtual void setNewWindowRequestAllowed(bool on) = 0;
    virtual void setCursorPosition(qint64 pos) = 0;
    virtual void updateContents() = 0;
    virtual void addData(quint64 address, const QByteArray &data) = 0;

    virtual void clearMarkup() = 0;
    virtual void addMarkup(quint64 address, quint64 len, const QColor &color, const QString &toolTip) = 0;
    virtual void commitMarkup() = 0;

    // "Signals"
    virtual void setFetchDataHandler(const std::function<void(quint64 block)> &) = 0;
    virtual void setNewWindowRequestHandler(const std::function<void(quint64 address)> &) = 0;
    virtual void setNewRangeRequestHandler(const std::function<void(quint64 address)> &) = 0;
    virtual void setDataChangedHandler(const std::function<void(quint64 address, const QByteArray &)> &) = 0;
    virtual void setWatchPointRequestHandler(const std::function<void(quint64 address, uint size)> &) = 0;
    virtual void setAboutToBeDestroyedHandler(const std::function<void()> &) = 0;
};

class FactoryService
{
public:
    virtual ~FactoryService() = default;

    // Create a BinEditor widget. Embed into a Core::IEditor iff wantsEditor == true.
    virtual EditorService *createEditorService(const QString &title, bool wantsEditor) = 0;
};

} // namespace BinEditor

#define BinEditor_FactoryService_iid "org.qt-project.Qt.Creator.BinEditor.EditorService"

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(BinEditor::FactoryService, BinEditor_FactoryService_iid)
QT_END_NAMESPACE
