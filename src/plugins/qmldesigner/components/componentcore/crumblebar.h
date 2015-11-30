/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef QMLDESIGNER_FORMEDITORCRUMBLEBAR_H
#define QMLDESIGNER_FORMEDITORCRUMBLEBAR_H

#include <QObject>
#include <utils/crumblepath.h>
#include <modelnode.h>

namespace QmlDesigner {

class CrumbleBar : public QObject
{
    Q_OBJECT
public:
    explicit CrumbleBar(QObject *parent = 0);
    ~CrumbleBar();

    void pushFile(const QString &fileName);
    void pushInFileComponent(const ModelNode &modelNode);

    void nextFileIsCalledInternally();

    Utils::CrumblePath *crumblePath();

private slots:
    void onCrumblePathElementClicked(const QVariant &data);

private:
    void updateVisibility();
    void showSaveDialog();

private:
    bool m_isInternalCalled;
    Utils::CrumblePath *m_crumblePath;
};

class CrumbleBarInfo {
public:
    QString fileName;
    QString displayName;
    ModelNode modelNode;
};

bool operator ==(const CrumbleBarInfo &first, const CrumbleBarInfo &second);
bool operator !=(const CrumbleBarInfo &first, const CrumbleBarInfo &second);
} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CrumbleBarInfo)

#endif // QMLDESIGNER_FORMEDITORCRUMBLEBAR_H
