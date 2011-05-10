//! [0]
bool MyPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Core::BaseFileWizardParameters p(IWizard::FileWizard);
    p.setCategory(QLatin1String("P.web"));
    p.setDisplayCategory(tr("Web"));
    p.setDescription(tr("Creates a Web Page."));
    p.setDisplayName(tr("Web Page"));
    p.setId(QLatin1String("A.WebPage"));
    addAutoReleasedObject(new WebPageWizard(p));
}

//! [0]
