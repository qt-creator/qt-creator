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

#include "editorwindow.h"

#include "editorarea.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <QVBoxLayout>

namespace Core {
namespace Internal {

EditorWindow::EditorWindow(QWidget *parent) :
    QWidget(parent)
{
    m_area = new EditorArea;
    auto layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);
    layout->addWidget(m_area);
    setFocusProxy(m_area);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false); // don't prevent Qt Creator from closing
    resize(QSize(800, 600));

    static int windowId = 0;
    ICore::registerWindow(this, Context(Id("EditorManager.ExternalWindow.").withSuffix(++windowId)));
}

EditorArea *EditorWindow::editorArea() const
{
    return m_area;
}

} // Internal
} // Core
