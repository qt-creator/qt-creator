accountFieldsVisible = function()
{
    var ifwVersion = installer.value("FrameworkVersion");
    if (installer.versionMatches(ifwVersion, "=4.9.0"))
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
    if (installer.versionMatches(ifwVersion, "=4.9.0"))
        page.usageStatisticVisible.connect(usageStatisticVisible);
}

Controller.prototype.IntroductionPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function()
{
    var componentString = installer.environmentVariable("QTC_MAINTENANCE_TOOL_COMPONENT");
    var componentList = componentString.split(";");
    for (var idx = 0; idx < componentList.length; idx++) {
        installer.selectComponent(componentList[idx]);
    }
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
