/*
 * nvs_keymaps.h
 *
 *  Created on: 19 Sep 2018
 *      Author: gal
 */

#ifndef NVS_KEYMAPS_H_
#define NVS_KEYMAPS_H_
#include <keyboard_config.h>
#include "config.h"

//array for for holding the keymaps
extern uint16_t keymaps[LAYERS][MATRIX_ROWS][MATRIX_COLS];

//array for keymap names
extern char **layer_names_arr;

//amount of arrays
extern uint8_t layers_num;

#endif /* NVS_KEYMAPS_H_ */
