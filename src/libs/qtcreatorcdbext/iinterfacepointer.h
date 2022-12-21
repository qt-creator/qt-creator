// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "common.h"

/* AutoPtr for a IDebugInterface */
template <class IInterface>
class IInterfacePointer
{
    explicit IInterfacePointer(const IInterfacePointer&);
    IInterfacePointer& operator=(const IInterfacePointer&);

public:
    IInterfacePointer() = default;

    // IDebugClient4 does not inherit IDebugClient.
    inline explicit IInterfacePointer(IDebugClient *client) : m_instance(0), m_hr(S_OK)  { create(client); }
    inline explicit IInterfacePointer(CIDebugClient *client) : m_instance(0), m_hr(S_OK) { create(client); }
    inline ~IInterfacePointer() { release(); }

    inline IInterface *operator->() const { return m_instance; }
    inline IInterface *data() const { return m_instance; }
    inline bool isNull() const   { return m_instance == 0; }
    explicit inline operator bool() const { return !isNull(); }
    inline HRESULT hr() const { return m_hr; }

    // This is for creating a IDebugClient from scratch. Not matches by a constructor,
    // unfortunately
    inline bool create() {
        release();
        m_hr = DebugCreate(__uuidof(IInterface), (void **)&m_instance);
        return m_hr == S_OK;
    }

    inline bool create(IDebugClient *client) {
        release();
        m_hr = client->QueryInterface(__uuidof(IInterface), (void **)&m_instance);
        return m_hr == S_OK;
    }
    inline bool create(CIDebugClient *client) {
        release();
        m_hr = client->QueryInterface(__uuidof(IInterface), (void **)&m_instance);
        return m_hr == S_OK;
    }


private:
    void release() {
        if (m_instance) {
            m_instance->Release();
            m_instance = 0;
            m_hr = S_OK;
        }
    }
    IInterface *m_instance = nullptr;
    HRESULT m_hr = S_OK;
};
