#include <SimpleWidget.h>
#include <nana/gui.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/place.hpp>
#include <nana/gui/widgets/combox.hpp>

using namespace nana;

std::optional<size_t> AskChooseOption(const char** optArr, size_t optArrCount)
{
    if (optArrCount < 1)
        return {};

    size_t selected = 0;
    form fm(API::make_center(280, 100)); fm.caption("Choose Option");
    combox options(fm);
    button btn(fm);
    place plc(fm);

    options.borderless(true);

    std::for_each(optArr, optArr + optArrCount, [&options](const char* opt) {
        options.push_back(opt);
        });

    options.option(selected);
    options.events().selected([&selected, &options] {
        selected = options.option();
        });

    btn.caption("Ok");
    btn.transparent(true);
    btn.events().click([&fm] {
        fm.close();
        });

    plc.div("vert <cbx weight=62%><btn weight=28%>");
    plc["cbx"] << options;
    plc["btn"] << btn;
    plc.collocate();

    fm.show();
    exec();

    return selected;
}