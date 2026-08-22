#include <nds.h>
#include <nds/ndstypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192

#define MAX_ORDERS 5
#define MAX_PATTIES 3

typedef enum {
	STATE_MENU,
	STATE_COOKING,
	STATE_GAME_OVER
} GameState;

typedef struct {
	int patties;
	int lettuce;
	int tomato;
	int onion;
	int cheese;
} Burger;

typedef struct {
	int customer_id;
	Burger order;
	int wait_time;
	int max_wait;
	int satisfied;
} Order;

typedef struct {
	GameState state;
	Order orders[MAX_ORDERS];
	int active_orders;
	Burger current_burger;
	int money;
	int time_left;
	int level;
	int score;
	int frame_count;
} Game;

// Touch screen button areas (bottom screen)
typedef struct {
	int x;
	int y;
	int width;
	int height;
	const char *label;
} TouchButton;

Game game;

// Define touch buttons for burger assembly
TouchButton buttons[] = {
	{10, 50, 50, 40, "Patty"},
	{70, 50, 50, 40, "Lettuce"},
	{130, 50, 50, 40, "Tomato"},
	{190, 50, 50, 40, "Cheese"},
	{10, 100, 50, 40, "Onion"},
	{70, 100, 50, 40, "Serve"},
	{130, 100, 50, 40, "New Order"},
	{190, 100, 50, 40, "Start"},
};

#define NUM_BUTTONS (sizeof(buttons) / sizeof(TouchButton))

void drawSimpleSprite(int x, int y, int width, int height, u16 color) {
	int *bg = (int*)BG_BMP_RAM(0);
	for(int py = 0; py < height; py++) {
		for(int px = 0; px < width; px++) {
			int screenX = x + px;
			int screenY = y + py;
			if(screenX >= 0 && screenX < SCREEN_WIDTH && screenY >= 0 && screenY < SCREEN_HEIGHT) {
				bg[screenY * SCREEN_WIDTH + screenX] = color;
			}
		}
	}
}

void drawBurgerGraphic(int x, int y, Burger *burger) {
	// Draw burger components as simple colored rectangles
	int offsetY = y;
	
	// Bottom bun
	drawSimpleSprite(x + 10, offsetY, 30, 8, RGB15(31, 20, 0)); // Brown
	offsetY += 10;
	
	// Patties
	for(int i = 0; i < burger->patties; i++) {
		drawSimpleSprite(x + 8, offsetY, 34, 6, RGB15(20, 10, 0)); // Dark brown patty
		offsetY += 8;
		
		if(burger->lettuce && i == 0) {
			drawSimpleSprite(x + 5, offsetY, 40, 4, RGB15(0, 31, 0)); // Green lettuce
			offsetY += 5;
		}
		if(burger->tomato && i == 1) {
			drawSimpleSprite(x + 6, offsetY, 38, 4, RGB15(31, 0, 0)); // Red tomato
			offsetY += 5;
		}
		if(burger->cheese && i == 0) {
			drawSimpleSprite(x + 7, offsetY, 36, 3, RGB15(31, 31, 0)); // Yellow cheese
			offsetY += 4;
		}
	}
	
	if(burger->onion) {
		drawSimpleSprite(x + 6, offsetY, 38, 4, RGB15(31, 15, 0)); // Orange onion
		offsetY += 5;
	}
	
	// Top bun
	drawSimpleSprite(x + 10, offsetY, 30, 8, RGB15(31, 20, 0)); // Brown
}

void drawOrderDisplay(int x, int y, Order *order) {
	// Draw customer's order as a burger
	consoleInit(NULL, 0, BgType_Text4bpp, BgFormat_Rot, 0, false, true);
	iprintf("\x1b[%d;%dH", y/8, x/6);
	iprintf("Customer Order:");
	
	drawBurgerGraphic(x, y + 20, &order->order);
}

void drawCurrentBurger(int x, int y) {
	consoleInit(NULL, 0, BgType_Text4bpp, BgFormat_Rot, 0, false, true);
	iprintf("\x1b[%d;%dH", y/8, x/6);
	iprintf("Your Burger:");
	
	drawBurgerGraphic(x, y + 20, &game.current_burger);
}

void drawTouchButtons(PrintConsole *console) {
	printf("\x1b[20;0H"); // Position cursor
	printf("====== CONTROLS ======");
	
	for(int i = 0; i < NUM_BUTTONS; i++) {
		printf("\x1b[%d;%dH", 21 + i, 0);
		printf("[%s]", buttons[i].label);
	}
}

void initGame() {
	game.state = STATE_MENU;
	game.active_orders = 0;
	game.money = 0;
	game.time_left = 60;
	game.level = 1;
	game.score = 0;
	game.frame_count = 0;
	memset(&game.current_burger, 0, sizeof(Burger));
}

void generateOrder() {
	if(game.active_orders >= MAX_ORDERS) return;
	
	Order *order = &game.orders[game.active_orders];
	order->customer_id = game.active_orders;
	order->wait_time = 0;
	order->max_wait = 30 - (game.level * 2);
	if(order->max_wait < 10) order->max_wait = 10;
	
	// Random burger generation
	order->order.patties = (rand() % 3) + 1;
	order->order.lettuce = rand() % 2;
	order->order.tomato = rand() % 2;
	order->order.onion = rand() % 2;
	order->order.cheese = rand() % 2;
	order->satisfied = 0;
	
	game.active_orders++;
}

void serveBurger() {
	if(game.active_orders == 0) return;
	
	Order *order = &game.orders[0];
	
	// Check if burger matches order
	if(order->order.patties == game.current_burger.patties &&
	   order->order.lettuce == game.current_burger.lettuce &&
	   order->order.tomato == game.current_burger.tomato &&
	   order->order.onion == game.current_burger.onion &&
	   order->order.cheese == game.current_burger.cheese) {
		
		// Correct burger!
		int bonus = (order->max_wait - order->wait_time) * 10;
		game.money += 100 + bonus;
		game.score += 100 + bonus;
		order->satisfied = 1;
	} else {
		// Wrong burger
		game.money -= 50;
		if(game.money < 0) game.money = 0;
	}
	
	// Remove served order
	for(int i = 0; i < game.active_orders - 1; i++) {
		game.orders[i] = game.orders[i+1];
	}
	game.active_orders--;
	
	// Reset burger
	memset(&game.current_burger, 0, sizeof(Burger));
}

void updateGame() {
	game.frame_count++;
	
	// Update every 60 frames (1 second)
	if(game.frame_count >= 60) {
		game.frame_count = 0;
		
		// Update order wait times
		for(int i = 0; i < game.active_orders; i++) {
			game.orders[i].wait_time++;
			
			// Order timeout
			if(game.orders[i].wait_time >= game.orders[i].max_wait) {
				game.money -= 75;
				if(game.money < 0) game.money = 0;
				
				// Remove order
				for(int j = i; j < game.active_orders - 1; j++) {
					game.orders[j] = game.orders[j+1];
				}
				game.active_orders--;
				i--;
			}
		}
		
		game.time_left--;
		
		if(game.time_left <= 0) {
			game.state = STATE_GAME_OVER;
		}
	}
}

void handleTouchInput() {
	touchRead(&touch);
	
	if(touch.px > 0 && touch.py > 0) {
		// Check which button was touched
		if(touch.px > 10 && touch.px < 60 && touch.py > 50 && touch.py < 90) {
			// Patty button
			if(game.current_burger.patties < MAX_PATTIES) {
				game.current_burger.patties++;
			}
		}
		else if(touch.px > 70 && touch.px < 120 && touch.py > 50 && touch.py < 90) {
			// Lettuce button
			game.current_burger.lettuce = !game.current_burger.lettuce;
		}
		else if(touch.px > 130 && touch.px < 180 && touch.py > 50 && touch.py < 90) {
			// Tomato button
			game.current_burger.tomato = !game.current_burger.tomato;
		}
		else if(touch.px > 190 && touch.px < 240 && touch.py > 50 && touch.py < 90) {
			// Cheese button
			game.current_burger.cheese = !game.current_burger.cheese;
		}
		else if(touch.px > 10 && touch.px < 60 && touch.py > 100 && touch.py < 140) {
			// Onion button
			game.current_burger.onion = !game.current_burger.onion;
		}
		else if(touch.px > 70 && touch.px < 120 && touch.py > 100 && touch.py < 140) {
			// Serve button
			serveBurger();
		}
		else if(touch.px > 130 && touch.px < 180 && touch.py > 100 && touch.py < 140) {
			// New Order button
			generateOrder();
		}
		else if(touch.px > 190 && touch.px < 240 && touch.py > 100 && touch.py < 140) {
			// Start Game button
			if(game.state == STATE_MENU) {
				game.state = STATE_COOKING;
				game.time_left = 60;
				game.money = 0;
				game.score = 0;
				generateOrder();
			}
		}
	}
}

void drawMenu() {
	consoleClear();
	printf("\n\n\n");
	printf("      PAPA'S BURGERIA DS\n");
	printf("      ==================\n\n\n");
	printf("      Touch \"Start\" to play\n");
	printf("      Level: %d | Money: $%d\n", game.level, game.money);
}

void drawGameScreen() {
	consoleClear();
	printf("PAPA'S BURGERIA - Level %d\n", game.level);
	printf("Time: %d | Money: $%d | Score: %d\n", game.time_left, game.money, game.score);
	printf("Active Orders: %d\n\n", game.active_orders);
	
	if(game.active_orders > 0) {
		printf("Customer waiting: %d/%d seconds\n", 
			game.orders[0].wait_time, game.orders[0].max_wait);
	}
	
	// Draw touch button hints
	drawTouchButtons(NULL);
}

int main(void) {
	powerOn(POWER_ALL);
	videoSetMode(MODE_5_2D);
	videoSetModeSub(MODE_5_2D);
	
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	
	BG3_CR = BG_BMP8_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(3);
	BG3_XDX = 1 << 8;
	BG3_XDY = 0;
	BG3_YDX = 0;
	BG3_YDY = 1 << 8;
	BG3_CX = 0;
	BG3_CY = 0;
	
	// Initialize console for text on top screen
	consoleInit(NULL, 0, BgType_Text4bpp, BgFormat_Rot, 0, false, true);
	consoleInit(NULL, 1, BgType_Text4bpp, BgFormat_Rot, 0, false, false);
	
	initGame();
	
	while(1) {
		swiWaitForVBlank();
		handleTouchInput();
		
		if(game.state == STATE_MENU) {
			drawMenu();
		}
		else if(game.state == STATE_COOKING) {
			updateGame();
			drawGameScreen();
			
			// Draw graphics on bottom screen
			if(game.active_orders > 0) {
				drawOrderDisplay(10, 10, &game.orders[0]);
			}
			drawCurrentBurger(100, 80);
		}
		else if(game.state == STATE_GAME_OVER) {
			consoleClear();
			printf("\n\n\n");
			printf("        GAME OVER!\n");
			printf("        ==========\n\n");
			printf("  Final Score: %d\n", game.score);
			printf("  Money Earned: $%d\n", game.money);
			printf("  Level Reached: %d\n\n", game.level);
			printf("  Touch \"Start\" to continue\n");
		}
	}
	
	return 0;
}
