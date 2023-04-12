#include "../shapereader.h"
#include "tap.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

int tests_planned = 0;
int tests_run = 0;
int tests_failed = 0;

shp_header_t shp_header;
shp_record_t *shp_record;
const shp_polyline_t *polyline;

size_t record_number;

/*
 * Main file tests
 */

static int
test_header_shape_type(void)
{
    return shp_header.shape_type == SHP_TYPE_POLYLINE;
}

static int
test_record_shape_type(void)
{
    return shp_record->shape_type == SHP_TYPE_POLYLINE;
}

static int
test_bbox(void)
{
    return polyline->x_min == 1 && polyline->y_min == 1 &&
           polyline->x_max == 3 && polyline->y_max == 3;
}

static int
test_has_two_parts(void)
{
    return polyline->num_parts == 2;
}

static int
test_has_four_points(void)
{
    return polyline->num_points == 4;
}

static int
test_has_six_points(void)
{
    return polyline->num_points == 6;
}

static void
test_shp(void)
{
    ok(test_record_shape_type, "record shape type is polyline");
    polyline = &shp_record->shape.polyline;
    switch (record_number) {
    case 0:
        ok(test_bbox, "bounding box matches");
        ok(test_has_two_parts, "diagonal cross has two parts");
        ok(test_has_four_points, "diagonal cross has four points");
        break;
    case 1:
        ok(test_has_two_parts, "greek cross has two parts");
        ok(test_has_six_points, "greek cross has six points");
        break;
    }
}

int
main(int argc, char *argv[])
{
    const char *filename = "polyline.shp";
    FILE *fp;
    shp_file_t fh;

    plan(8);

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 1;
    }

    shp_init_file(&fh, fp, NULL);

    if (shp_read_header(&fh, &shp_header) > 0) {
        ok(test_header_shape_type, "header shape type is polyline");
        record_number = 0;
        while (shp_read_record(&fh, &shp_record) > 0) {
            test_shp();
            free(shp_record);
            ++record_number;
        }
        fprintf(stderr, "# %s\n", fh.error);
    }

    fclose(fp);

    done_testing();
}
