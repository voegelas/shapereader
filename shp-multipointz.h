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

#ifndef _SHAPEREADER_SHP_MULTIPOINTZ_H
#define _SHAPEREADER_SHP_MULTIPOINTZ_H

#include "shp-box.h"
#include "shp-pointz.h"
#include "shp-range.h"
#include <stddef.h>

/**
 * MultiPointZ
 *
 * A MultiPointZ is a set of points with one measure per point, for example
 * a temperature.
 */
typedef struct shp_multipointz_t {
    shp_box_t box;        /**< Bounding box */
    size_t num_points;    /**< Number of points */
    const char *_points;  /* X and Y coordinates */
    shp_range_t z_range;  /**< Bounding Z range */
    const char *_z_array; /* Z coordinates */
    shp_range_t m_range;  /**< Bounding measure range */
    const char *_m_array; /* Measures */
} shp_multipointz_t;

/**
 * Get a PointZ
 *
 * Gets a point and a measure from a set of points.
 *
 * @b Example
 *
 * @code{.c}
 * // Iterate over all points
 * size_t i;
 * shp_pointz_t pointz;
 *
 * for (i = 0; i < multipointz->num_points; ++i) {
 *   shp_multipointz_pointz(multipoint, i, &pointz);
 * }
 * @endcode
 *
 * @memberof shp_multipointz_t
 * @param multipointz a shp_multipointz_t structure.
 * @param point_num a zero-based point number.
 * @param[out] pointz a shp_pointz_t structure.
 */
extern void shp_multipointz_pointz(const shp_multipointz_t *multipointz,
                                   size_t point_num, shp_pointz_t *pointz);

#endif
