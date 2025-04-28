accountFieldsVisible = function()
{
    var ifwVersion = installer.value("FrameworkVersion");
    if (installer.versionMatches(ifwVersion, "<4.10.0"))
        gui.clickButton("submitButtonLogin");
    else
        gui.clickButton(buttons.NextButton);
}

usageStatisticVisible = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.CredentialsPageCallback = function()
{
    var page = gui.currentPageWidget();
    page.accountFieldsVisible.connect(accountFieldsVisible)
    var ifwVersion = installer.value("FrameworkVersion");
    if (installer.versionMatches(ifwVersion, "<4.10.0"))
        page.usageStatisticVisible.connect(usageStatisticVisible);
}

Controller.prototype.IntroductionPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function()
{
    var component = installer.environmentVariable("QTC_MAINTENANCE_TOOL_COMPONENT");
    installer.selectComponent(component);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function()
{
    gui.clickButton(buttons.FinishButton);
}

function Controller()
{
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    })
}
