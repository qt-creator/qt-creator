# Maintaining the Welcome page tips

Use this prompt when adding tips for a feature or updating existing tips.

> Read the relevant Qt Creator documentation under `doc/qtcreator/src` and
> the feature's implementation. Update `tips.json` with short, actionable tips
> about things users can do in Qt Creator. Keep existing tips in order and
> avoid duplicates. Use a short title and one or two plain-text sentences.
>
> Give each entry a `helpUrl`. Prefer
> `qthelp://org.qt-project.qtcreator/doc/<page>.html` and verify the page
> against its QDoc page name. Add a fragment when it points users to a
> specific section. Use a public HTTPS URL for online-only material.
>
> Add `settingsPageId` only when the tip points to a specific Settings page.
> Verify the ID against the page's registration in the source. Check any
> shortcuts against the registered commands and note platform differences
> when relevant. Keep the JSON valid and use UTF-8.

Run the Learning plugin tests after editing the catalog. Open the Welcome
overview and use **Next Tip** to check **Read More** and **Open Settings**
where applicable. CMake and Qbs embed the catalog into the Learning plugin.
