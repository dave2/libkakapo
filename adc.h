/* Copyright (C) 2013-2014 David Zanetti
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

/* ADC public interface */

#ifndef ADC_H_INCLUDED
#define ADC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \file
 *  \brief XMEGA ADC Public Interface
 */

/** \brief ADC conversion modes
 */
typedef enum {
	adc_unsigned, /**< ADC should be unsigned mode */
	adc_signed, /**< ADC should be signed mode */
} adc_mode_t;

/** \brief ADC reference sources
 */
typedef enum {
	adc_vref_int1v, /**< ADC VREF = Internal 1V */
	adc_vref_intvcc16, /**< ADC VREF = Internal VCC/1.6 */
	adc_vref_arefa, /**< ADC VREF = External AREFA pin */
	adc_vref_arefb, /**< ADC VREF = External AREFB pin */
	adc_vref_intvcc2, /**< ADC VREF = Internal VCC/2 */
} adc_vref_t;

/** \brief ADC prescaler clock
 *
 *  Note: typical maximum clock for ADC is 1.4-2MHz
 */
typedef enum {
	adc_div4, /**< CLKper/4 */
	adc_div8, /**< CLKper/8 */
	adc_div16, /**< CLKper/16 */
	adc_div32, /**< CLKper/32 */
	adc_div64, /**< CLKper/64 */
	adc_div128, /**< CLKper/128 */
	adc_div256, /**< CLKper/256 */
	adc_div512, /**< CLKper/512 */
} adc_clkpre_t;

/** \brief ADC input modes
 *
 * Note: Differential modes are only available in signed mode
 */
typedef enum {
	adc_internal, /**< Internal input */
	adc_singleend, /**< Single-Ended */
	adc_diff, /**< Differential */
	adc_diffgain, /**< Differential with gain */
} adc_input_t;

/** \brief Initalise the ADC with the following features
 *  \param mode Signed or Unsigned (Unsigned is invalid for differential
 *  \param vref ADC reference source
 *  \param bits ADC resolution (8 or 12)
 *  \param clkpre ADC CLKper divider
 *  \return 0 on success, errors.h otherwise
 */
uint8_t adc_init(adc_mode_t mode, adc_vref_t vref, uint8_t bits,
	adc_clkpre_t clkpre);

/** \brief Set up conversion parameters for the given channel
 *
 *  Note: not all pin numbers for muxpos/muxneg apply to all chips
 *
 *  \param chan Channel number
 *  \param input Input type
 *  \param muxpos Postive MUX input (pin number from 0-15)
 *  \param muxneg Negative MUX input (pin number from 0-3/4-7 for gain),
 *  only used in differential mode
 *  \return 0 on success, errors.h otherwise
 */
uint8_t adc_conf(uint8_t chan, adc_input_t input, uint8_t muxpos,
	uint8_t muxneg);

/** \brief Blocking conversion
 *
 *  Once conversion parameters are set, perform a conversion in
 *  blocking mode. The result is returned, or errors.h
 *  \param chan Channel number
 *  \param discard Discard this many passes before converting
 *  \param count Average over count samples
 *  \return ADC value
 */
uint16_t adc_conv_blocking(uint8_t chan, uint8_t discard, uint8_t count);

#ifdef __cplusplus
}
#endif

#endif // ADC_H_INCLUDED
