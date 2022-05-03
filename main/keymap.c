#ifndef KEYMAP_C
#define KEYMAP_C

#include "key_definitions.h"
#include "keyboard_config.h"
#include "keymap.h"
#include "plugins.h"

// A bit different from QMK, default returns you to the first layer, LOWER and raise increase/lower layer by order.
#define DEFAULT 0x100
#define LOWER 0x101
#define RAISE 0x102

// Keymaps are designed to be relatively interchangeable with QMK
enum custom_keycodes {
	QWERTY, NUM,
    PLUGINS,
};

//Set these for each layer and use when layers are needed in a hold-to use layer
enum layer_holds {
	QWERTY_H = LAYER_HOLD_BASE_VAL, NUM_H,FUNCS_H
};

// array to hold names of layouts for oled
char default_layout_names[LAYERS][MAX_LAYOUT_NAME_LENGTH] = { "QWERTY", "NUM",
		  "Plugins",
		};

/* select a keycode for your macro
 * important - first macro must be initialized as MACRO_BASE_VAL
 * */

#define MACROS_NUM 2
enum custom_macros {
	KC_CTRL_ALT_DELETE = MACRO_BASE_VAL, KC_ALT_F4,
};

/*define what the macros do
 * important- make sure you you put the macros in the same order as the their enumeration
 */
uint16_t macros[MACROS_NUM][MACRO_LEN] = {
		// CTRL+ALT+DEL
		{ KC_LCTRL, KC_LALT, KC_DEL },
		//ALT +F4
		{ KC_RALT, KC_LALT, KC_NO } };

/*Encoder keys for each layer by order, and for each pad
 * First variable states what usage the encoder has
 */

uint16_t default_encoder_map[LAYERS][ENCODER_SIZE] = {
		// |VOL + | VOL - | MUTE |
		{ MEDIA_ENCODER, KC_AUDIO_VOL_UP, KC_AUDIO_VOL_DOWN, KC_AUDIO_MUTE },
		// |Y+|Y-| LEFT CLICK|
		{ MOUSE_ENCODER, KC_MS_UP, KC_MS_DOWN, KC_MS_BTN1 },

		{ MOUSE_ENCODER, KC_MS_UP, KC_MS_DOWN, KC_MS_BTN1 } };

uint16_t default_slave_encoder_map[LAYERS][ENCODER_SIZE] = {
		// |VOL + | VOL - | MUTE |
		{ MOUSE_ENCODER, KC_MS_WH_UP, KC_MS_WH_DOWN	, KC_AUDIO_MUTE },
		// |Y+|Y-| LEFT CLICK|
		{ MOUSE_ENCODER, KC_MS_RIGHT, KC_MS_LEFT, KC_MS_BTN2 },
		// |Y+|Y-| LEFT CLICK|
		{ MOUSE_ENCODER, KC_MS_RIGHT, KC_MS_LEFT, KC_MS_BTN2 } };

// Fillers to make layering more clear
#define _______ KC_TRNS
#define XXXXXXX KC_NO

//NOTE: For this keymap due to wiring constraints the the two last rows on the left are wired unconventionally
// Each keymap is represented by an array, with an array that points to all the keymaps  by order
	 uint16_t _QWERTY[MATRIX_ROWS][KEYMAP_COLS]={

			/* Qwerty
			 * ,-----------------------------------------.    .-----------------------------------------.
			 * |  ~   |   1  |   2  |   3   |   4  |   5  |   |    6  |   7  |   8  |   9  |   0  |   -  |
			 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
			 * | Esc  |   Q  |   W  |   E   |   R  |   T  |   |    Y  |   U  |   I  |   O  |   P  |  =   |
			 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
			 * | Tab  |   A  |   S  |   D   |   F  |   G  |   |    H  |   J  |   K  |   L  |   ;  |  '   |
			 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
			 * | Shift|   Z  |   X  |   C   |   V  |   B  |   |    N  |   M  |   ,  |   .  |   /  |  up  |
			 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
			 * | Ctrl | GUI  |  Alt | Space |Space |NUM_H |   |  Del  |Bspc  |Enter | Left | Right  |Down  |
			 * `------------------------------------------'    ------------------------------------------'
			 *                            |default |RAISE|    |lower|NUM |
			 *                              --------------    ---------------
			 */
			  {KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINUS },
			  {KC_ESC,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_EQUAL},
			  {KC_TAB,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT },
			  {KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_UP } ,
			  {KC_LCTRL,KC_LGUI, KC_LALT, KC_SPC, KC_SPC,  NUM_H,  KC_DEL,  NUM,   LOWER, KC_BSPC, KC_ENT,  KC_LEFT },
			  {DEFAULT,RAISE, KC_NONE, KC_NONE, KC_NONE,   KC_NONE,  KC_RIGHT,  KC_DOWN,   KC_NONE, KC_NONE, KC_NONE,   KC_NONE }

	};

	 uint16_t _NUM[MATRIX_ROWS][KEYMAP_COLS]={


			/* Nums
			 * ,-----------------------------------------.    .-----------------------------------------.
			 * | F1   |   F2 |   F3 |   F4  |  F5  |  F6  |   |   F7  |  F8  |  F9  |  F10 | F11  |  F12 |
			 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
			 * | Esc  |   Q  |   W  |   E   |   R  |   T  |   |    Y  |   U  |   I  |   [ |   ]   |  \   |
			 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
			 * | Tab  |   A  |   S  |   D   |   F  |   G  |   |    H  |   J  |   K  |   L  |   ;  |  '   |
			 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
			 * | Shift|   Z  |   X  |   C   |   V  |   B  |   |    N  |   M  |   ,  |   .  |   /  |  up  |
			 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
			 * | Ctrl | GUI  |  Alt | Space |Space |NUM_H |   |  Del  |Bspc  |Enter | Left | Right  |Down  |
			 * `------------------------------------------'    ------------------------------------------'
			 *                            |default |RAISE|    |lower|NUM |
			 *                              --------------    ---------------
			 */
			  { KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9, KC_F10,KC_F11,  KC_F12 },
			  {KC_ESC,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_LBRC,    KC_RBRC, KC_BSLASH },
			  {KC_TAB,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT },
			  {KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_EQUAL } ,
			  {KC_LCTRL,KC_LGUI, KC_LALT, KC_SPC, KC_SPC,  NUM_H,  KC_DEL,  NUM,   LOWER, KC_BSPC, KC_ENT,  KC_LEFT },
			  {DEFAULT,RAISE, KC_NONE, KC_NONE, KC_NONE,   KC_NONE,  KC_RIGHT,  KC_DOWN,   KC_NONE, KC_NONE, KC_NONE,   KC_NONE }
	};

	 uint16_t _PLUGINS[MATRIX_ROWS][KEYMAP_COLS]={


				/* Plugins
				 * ,-----------------------------------------.    .-----------------------------------------.
				 * |  ~   |   1  |   2  |   3   |   4  |   5  |   |    6  |   7  |   8  |   9  |   0  |   -  |
				 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
				 * | Esc  |   Q  |   W  |   E   |   R  |   T  |   |    Y  |   U  |   I  |   O  |   P  |  =   |
				 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
				 * | Tab  |   A  |   S  |   D   |   F  |   G  |   |    H  |   J  |   K  |   L  |   ;  |  '   |
				 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
				 * | Shift|   Z  |   X  |   C   |   V  |   B  |   |    N  |   M  |   ,  |   .  |   /  |  up  |
				 * |------+------+------+-------+------+------|   |-------+------+------+------+------+------|
				 * | Ctrl | GUI  |  Alt | Space |Space |NUM_H |   |  Del  |Bspc  |Enter | Left | Right  |Down  |
				 * `------------------------------------------'    ------------------------------------------'
				 *                            |default |RAISE|    |lower|NUM |
				 *                              --------------    ---------------
				 */
				  {KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC },
				  {KC_ESC,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSPC },
				  {KC_TAB,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT },
				  {KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_ENT } ,
				  {KC_LCTRL,KC_LGUI, KC_LALT, KC_SPC, KC_SPC,  NUM_H,  KC_DEL,  NUM,   LOWER, KC_BSPC, KC_ENT,  KC_LEFT },
				  {DEFAULT,RAISE, KC_NONE, KC_NONE, KC_NONE,   KC_NONE,  KC_RIGHT,  KC_DOWN,   KC_NONE, KC_NONE, KC_NONE,   KC_NONE }

		};
 //Create an array that points to the various keymaps
uint16_t (*default_layouts[])[MATRIX_ROWS][KEYMAP_COLS] = { &_QWERTY, &_NUM,
			&_PLUGINS
		};

uint8_t current_layout = 0;

const char* key_code_name[256] = {
	"NO",
	"ROLL_OVER",
	"POST_FAIL",
	"UNDEFINED",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M", /* 0x10 */
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"1",
	"2",
	"3", /* 0x20 */
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"0",
	"ENTER",
	"ESCAPE",
	"BSPACE",
	"TAB",
	"SPACE",
	"MINUS",
	"EQUAL",
	"LBRACKET",
	"RBRACKET", /* 0x30 */
	"BSLASH", /* \ (and |) */
	"NONUS_HASH", /* Non-US # and ~ (Typically near the Enter key) */
	"SCOLON", /* ; (and :) */
	"QUOTE", /* ' and " */
	"GRAVE", /* Grave accent and tilde */
	"COMMA", /* ", and < */
	"DOT", /* . and > */
	"SLASH", /* / and ? */
	"CAPSLOCK",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7", /* 0x40 */
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"PSCREEN",
	"SCROLLLOCK",
	"PAUSE",
	"INSERT",
	"HOME",
	"PGUP",
	"DELETE",
	"END",
	"PGDOWN",
	"RIGHT",
	"LEFT", /* 0x50 */
	"DOWN",
	"UP",
	"NUMLOCK",
	"KP_SLASH",
	"KP_ASTERISK",
	"KP_MINUS",
	"KP_PLUS",
	"KP_ENTER",
	"KP_1",
	"KP_2",
	"KP_3",
	"KP_4",
	"KP_5",
	"KP_6",
	"KP_7",
	"KP_8", /* 0x60 */
	"KP_9",
	"KP_0",
	"KP_DOT",
	"NONUS_BSLASH", /* Non-US \ and | (Typically near the Left-Shift key) */
	"APPLICATION",
	"POWER",
	"KP_EQUAL",
	"F13",
	"F14",
	"F15",
	"F16",
	"F17",
	"F18",
	"F19",
	"F20",
	"F21", /* 0x70 */
	"F22",
	"F23",
	"F24",
	"EXECUTE",
	"HELP",
	"MENU",
	"SELECT",
	"STOP",
	"AGAIN",
	"UNDO",
	"CUT",
	"COPY",
	"PASTE",
	"FIND",
	"MUTE",
	"VOLUP", /* 0x80 */
	"VOLDOWN",
	"LOCKING_CAPS", /* locking Caps Lock */
	"LOCKING_NUM", /* locking Num Lock */
	"LOCKING_SCROLL", /* locking Scroll Lock */
	"KP_COMMA",
	"KP_EQUAL_AS400", /* equal sign on AS/400 */
	"INT1",
	"INT2",
	"INT3",
	"INT4",
	"INT5",
	"INT6",
	"INT7",
	"INT8",
	"INT9",
	"LANG1", /* 0x90 */
	"LANG2",
	"LANG3",
	"LANG4",
	"LANG5",
	"LANG6",
	"LANG7",
	"LANG8",
	"LANG9",
	"ALT_ERASE",
	"SYSREQ",
	"CANCEL",
	"CLEAR",
	"PRIOR",
	"RETURN",
	"SEPARATOR",
	"OUT", /* 0xA0 */
	"OPER",
	"CLEAR_AGAIN",
	"CRSEL",
	"EXSEL", /* 0xA4 */
    "","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","","",
	/* Modifiers */
	"LCTRL",
	"LSHIFT",
	"LALT",
	"LGUI",
	"RCTRL",
	"RSHIFT",
	"RALT",
	"RGUI",

/* NOTE: 0xE8-FF are used for internal special purpose */
    "","","","","","","","","","","","","","","","","","","","","","","","",
};

#endif

