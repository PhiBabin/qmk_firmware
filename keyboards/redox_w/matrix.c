/* Copyright 2017 Mattia Dal Ben
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "quantum.h"
#include "matrix.h"
#include "uart.h"
#include "quantum/action.h"

#define UART_MATRIX_RESPONSE_TIMEOUT 10000

void matrix_init_custom(void) {
    uart_init(1000000);
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    uint32_t timeout = 0;
    bool changed = false;

    //the s character requests the RF slave to send the matrix
    uart_write('s');


    //trust the external keystates entirely, erase the last data
    uint8_t uart_data[PACKET_LEN] = {0};

    //there are 14 bytes corresponding to 14 columns, and an end byte
    for (uint8_t i = 0; i < PACKET_LEN; i++) {
        //wait for the serial data, timeout if it's been too long
        //this only happened in testing with a loose wire, but does no
        //harm to leave it in here
        while (!uart_available()) {
            timeout++;
            if (timeout > UART_MATRIX_RESPONSE_TIMEOUT) {
                break;
            }
        }

        if (timeout < UART_MATRIX_RESPONSE_TIMEOUT) {
            uart_data[i] = uart_read();
        } else {
            uart_data[i] = 0x00;
        }
    }

    //check for the end packet, the key state bytes use the LSBs, so 0xE0
    //will only show up here if the correct bytes were recieved
    if (uart_data[PACKET_LEN - 1] == 0xE0)
    {
        //shifting and transferring the keystates to the QMK matrix variable
        for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
            matrix_row_t current_row = (uint16_t) uart_data[i * 2] | (uint16_t) uart_data[i * 2 + 1] << 7;
            if (current_matrix[i] != current_row) {
                changed = true;
            }
            current_matrix[i] = current_row;
        }
        for (size_t i = 0; i < 2; ++i)
        {
            const uint8_t new_value = uart_data[2 * MATRIX_ROWS + i];
            if (new_value != encoder_values[i])
            {
                const uint8_t old_value = encoder_values[i];

                encoder_values[i] = new_value;
                dprintf("encL: %d encR: %d", encoder_values[0]%20, encoder_values[1]%20);

                const uint16_t diff = (uint16_t)new_value - (uint16_t)old_value;
                print("\n");
                if (diff <= 256/2) {
                    // tap_code(KC_VOLU);
                    dprintf("+1 => %d", diff);
                } else {
                    // tap_code(KC_VOLD);
                    dprintf("-1 => %d", diff);
                }
                print("\n");
                // if (new_value < encoder_values[i])
                // {
                //     tap_code(KC_VOLD);
                // }
                // else
                // {
                //     tap_code(KC_VOLU);
                // }
            }
        }
    }

    return changed;
}
