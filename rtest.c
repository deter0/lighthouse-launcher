#include <stdio.h>
#include "raylib.h"

int main(void) {
	InitWindow(800, 600, "Hello, World!");

	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(RED);
		EndDrawing();
	}

	CloseWindow();
}
