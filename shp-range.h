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

#ifndef _SHAPEREADER_SHP_RANGE_H
#define _SHAPEREADER_SHP_RANGE_H

/**
 * Bounding range
 *
 * The smallest and largest value in a set of values.
 */
typedef struct shp_range_t {
    double min; /**< Smallest value */
    double max; /**< Largest value */
} shp_range_t;

#endif
