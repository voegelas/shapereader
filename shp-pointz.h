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

#ifndef _SHAPEREADER_SHP_POINTZ_H
#define _SHAPEREADER_SHP_POINTZ_H

/**
 * PointZ
 *
 * A location in three-dimensional space with a measure, for example a
 * temperature.
 */
typedef struct shp_pointz_t {
    double x; /**< The X coordinate */
    double y; /**< The Y coordinate */
    double z; /**< The Z coordinate */
    double m; /**< Some measure */
} shp_pointz_t;

#endif
