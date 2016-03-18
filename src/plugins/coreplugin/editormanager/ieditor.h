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

#include <coreplugin/core_global.h>
#include <coreplugin/icontext.h>

#include <QMetaType>

namespace Core {

class IDocument;

class CORE_EXPORT IEditor : public IContext
{
    Q_OBJECT

public:
    IEditor(QObject *parent = 0);

    bool duplicateSupported() const;
    void setDuplicateSupported(bool duplicateSupported);

    virtual IDocument *document() = 0;

    virtual IEditor *duplicate() { return 0; }

    virtual QByteArray saveState() const { return QByteArray(); }
    virtual bool restoreState(const QByteArray &/*state*/) { return true; }

    virtual int currentLine() const { return 0; }
    virtual int currentColumn() const { return 0; }
    virtual void gotoLine(int line, int column = 0, bool centerLine = true) { Q_UNUSED(line) Q_UNUSED(column) Q_UNUSED(centerLine) }

    virtual QWidget *toolBar() = 0;

    virtual bool isDesignModePreferred() const { return false; }

private:
    bool m_duplicateSupported;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::IEditor*)
