/**
 * @author (c) Eyal Rozenberg <eyalroz1@gmx.com>
 *             2021, Haifa, Palestine/Israel
 * @author (c) Marco Paland (info@paland.com)
 *             2014-2019, PALANDesign Hannover, Germany
 *
 * @note Others have made smaller contributions to this file: see the
 * contributors page at https://github.com/eyalroz/printf/graphs/contributors
 * or ask one of the authors.
 *
 * @brief Small stand-alone implementation of the printf family of functions
 * (`(v)printf`, `(v)s(n)printf` etc., geared towards use on embedded systems with
 * a very limited resources.
 *
 * @note the implementations are thread-safe; re-entrant; use no functions from
 * the standard library; and do not dynamically allocate any memory.
 *
 * @license The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef PRINTF_H_
#define PRINTF_H_

#include <stdarg.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif



#ifdef __GNUC__
# define ATTR_PRINTF(one_based_format_index, first_arg) \
__attribute__((format(__printf__, (one_based_format_index), (first_arg))))
# define ATTR_VPRINTF(one_based_format_index) ATTR_PRINTF(one_based_format_index, 0)
#else
# define ATTR_PRINTF(one_based_format_index, first_arg)
# define ATTR_VPRINTF(one_based_format_index)
#endif

#ifndef PRINTF_ALIAS_STANDARD_FUNCTION_NAMES
#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES 0
#endif

#if PRINTF_ALIAS_STANDARD_FUNCTION_NAMES
# define printf_    printf
# define sprintf_   sprintf
# define vsprintf_  vsprintf
# define snprintf_  snprintf
# define vsnprintf_ vsnprintf
# define vprintf_   vprintf
#endif


// 'ntoa' conversion buffer size, this must be big enough to hold one converted
// numeric number including padded zeros (dynamically created on stack)
#ifndef PRINTF_INTEGER_BUFFER_SIZE
#define PRINTF_INTEGER_BUFFER_SIZE    32
#endif

// 'ftoa' conversion buffer size, this must be big enough to hold one converted
// float number including padded zeros (dynamically created on stack)
#ifndef PRINTF_FTOA_BUFFER_SIZE
#define PRINTF_FTOA_BUFFER_SIZE    32
#endif

// Support for the decimal notation floating point conversion specifiers (%f, %F)
#ifndef PRINTF_SUPPORT_DECIMAL_SPECIFIERS
#define PRINTF_SUPPORT_DECIMAL_SPECIFIERS 1
#endif

// Support for the exponential notation floating point conversion specifiers (%e, %g, %E, %G)
#ifndef PRINTF_SUPPORT_EXPONENTIAL_SPECIFIERS
#define PRINTF_SUPPORT_EXPONENTIAL_SPECIFIERS 1
#endif

// Support for the length write-back specifier (%n)
#ifndef PRINTF_SUPPORT_WRITEBACK_SPECIFIER
#define PRINTF_SUPPORT_WRITEBACK_SPECIFIER 1
#endif

// Default precision for the floating point conversion specifiers (the C standard sets this at 6)
#ifndef PRINTF_DEFAULT_FLOAT_PRECISION
#define PRINTF_DEFAULT_FLOAT_PRECISION  6
#endif

// According to the C languages standard, printf() and related functions must be able to print any
// integral number in floating-point notation, regardless of length, when using the %f specifier -
// possibly hundreds of characters, potentially overflowing your buffers. In this implementation,
// all values beyond this threshold are switched to exponential notation.
#ifndef PRINTF_MAX_INTEGRAL_DIGITS_FOR_DECIMAL
#define PRINTF_MAX_INTEGRAL_DIGITS_FOR_DECIMAL 9
#endif

// Support for the long long integral types (with the ll, z and t length modifiers for specifiers
// %d,%i,%o,%x,%X,%u, and with the %p specifier). Note: 'L' (long double) is not supported.
#ifndef PRINTF_SUPPORT_LONG_LONG
#define PRINTF_SUPPORT_LONG_LONG 1
#endif

/**
 * Printf implementation for Artemis
 *
 * @param buffer A pointer to the buffer where to store the formatted string. MUST be big enough to store the output!
 *               if zero, only the expected count of characters is returned
 * @param maxlen : maximum length allowed in the buffer.
 * @param format A string that specifies the format of the output
 * @param va A value identifying a variable arguments list
 * @return The number of characters that are  WRITTEN into the buffer (or WILL if buffer is zero), not counting the terminating null character
 */
int Artemis_printf(char* buffer, const size_t maxlen, const char* format, va_list va);
int Artemis_sprintf(char* buffer, const char* format, ...) ATTR_PRINTF(2, 3);

/**
 * Artemis_dtostrf() may be the function you need if you have a floating point value that you need to convert to a string
 *
 * @param val   : Floating point value to convert
 * @param width : Minimum filed width
 * @param prec  : Number of digits after the decimal point
 * @param sout  : Character buffer to store the result
 * @return      : provided pointer to sout.
 *
 * If you set the  minimum field width to 6, and you convert a floating point value that has less than 6 digits, then spaces will be
 * added before the number starts. And for counting purposes, the “.” decimal point and the “-” negative sign count as spaces too.
 *
 * For Prec : If the floating point value you convert to a string has more digits after the decimal point than the number specified in the
 * precision, then it will be cut off and rounded accordingly in the output string.
 * If you pass a number with fewer digits after the decimal point, it will add trailing zeros to the number.
 *
 * When setting the sout consider
 *  - What the biggest number might ever be
 *  - Size must include space for the “.” and the possible “-” sign
 *  - Add 1 for the null-terminating character “\0”
 *
 */

char *Artemis_dtostrf (double val, signed char width, unsigned char prec, char *sout);

/**
 * Output a character to a custom device like UART, used by the printf() function
 * This function is declared here only. You have to write your custom implementation somewhere
 * @param character Character to output
 */
void putchar_(char character);


/**
 * Tiny printf implementation
 * You have to implement putchar_ if you use printf()
 * To avoid conflicts with the regular printf() API it is overridden by macro defines
 * and internal underscore-appended functions like printf_() are used
 * @param format A string that specifies the format of the output
 * @return The number of characters that are written into the array, not counting the terminating null character
 */
int printf_(const char* format, ...) ATTR_PRINTF(1, 2);


/**
 * Tiny sprintf/vsprintf implementation
 * Due to security reasons (buffer overflow) YOU SHOULD CONSIDER USING (V)SNPRINTF INSTEAD!
 * @param buffer A pointer to the buffer where to store the formatted string. MUST be big enough to store the output!
 * @param format A string that specifies the format of the output
 * @param va A value identifying a variable arguments list
 * @return The number of characters that are WRITTEN into the buffer, not counting the terminating null character
 */
int  sprintf_(char* buffer, const char* format, ...) ATTR_PRINTF(2, 3);
int vsprintf_(char* buffer, const char* format, va_list va) ATTR_VPRINTF(2);


/**
 * Tiny snprintf/vsnprintf implementation
 * @param buffer A pointer to the buffer where to store the formatted string
 * @param count The maximum number of characters to store in the buffer, including a terminating null character
 * @param format A string that specifies the format of the output
 * @param va A value identifying a variable arguments list
 * @return The number of characters that COULD have been written into the buffer, not counting the terminating
 *         null character. A value equal or larger than count indicates truncation. Only when the returned value
 *         is non-negative and less than count, the string has been completely written.
 */
int  snprintf_(char* buffer, size_t count, const char* format, ...) ATTR_PRINTF(3, 4);
int vsnprintf_(char* buffer, size_t count, const char* format, va_list va) ATTR_VPRINTF(3);


/**
 * Tiny vprintf implementation
 * @param format A string that specifies the format of the output
 * @param va A value identifying a variable arguments list
 * @return The number of characters that are WRITTEN into the buffer, not counting the terminating null character
 */
int vprintf_(const char* format, va_list va) ATTR_VPRINTF(1);


/**
 * printf/vprintf with output function
 * You may use this as dynamic alternative to printf() with its fixed putchar_() output
 * @param out An output function which takes one character and an argument pointer
 * @param arg An argument pointer for user data passed to output function
 * @param format A string that specifies the format of the output
 * @param va A value identifying a variable arguments list
 * @return The number of characters that are sent to the output function, not counting the terminating null character
 */
int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...) ATTR_PRINTF(3, 4);
int vfctprintf(void (*out)(char character, void* arg), void* arg, const char* format, va_list va) ATTR_VPRINTF(3);

#ifdef __cplusplus
}
#endif

#if PRINTF_ALIAS_STANDARD_FUNCTION_NAMES
# undef printf_
# undef sprintf_
# undef vsprintf_
# undef snprintf_
# undef vsnprintf_
# undef vprintf_
#endif

#endif  // PRINTF_H_
