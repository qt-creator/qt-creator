// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef GLUEINTERFACE_H
#define GLUEINTERFACE_H

#include <QObject>

class GlueInterface final : public QObject
{
    Q_OBJECT

public:
    // business logic -> GUI
    void setRed(bool on) { emit redChanged(on); }
    void setYellow(bool on) { emit yellowChanged(on); }
    void setGreen(bool on) { emit greenChanged(on); }

    // GUI -> business logic
    void smash() { emit smashed(); }
    void repair() { emit repaired(); }

signals:
    void redChanged(bool on);
    void yellowChanged(bool on);
    void greenChanged(bool on);

    void smashed();
    void repaired();
};

#endif // GLUEINTERFACE_H
