#include "app.h"

int main() {

	App app;
	if (!app.init()) {
		return 1;
	}

    while (app.isRunning()) {
        app.loop();
    }

    app.cleanup();

    return 0;
}