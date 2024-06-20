#include <iostream>
#include <SimpleWidget.h>

int main() {
    const char* options[]{
        "Foo",
        "Bar",
        "Baz",
        "Qux"
    };

    std::cout << AskChooseOption(options, ARR_SIZE(options)).value() << std::endl;
}

