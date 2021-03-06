/*
 * parser.h
 *
 *  Created on: 10 dec. 2021
 *      Author: Ludo
 */

#ifndef PARSER_H
#define	PARSER_H

/*** PARSER structures ***/

typedef enum at_param_type {
	PARSER_PARAMETER_TYPE_BOOLEAN,
	PARSER_PARAMETER_TYPE_HEXADECIMAL,
	PARSER_PARAMETER_TYPE_DECIMAL
} PARSER_ParameterType;

typedef enum {
    PARSER_SUCCESS,
    PARSER_ERROR_UNKNOWN_COMMAND,
	PARSER_ERROR_MODE,
    PARSER_ERROR_HEADER_NOT_FOUND,
    PARSER_ERROR_SEPARATOR_NOT_FOUND,
    PARSER_ERROR_PARAMETER_NOT_FOUND,
    PARSER_ERROR_PARAMETER_BIT_INVALID,
    PARSER_ERROR_PARAMETER_BIT_OVERFLOW,
    PARSER_ERROR_PARAMETER_HEXA_INVALID,
    PARSER_ERROR_PARAMETER_HEXA_OVERFLOW,
    PARSER_ERROR_PARAMETER_HEXA_ODD_SIZE,
    PARSER_ERROR_PARAMETER_DEC_INVALID,
    PARSER_ERROR_PARAMETER_DEC_OVERFLOW,
    PARSER_ERROR_PARAMETER_BYTE_ARRAY_INVALID_LENGTH,
} PARSER_Status;

typedef enum {
	PARSER_MODE_COMMAND,
	PARSER_MODE_HEADER,
	PARSER_MODE_LAST
} PARSER_mode_t;

typedef struct {
    unsigned char* rx_buf;
    unsigned int rx_buf_length;
    unsigned char start_idx;
    unsigned char separator_idx;
} PARSER_Context;

/*** PARSER functions ***/

PARSER_Status PARSER_compare(PARSER_Context* parser_ctx, PARSER_mode_t mode, char* command);
PARSER_Status PARSER_get_parameter(PARSER_Context* parser_ctx, PARSER_ParameterType param_type, char separator, unsigned char last_param, int* param);
PARSER_Status PARSER_get_byte_array(PARSER_Context* parser_ctx, char separator, unsigned char last_param, unsigned char max_length, unsigned char* param, unsigned char* extracted_length);

#endif	/* PARSER_H */

