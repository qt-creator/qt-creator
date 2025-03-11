local function tst_markdownBrowser()
    G = require 'Gui'

    G.Column {
        G.MarkdownBrowser {
            enableCodeCopyButton = true,
            markdown = [[# Markdown Browser Test

## Code Snippets

* Is the following code formatted correctly, and is the syntax highlighting working?
* Can you press the copy button and paste the code into a text editor?
* Is there a copy icon visible for the code snippet?

```c++
#include <print>

int main() {
  std::print("Hello World!");
}
```

## Links

* [Is this a link to the Qt website?](https://www.qt.io)
* [Is this an anchor link that scrolls up to the top?](#markdown-browser-test)

]]
        }
    }:show()
end


local function setup()
    Action = require 'Action'
    Action.create("LuaTests.markdownBrowserDemo", {
        text = "Lua Markdown Browser Test",
        onTrigger = tst_markdownBrowser,
    })
end

return {
    setup = setup,
}
