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
const shp_point_t *point;

size_t record_number;

/*
 * Main file tests
 */

static int
test_header_shape_type(void)
{
    return shp_header.shape_type == SHPT_POINT;
}

static int
test_record_shape_type(void)
{
    return shp_record->shape_type == SHPT_POINT;
}

static int
test_freiburg(void)
{
    return point->x == 7.8522 && point->y == 47.9959;
}

static int
test_karlsruhe(void)
{
    return point->x == 8.4044 && point->y == 49.0094;
}

static int
test_mannheim(void)
{
    return point->x == 8.4669 && point->y == 49.4891;
}

static int
test_stuttgart(void)
{
    return point->x == 9.1770 && point->y == 48.7823;
}

static void
test_shp(void)
{
    ok(test_record_shape_type, "record shape type is point");
    point = &shp_record->shape.point;
    switch (record_number) {
    case 0:
        ok(test_freiburg, "location is Freiburg");
        break;
    case 1:
        ok(test_karlsruhe, "location is Karlsruhe");
        break;
    case 2:
        ok(test_mannheim, "location is Mannheim");
        break;
    case 3:
        ok(test_stuttgart, "location is Stuttgart");
        break;
    }
}

int
main(int argc, char *argv[])
{
    const char *filename = "point.shp";
    FILE *fp;
    shp_file_t fh;

    plan(9);

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 1;
    }

    shp_init_file(&fh, fp, NULL);

    if (shp_read_header(&fh, &shp_header) > 0) {
        ok(test_header_shape_type, "header shape type is point");
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
