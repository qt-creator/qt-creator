/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
