/*
 * Copyright ©2009  Simon Arlott
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "digit_dash.xbm"
#include "digit_dot.xbm"
#include "digit_zero.xbm"
#include "digit_one.xbm"
#include "digit_two.xbm"
#include "digit_three.xbm"
#include "digit_four.xbm"
#include "digit_five.xbm"
#include "digit_six.xbm"
#include "digit_seven.xbm"
#include "digit_eight.xbm"
#include "digit_nine.xbm"

static unsigned char *digits_bits[10] = {
	digit_zero_bits,
	digit_one_bits,
	digit_two_bits,
	digit_three_bits,
	digit_four_bits,
	digit_five_bits,
	digit_six_bits,
	digit_seven_bits,
	digit_eight_bits,
	digit_nine_bits
};

static int digits_width[10] = {
	digit_zero_width,
	digit_one_width,
	digit_two_width,
	digit_three_width,
	digit_four_width,
	digit_five_width,
	digit_six_width,
	digit_seven_width,
	digit_eight_width,
	digit_nine_width
};

static int digits_height[10] = {
	digit_zero_height,
	digit_one_height,
	digit_two_height,
	digit_three_height,
	digit_four_height,
	digit_five_height,
	digit_six_height,
	digit_seven_height,
	digit_eight_height,
	digit_nine_height
};
