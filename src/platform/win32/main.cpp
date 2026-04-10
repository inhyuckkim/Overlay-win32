#include "app.h"
#include <ixwebsocket/IXNetSystem.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    ix::initNetSystem();

    if (!App::instance().init(hInstance)) {
        ix::uninitNetSystem();
        return 1;
    }

    int ret = App::instance().run();
    App::instance().shutdown();
    ix::uninitNetSystem();
    return ret;
}
