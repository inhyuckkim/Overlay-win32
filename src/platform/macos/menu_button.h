#pragma once
#include <memory>

class MenuButton {
public:
    MenuButton();
    ~MenuButton();

    bool create();
    void destroy();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
