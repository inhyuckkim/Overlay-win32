#include "app.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    if (!App::instance().init(hInstance))
        return 1;

    int ret = App::instance().run();
    App::instance().shutdown();
    return ret;
}
