/* Copyright (C) 2014 David Zanetti
 *
 * This file is part of libkakapo.
 *
 * libkakapo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License.
 *
 * libkakapo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libkapapo.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

 /* This is not a complete NVM driver, but focussed on the limited
  * functions usually useful in an applicaiton, specifically the
  * user sig and prog sig rows */

#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stddef.h>
#include "global.h"
#include "nvm.h"

/* obtain the serial number from the prodsig row */
void nvm_serial(uint8_t *buf) {
    uint8_t sreg_save;

    if (!buf) {
        return;
    }
    /* as per datasheet, we disable interrupts over this */
    sreg_save = SREG;
    cli();
    NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
    buf[0] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,LOTNUM0));
    buf[1] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,LOTNUM1));
    buf[2] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,LOTNUM2));
    buf[3] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,LOTNUM3));
    buf[4] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,LOTNUM4));
    buf[5] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,LOTNUM5));
    buf[6] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,WAFNUM));
    buf[7] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,COORDX0));
    buf[8] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,COORDX1));
    buf[9] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,COORDY0));
    buf[10] = pgm_read_byte(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,COORDY1));
    NVM.CMD = NVM_CMD_NO_OPERATION_gc;
    SREG = sreg_save;
    return;
}

/* return the ADCA cal value. On a lot of shipping silicon this is meaningless,
 * but maybe it'll be useful one day. FIXME: also allow ADCB cal value */
uint16_t nvm_adccal(void) {
    uint16_t n;
    uint8_t sreg_save;

    /* as per datasheet, we disable interrupts over this */
    sreg_save = SREG;
    cli();
    NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
    n = pgm_read_word(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,ADCACAL0));
    NVM.CMD = NVM_CMD_NO_OPERATION_gc;
    SREG = sreg_save;

    return n;
}

/* return the temp sensor calibration value */
uint16_t nvm_tempcal(void) {
    uint16_t n;
    uint8_t sreg_save;

    /* as per datasheet, we disable interrupts over this */
    sreg_save = SREG;
    cli();
    NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
    n = pgm_read_word(PROD_SIGNATURES_START+offsetof(NVM_PROD_SIGNATURES_t,TEMPSENSE0));
    NVM.CMD = NVM_CMD_NO_OPERATION_gc;
    SREG = sreg_save;

    return n;
}

/* copy n bytes from the usersig row into the buffer */
uint16_t nvm_usersig(uint8_t *buf, uint16_t offset, uint16_t len) {
    uint16_t n = 0;
    uint8_t sreg_save;

    if (!buf || len == 0 || len > SPM_PAGESIZE) {
        return 0;
    }

    if (offset+len > SPM_PAGESIZE) {
        len -= ((offset+len) - SPM_PAGESIZE); /* cap it */
    }

    /* as per datasheet, we disable interrupts over this */
    sreg_save = SREG;
    cli();
    NVM.CMD = NVM_CMD_READ_USER_SIG_ROW_gc;
    for (n = 0; n < len; n++) {
        buf[n] = pgm_read_byte(n);
    }
    NVM.CMD = NVM_CMD_NO_OPERATION_gc;
    SREG = sreg_save;

    return len;
}
