#include <exception>

#include "Core/Application.h"
#include "Core/Log.h"

int main(int, char**) {
    try {
        vajra::Application app;
        app.run();
    } catch (const std::exception& e) {
        VJ_ERROR("Fatal: %s", e.what());
        return 1;
    }
    return 0;
}
