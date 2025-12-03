// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercomponents_global.h>

#include <coreplugin/icontext.h>

#include <abstractview.h>
#include <widgetregistration.h>

#include <utils/filepath.h>

#include <QWidgetAction>

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class AbstractCustomTool;
class DesignDocument;
class DesignerActionManager;
class Edit3DView;
class NodeInstanceView;
class PropertyEditorView;
class RewriterView;
class TextEditorView;

namespace Internal { class DesignModeWidget; }

class ViewManagerData;

class QMLDESIGNERCOMPONENTS_EXPORT ViewManager : private WidgetRegistrationInterface
{
public:
    ViewManager(class AsynchronousImageCache &imageCache,
                class ExternalDependenciesInterface &externalDependencies,
                ModulesStorage &modulesStorage);
    ~ViewManager();
    void attachRewriterView();
    void detachRewriterView();

    void attachComponentView();
    void detachComponentView();

    void attachViewsExceptRewriterAndComponetView();
    void detachViewsExceptRewriterAndComponetView();

    void setComponentNode(const ModelNode &componentNode);
    void setComponentViewToMaster();
    void setNodeInstanceViewTarget(ProjectExplorer::Target *target);
    void resetPropertyEditorView();

    void registerFormEditorTool(std::unique_ptr<AbstractCustomTool> &&tool);
    template<typename View>
    View *registerView(std::unique_ptr<View> &&view)
    {
        auto notOwningPointer = view.get();
        addView(std::move(view));
        return notOwningPointer;
    }
    QList<WidgetInfo> widgetInfos();

    void disableWidgets();
    void enableWidgets();
    void removeExtraView(WidgetInfo info);
    void pushFileOnCrumbleBar(const ::Utils::FilePath &fileName);
    void pushInFileComponentOnCrumbleBar(const ModelNode &modelNode);
    void nextFileIsCalledInternally();

    const AbstractView *view() const;
    AbstractView *view();

    PropertyEditorView *propertyEditorView() const;
    TextEditorView *textEditorView();

    void emitCustomNotification(const QString &identifier, const QList<ModelNode> &nodeList,
                                const QList<QVariant> &data);

    void exportAsImage();
    QImage takeFormEditorScreenshot();
    void reformatFileUsingTextEditorView();

    QWidgetAction *componentViewAction() const;
    QAction *propertyEditorUnifiedAction() const;

    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;

    void qmlJSEditorContextHelp(const Core::IContext::HelpCallback &callback) const;
    DesignDocument *currentDesignDocument() const;

    bool usesRewriterView(RewriterView *rewriterView);

    void disableStandardViews();
    void enableStandardViews();
    void jumpToCodeInTextEditor(const ModelNode &modelNode);
    QList<AbstractView *> views() const;
    AbstractView *findView(const QString &uniqueId);

    void hideView(AbstractView &view);
    void showView(AbstractView &view);
    void removeView(AbstractView &view);
    void hideSingleWidgetTitleBars(const QString &uniqueId);
    void initializeWidgetInfos();

private: // functions
    Q_DISABLE_COPY(ViewManager)

    void addView(std::unique_ptr<AbstractView> &&view);

    void attachNodeInstanceView();
    void attachAdditionalViews();
    void detachAdditionalViews();
    void detachStandardViews();

    Model *currentModel() const;
    Model *documentModel() const;
    QString pathToQt() const;

    void switchStateEditorViewToBaseState();
    void switchStateEditorViewToSavedState();
    QList<AbstractView *> standardViews() const;

    void registerNanotraceActions();

    void registerViewActions();
    void registerViewAction(AbstractView &view);
    void enableView(AbstractView &view);

    void disableView(AbstractView &view);

    void registerWidgetInfo(WidgetInfo info) override;
    void deregisterWidgetInfo(WidgetInfo info) override;
    void showExtraWidget(WidgetInfo info) override;
    void hideExtraWidget(WidgetInfo info) override;
    void removeExtraWidget(WidgetInfo info) override;

private: // variables
    std::unique_ptr<ViewManagerData> d;
    QList<WidgetInfo> m_widgetInfo;
};

} // namespace QmlDesigner
