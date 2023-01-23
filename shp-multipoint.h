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

#ifndef _SHAPEREADER_SHP_MULTIPOINT_H
#define _SHAPEREADER_SHP_MULTIPOINT_H

#include "shp-box.h"
#include <stdint.h>

/**
 * Multi point
 *
 * A multi point is a set of points.
 */
typedef struct shp_multipoint_t {
    shp_box_t box;       /**< Bounding box */
    int32_t num_points;  /**< Number of points */
    const char *_points; /* Points */
} shp_multipoint_t;

/**
 * Get a point
 *
 * Gets a point from a set of points.
 *
 * @b Example
 *
 * @code{.c}
 * // Iterate over all points
 * int32_t i;
 * shp_point_t point;
 *
 * for (i = 0; i < multipoint->num_points; ++i) {
 *   shp_multipoint_point(multipoint, i, &point);
 * }
 * @endcode
 *
 * @memberof shp_multipoint_t
 * @param multipoint a shp_multipoint_t structure.
 * @param point_num a zero-based point number.
 * @param[out] point a shp_point_t structure.
 */
extern void shp_multipoint_point(const shp_multipoint_t *multipoint,
                                 int32_t point_num, shp_point_t *point);

#endif
