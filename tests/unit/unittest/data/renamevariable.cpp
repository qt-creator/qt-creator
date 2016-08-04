int variable;

int x = variable + 3;

template <typename TemplateName>
class TemplateKlasse {
public:
    TemplateName value;
};

void function()
{
    TemplateKlasse<int> instance;

    instance.value += 5;

    auto l = [x=instance] {
        int k = x.value + 3;
    };
}
