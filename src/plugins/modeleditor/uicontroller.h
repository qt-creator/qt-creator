// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace ModelEditor {
namespace Internal {

class UiController : public QObject
{
    Q_OBJECT
    class UiControllerPrivate;

public:
    UiController() {}

signals:
    void rightSplitterChanged(const QByteArray &state);
    void rightHorizSplitterChanged(const QByteArray &state);

public:
    bool hasRightSplitterState() const;
    QByteArray rightSplitterState() const;
    bool hasRightHorizSplitterState() const;
    QByteArray rightHorizSplitterState() const;

    void onRightSplitterChanged(const QByteArray &state);
    void onRightHorizSplitterChanged(const QByteArray &state);
    void saveSettings();
    void loadSettings();

private:
    QByteArray m_rightSplitterState;
    QByteArray m_rightHorizSplitterState;
};

} // namespace Internal
} // namespace ModelEditor
