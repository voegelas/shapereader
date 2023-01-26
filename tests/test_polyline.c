#include "../shapereader.h"
#include "tap.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int tests_planned = 0;
int tests_run = 0;
int tests_failed = 0;

shp_header_t shp_header;
shp_record_t *shp_record;
const shp_polyline_t *polyline;
shp_file_t shp_fh;

size_t record_number;

/*
 * Main file tests
 */

static int
test_header_shape_type(void)
{
    return shp_header.shape_type == SHPT_POLYLINE;
}

static int
test_record_shape_type(void)
{
    return shp_record->shape_type == SHPT_POLYLINE;
}

static int
test_bbox(void)
{
    const shp_box_t *b = &polyline->box;
    return b->x_min == 1 && b->y_min == 1 && b->x_max == 3 && b->y_max == 3;
}

static int
test_has_four_parts(void)
{
    return polyline->num_parts == 4;
}

static int
test_has_twelve_points(void)
{
    return polyline->num_points == 12;
}

static int
test_points_on_line(void)
{
    int x, y;
    shp_point_t p;

    for (x = 1; x <= 3; ++x) {
        for (y = 1; y <= 3; ++y) {
            p.x = x;
            p.y = y;
            if (shp_polyline_point_on_polyline(polyline, &p, 0) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

static int
test_point_not_on_line(void)
{
    shp_point_t p = {1.2999, 1.3};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static int
test_point_on_line_with_epsilon(void)
{
    shp_point_t p = {1.2999, 1.3};
    return shp_polyline_point_on_polyline(polyline, &p, 1e-4) != 0;
}

static int
test_point_left_of_polyline(void)
{
    shp_point_t p = {0.9999, 2};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static int
test_point_right_of_polyline(void)
{
    shp_point_t p = {3.0001, 2};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static int
test_point_below_polyline(void)
{
    shp_point_t p = {2, 0.9999};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static int
test_point_above_polyline(void)
{
    shp_point_t p = {2, 3.0001};
    return shp_polyline_point_on_polyline(polyline, &p, 0) == 0;
}

static void
test_shp(void)
{
    ok(test_record_shape_type, "record shape type is polyline");
    polyline = &shp_record->shape.polyline;
    switch (record_number) {
    case 0:
        ok(test_bbox, "bounding box matches");
        ok(test_has_four_parts, "star has four parts");
        ok(test_has_twelve_points, "star has twelve points");
        ok(test_points_on_line, "points on line");
        ok(test_point_not_on_line, "point not on line");
        ok(test_point_on_line_with_epsilon, "point on line with epsilon");
        ok(test_point_left_of_polyline, "point left of polyline");
        ok(test_point_right_of_polyline, "point right of polyline");
        ok(test_point_below_polyline, "point below polyline");
        ok(test_point_above_polyline, "point above polyline");
        break;
    }
}

int
main(int argc, char *argv[])
{
    const char *testdatadir = getenv("testdatadir");
    FILE *shp_fp;

    plan(12);

    if (testdatadir == NULL) {
        fprintf(stderr,
                "# The environment variable \"testdatadir\" is not set\n");
        return 1;
    }

    if (chdir(testdatadir) == -1) {
        fprintf(stderr, "# Cannot change directory to \"%s\": %s\n",
                testdatadir, strerror(errno));
        return 1;
    }

    shp_fp = fopen("polyline.shp", "rb");
    if (shp_fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", "polyline.shp",
                strerror(errno));
        return 1;
    }

    shp_init_file(&shp_fh, shp_fp, NULL);

    if (shp_read_header(&shp_fh, &shp_header) > 0) {
        ok(test_header_shape_type, "header shape type is polyline");
        record_number = 0;
        while (shp_read_record(&shp_fh, &shp_record) > 0) {
            test_shp();
            free(shp_record);
            ++record_number;
        }
        fprintf(stderr, "# %s\n", shp_fh.error);
    }

    fclose(shp_fp);

    done_testing();
}
