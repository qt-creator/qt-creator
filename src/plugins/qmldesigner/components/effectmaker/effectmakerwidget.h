// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/icontext.h>

#include <QFrame>

class StudioQuickWidget;

namespace QmlDesigner {

class EffectMakerView;
class EffectMakerModel;

class EffectMakerWidget : public QFrame
{
    Q_OBJECT

public:
    EffectMakerWidget(EffectMakerView *view);
    ~EffectMakerWidget() = default;

    void contextHelp(const Core::IContext::HelpCallback &callback) const;

    static QString qmlSourcesPath();
    void clearSearchFilter();

    void delayedUpdateModel();
    void updateModel();

    StudioQuickWidget *quickWidget() const;
    QPointer<EffectMakerModel> effectMakerModel() const;

    Q_INVOKABLE void focusSection(int section);


protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void reloadQmlSource();

    QPointer<EffectMakerModel> m_effectMakerModel;
    QPointer<EffectMakerView> m_effectMakerView;
    QPointer<StudioQuickWidget> m_quickWidget;
};

} // namespace QmlDesigner
