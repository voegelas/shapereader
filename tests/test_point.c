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
const shp_point_t *point;
shp_file_t shp_fh;

int32_t record_number;

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
        ok(test_freiburg, "Freiburg");
        break;
    case 1:
        ok(test_karlsruhe, "Karlsruhe");
        break;
    case 2:
        ok(test_mannheim, "Mannheim");
        break;
    case 3:
        ok(test_stuttgart, "Stuttgart");
        break;
    }
}

int
main(int argc, char *argv[])
{
    const char *datadir = getenv("datadir");
    FILE *shp_fp;

    plan(9);

    if (datadir == NULL) {
        fprintf(stderr,
                "# The environment variable \"datadir\" is not set\n");
        return 1;
    }

    if (chdir(datadir) == -1) {
        fprintf(stderr, "# Cannot change directory to \"%s\": %s\n", datadir,
                strerror(errno));
        return 1;
    }

    shp_fp = fopen("point.shp", "rb");
    if (shp_fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", "point.shp",
                strerror(errno));
        return 1;
    }

    shp_init_file(&shp_fh, shp_fp, NULL);

    if (shp_read_header(&shp_fh, &shp_header) > 0) {
        ok(test_header_shape_type, "header shape type is point");
        record_number = 0;
        while (shp_read_record(&shp_fh, &shp_record) > 0) {
        fprintf(stderr, "# bar\n");
            test_shp();
            free(shp_record);
            ++record_number;
        }
        fprintf(stderr, "# %s\n", shp_fh.error);
    }

    fclose(shp_fp);

    done_testing();
}
