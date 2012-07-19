/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef IINTERFACEPOINTER_H
#define IINTERFACEPOINTER_H

#include "common.h"

/* AutoPtr for a IDebugInterface */
template <class IInterface>
class IInterfacePointer
{
    explicit IInterfacePointer(const IInterfacePointer&);
    IInterfacePointer& operator=(const IInterfacePointer&);

public:
    inline IInterfacePointer() : m_instance(0), m_hr(S_OK) {}

    // IDebugClient4 does not inherit IDebugClient.
    inline explicit IInterfacePointer(IDebugClient *client) : m_instance(0), m_hr(S_OK)  { create(client); }
    inline explicit IInterfacePointer(CIDebugClient *client) : m_instance(0), m_hr(S_OK) { create(client); }
    inline ~IInterfacePointer() { release(); }

    inline IInterface *operator->() const { return m_instance; }
    inline IInterface *data() const { return m_instance; }
    inline bool isNull() const   { return m_instance == 0; }
    inline operator bool() const { return !isNull(); }
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
    IInterface *m_instance;
    HRESULT m_hr;
};

#endif // IINTERFACEPOINTER_H
