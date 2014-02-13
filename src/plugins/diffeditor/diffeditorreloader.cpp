/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "diffeditorreloader.h"
#include "diffeditorcontroller.h"

namespace DiffEditor {

DiffEditorReloader::DiffEditorReloader(QObject *parent)
    : QObject(parent),
      m_controller(0),
      m_reloading(false)
{
}

DiffEditorReloader::~DiffEditorReloader()
{

}

DiffEditorController *DiffEditorReloader::diffEditorController() const
{
    return m_controller;
}

void DiffEditorReloader::setDiffEditorController(DiffEditorController *controller)
{
    if (m_controller) {
        disconnect(m_controller, SIGNAL(ignoreWhitespaceChanged(bool)),
                   this, SLOT(requestReload()));
        disconnect(m_controller, SIGNAL(contextLinesNumberChanged(int)),
                   this, SLOT(requestReload()));
        disconnect(m_controller, SIGNAL(reloadRequested()),
                   this, SLOT(requestReload()));
    }

    m_controller = controller;

    if (m_controller) {
        connect(m_controller, SIGNAL(ignoreWhitespaceChanged(bool)),
                this, SLOT(requestReload()));
        connect(m_controller, SIGNAL(contextLinesNumberChanged(int)),
                this, SLOT(requestReload()));
        connect(m_controller, SIGNAL(reloadRequested()),
                this, SLOT(requestReload()));
    }
}

void DiffEditorReloader::requestReload()
{
    if (m_reloading)
        return;

    if (!m_controller)
        return;

    m_reloading = true;

    reload();
}

bool DiffEditorReloader::isReloading() const
{
    return m_reloading;
}

void DiffEditorReloader::reloadFinished()
{
    m_reloading = false;
}

} // namespace DiffEditor
