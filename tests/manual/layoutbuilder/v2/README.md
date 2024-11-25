
This is an ad-hoc instruction count to judge the overhead imposed
by Layoutbuilder for

    Label ll {
        text("World")
    };


(1) up to Label::Label
(2) after new Label to up to doit()
(3) tuple stuff in doit up to QLabel::setText parameter construction


With

    BuilderItem(IdAndArg<Id, Arg> && idarg)
    {
        apply = [&idarg](X *x) { doit(x, Id{}, idarg.arg); };
    }


    (1)   (2) + (3)    Sum
-O0 547   259 + 802   1608
-O2  24       23        47



With

    BuilderItem(IdAndArg<Id, Arg> && idarg)
        : apply([&idarg](X *x) { doit(x, Id{}, idarg.arg); })
    {}


    (1)   (2) + (3)    Sum
-O2  5        23        28
