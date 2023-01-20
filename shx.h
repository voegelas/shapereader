/*
 * Read ESRI shapefiles
 *
 * Copyright (C) 2023 Andreas Vögele
 *
 * This library is free software; you can redistribute it and/or modify it
 * under either the terms of the ISC License or the same terms as Perl.
 */

/* SPDX-License-Identifier: ISC OR Artistic-1.0-Perl OR GPL-1.0-or-later */

/**
 * @file
 */

#ifndef _SHAPEREADER_SHX_H
#define _SHAPEREADER_SHX_H

#include "shp.h"
#include <stdint.h>
#include <stdio.h>

/**
 * Header
 */
typedef shp_header_t shx_header_t;

/**
 * Record
 */
typedef struct shx_record_t {
    int32_t file_offset;    /**< Offset in the ".shp" file in 16-bit words */
    int32_t content_length; /**< Content length in 16-bit words */
} shx_record_t;

/**
 * File handle
 */
typedef shp_file_t shx_file_t;

/**
 * Initialize a file handle
 *
 * Initializes a shx_file_t structure.
 *
 * @param fh an uninitialized file handle.
 * @param fp a file pointer.
 * @param user_data callback data or NULL.
 * @return the initialized file handle.
 */
extern shx_file_t *shx_file(shx_file_t *fh, FILE *fp, void *user_data);

/**
 * Set an error message.
 *
 * Formats and sets an error message.
 *
 * @param fh a file handle.
 * @param format a printf format string followed by a variable number of
 *               arguments.
 */
extern void shx_error(shx_file_t *fh, const char *format, ...);

/**
 * Handle the file header.
 *
 * A callback function that is called for the file header.
 *
 * @param fh a file handle.
 * @param header a pointer to a shx_header_t structure.
 * @retval 1 on sucess.
 * @retval 0 to stop the processing.
 * @retval -1 on error.
 */
typedef int (*shx_header_callback_t)(shx_file_t *fh,
                                     const shx_header_t *header);

/**
 * Handle a record.
 *
 * A callback function that is called for each record.
 *
 * @param fh a file handle.
 * @param header a pointer to a shx_header_t structure.
 * @param record a pointer to a shx_record_t structure.
 * @retval 1 on sucess.
 * @retval 0 to stop the processing.
 * @retval -1 on error.
 */
typedef int (*shx_record_callback_t)(shx_file_t *fh,
                                     const shx_header_t *header,
                                     const shx_record_t *record);

/**
 * Read an index file.
 *
 * Reads files that have the file extension ".shx" and calls functions for the
 * file header and each record.
 *
 * The data that is passed to the callback functions is only valid during the
 * function call.  Do not keep pointers to the data.
 *
 * @param fh a file handle.
 * @param handle_header a function that is called for the file header.
 * @param handle_record a function that is called for each record.
 * @retval 1 on success.
 * @retval 0 on end of file.
 * @retval -1 on error.
 * @see the "ESRI Shapefile Technical Description" @cite ESRI_shape for
 *      information on the file format.
 */
extern int shx_read(shx_file_t *fh, shx_header_callback_t handle_header,
                    shx_record_callback_t handle_record);

/**
 * Read the file header.
 *
 * Reads the header from files that have the file extension ".shx".
 *
 * @param fh a file handle.
 * @param[out] pheader on sucess, a pointer to a shx_header_t structure.
 *                     Free the header with @c free() when you are done.
 * @retval 1 on success.
 * @retval 0 on end of file.
 * @retval -1 on error.
 */
extern int shx_read_header(shx_file_t *fh, shx_header_t **pheader);

/**
 * Read a record.
 *
 * Reads a record from files that have the file extension ".shx".
 *
 * @param fh a file handle.
 * @param[out] precord on success, a pointer to a shx_record_t structure.
 *                     Free the record with @c free() when you are done.
 * @retval 1 on success.
 * @retval 0 on end of file.
 * @retval -1 on error.
 */
extern int shx_read_record(shx_file_t *fh, shx_record_t **precord);

#endif
