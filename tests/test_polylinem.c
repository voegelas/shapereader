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
const shp_polylinem_t *polylinem;

size_t record_number;

/*
 * Main file tests
 */

static int
test_header_shape_type(void)
{
    return shp_header.shape_type == SHPT_POLYLINEM;
}

static int
test_record_shape_type(void)
{
    return shp_record->shape_type == SHPT_POLYLINEM;
}

static int
test_bbox(void)
{
    const shp_box_t *b = &polylinem->box;
    return b->x_min == 1 && b->y_min == 1 && b->x_max == 4 && b->y_max == 2;
}

static int
test_has_one_part(void)
{
    return polylinem->num_parts == 1;
}

static int
test_has_six_points(void)
{
    return polylinem->num_points == 6;
}

static int
test_points_match(void)
{
    size_t i, j, n;
    shp_pointm_t p;
    const shp_pointm_t points[6] = {{1, 1, 1}, {2, 1, 2}, {2, 2, 3},
                                    {3, 2, 4}, {3, 1, 5}, {4, 1, 6}};

    j = 0;
    shp_polylinem_points(polylinem, 0, &i, &n);
    while (i < n) {
        shp_polylinem_pointm(polylinem, i, &p);
        if (p.x != points[j].x || p.y != points[j].y || p.m != points[j].m) {
            return 0;
        }
        ++j;
        ++i;
    }
    return 1;
}

static void
test_shp(void)
{
    ok(test_record_shape_type, "record shape type is polylinem");
    polylinem = &shp_record->shape.polylinem;
    switch (record_number) {
    case 0:
        ok(test_bbox, "bounding box matches");
        ok(test_has_one_part, "line has one part");
        ok(test_has_six_points, "line has six points");
        ok(test_points_match, "points match");
        break;
    }
}

int
main(int argc, char *argv[])
{
    const char *filename = "polylinem.shp";
    FILE *fp;
    shp_file_t fh;

    plan(6);

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "# Cannot open file \"%s\": %s\n", filename,
                strerror(errno));
        return 1;
    }

    shp_init_file(&fh, fp, NULL);

    if (shp_read_header(&fh, &shp_header) > 0) {
        ok(test_header_shape_type, "header shape type is polylinem");
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
