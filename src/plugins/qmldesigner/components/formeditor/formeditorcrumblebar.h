/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMLDESIGNER_FORMEDITORCRUMBLEBAR_H
#define QMLDESIGNER_FORMEDITORCRUMBLEBAR_H

#include <QObject>
#include <utils/crumblepath.h>

namespace QmlDesigner {



class FormEditorCrumbleBar : public QObject
{
    Q_OBJECT
public:
    explicit FormEditorCrumbleBar(QObject *parent = 0);

    void pushFile(const QString &fileName);
    void pushInFileComponent(const QString &componentId);

    void nextFileIsCalledInternally();

    Utils::CrumblePath *crumblePath();

private slots:
    void onCrumblePathElementClicked(const QVariant &data);

private:
    bool m_isInternalCalled;
    Utils::CrumblePath *m_crumblePath;
};

class CrumbleBarInfo {
public:
    QString fileName;
    QString componentId;
};

bool operator ==(const CrumbleBarInfo &first, const CrumbleBarInfo &second);
bool operator !=(const CrumbleBarInfo &first, const CrumbleBarInfo &second);
} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CrumbleBarInfo)

#endif // QMLDESIGNER_FORMEDITORCRUMBLEBAR_H
