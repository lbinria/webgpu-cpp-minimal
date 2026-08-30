#include "app.h"
#include <string_view>
#include <chrono>

int main(int argc, char *argv[]) {

    const std::string_view mode =
        argc > 1 ? std::string_view{argv[1]} : "";

    const bool ciTest = mode == "--ci-test";

    if (ciTest) {
        std::cout << "ci test" << std::endl;
    }

	App app;
	if (!app.init()) {
		return 1;
	}

    const auto start = std::chrono::steady_clock::now();
    constexpr auto timeout = std::chrono::seconds(2);

    while (app.isRunning()) {
        app.loop();

        if (ciTest &&
            std::chrono::steady_clock::now() - start >= timeout) {
            std::cout << "CI timeout reached" << std::endl;
            break;
        }
    }

    app.cleanup();

    return 0;
}