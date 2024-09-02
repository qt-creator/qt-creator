-- Copyright (C) 2024 The Qt Company Ltd.
-- SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
return {
    Id = "aiassistant",
    Name = "Qt AI Assistant",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    VendorId = "theqtcompany",
    Vendor = "The Qt Company",
    Category = "Language Client",
    Description = "Qt AI Assistant",
    Experimental = true,
    DisabledByDefault = true,
    LongDescription = [[
    Qt AI Assistant is a coding assistant. When connected to a Large Language
    Model (LLM), it auto-completes your code, as well as writes test cases and
    code documentation.

    Qt AI Assistant is available for selected commercial Qt developer
    license holders. For more information on licensing, select `Compare`
    in [Qt pricing](https://www.qt.io/pricing).

    > **Note:** Qt AI Assistant is LLM-agnostic. The subscription to a third-party
    LLM service or a third-party LLM for local or cloud deployment is not a part
    of it. You need to connect to a third-party LLM and agree to the terms and
    conditions, as well as to the acceptable use policy of the LLM provider. By
    using Qt AI Assistant, you agree to
    [Terms & Conditions - Qt Development Framework](https://www.qt.io/terms-conditions/qt-dev-framework).

    Qt AI Assistant is currently experimental and powered by generative AI. Check
    all suggestions to make sure that they are fit for use in your project.

    > **Note:** [Install and load](https://doc.qt.io/qtcreator/creator-how-to-load-extensions.html)
    the Qt AI Assistant extension to use it.

    ## Connect to a LLM

    You can connect to the following LLMs:
    - Meta Llama 3.1 70B (running in a cloud deployment of your choice
    - Anthropic Claude 3.5 Sonnet (provided as subscription-based service by Anthropic)

    To connect to a LLM:

    1. Go to `Preferences` > `AI Assistant`
    1. Select the use cases and programming languages to use the LLM for
    1. Enter the authentication token, user name, and API URL of the LLM.
       For more information on where to get the access information, see the
       third-party LLM provider documentation.

    ## Automatic code-completio

    Qt AI Assistant can help you write code by suggesting what to write next.
    It prompts the LLM to make one or several code suggestions based on the
    current cursor position and the code before and after the cursor when you
    stop typing. The code suggestions are shown after the cursor in grey color.

    To accept the entire suggestion, press the `Tab` key.

    To accept parts of the suggestions, press `Alt+Right`.

    To dismiss the suggestion, press `Esc` or navigate to another position in
    the code editor.

    To interact with Qt AI Assistant using the mouse, hover over the suggestion.

    When you hover over a suggestion, you can accept parts of the suggested code
    snippet word-by-word. Or, cycle through alternative suggestions in the code
    completion bar by selecting the `<` and `>` buttons.

    To close the code completion bar, press `Esc` key or move the cursor to
    another position.

    To turn auto-completion of code on or off globally for all projects, go to
    `Preferences` > `AI Assistant`. Qt AI Assistant consumes a significant number
    of tokens from the LLM. To cut costs, disable the auto-completion feature when
    not needed, and use keyboard shortcuts for code completion.

    ## Complete code from the keyboard

    To trigger code suggestions manually, press `Ctrl+Shift+A` (`Cmd+Shift+A` on macOS).

    ## Chat with the assistant

    In an inline chat window in the code editor, you can prompt the assistant to
    implement your requests in human language, ask questions, or execute
    *smart commands*. To open the chat, press `Ctrl+Shift+D` (`Cmd+Shift+D` on macOS).

    To close the chat, press `Esc` or select the `Close` button.

    To go to Qt AI Assistant preferences from the chat, select the `Settings` button.

    ### Request suggestions using human language

    To request suggestions using human language, type your requests in the chat.
    Qt AI Assistant shows a suggestion that you can copy to the clipboard by
    selecting the `Copy` button in the chat.

    ### Request test cases in Qt Test syntax

    To write test cases with Qt AI Assistant:

    - Highlight code in the code editor
    - Press `Ctrl+Shift+D` (`Cmd+Shift+D` on macOS) to open the chat
    - Select the `qtest` smart command.

    Qt AI Assistant generates a test case in [Qt Test](https://doc.qt.io/qt-6/qttest-index.html)
    format that you can copy and paste to your
    [Qt test project](https://doc.qt.io/qtcreator/creator-how-to-create-qt-tests.html.
    ]],
    Dependencies = {
        { Id = "lua",               Version = "14.0.0" },
        { Id = "lualanguageclient", Version = "14.0.0" }
    },
    hooks = {
        editors = {
            text = {
                contentsChanged = function(document, position, charsRemoved, charsAdded)
                                    require 'init'.Hooks.onDocumentContentsChanged(document, position, charsRemoved, charsAdded)
                                  end,
                currentChanged = function(editor)
                                    require 'init'.Hooks.onCurrentChanged(editor)
                                  end,
            }
        }
    },
    setup = function()
        require 'init'.setup()
    end,
} --[[@as QtcPlugin]]
