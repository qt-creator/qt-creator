// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qul/singleton.h>
#include <qul/property.h>
#include <qul/signal.h>

struct BackendObject : public Qul::Singleton<BackendObject>
{
    Qul::Property<bool> customProperty;
    Qul::Signal<void()> customPropertyChanged;

    BackendObject() : customProperty(true) {}
    void toggle()
    {
        customProperty.setValue(!customProperty.value());
        customPropertyChanged();
    }
};
