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
const shp_polylinez_t *polylinez;

size_t record_number;

/*
 * Main file tests
 */

static int
test_header_shape_type(void)
{
    return shp_header.shape_type == SHPT_POLYLINEZ;
}

static int
test_record_shape_type(void)
{
    return shp_record->shape_type == SHPT_POLYLINEZ;
}

static int
test_ranges(void)
{
    const shp_box_t *b = &polylinez->box;
    const shp_range_t *z = &polylinez->z_range;
    const shp_range_t *m = &polylinez->m_range;
    return b->x_min == 8.975675 && b->y_min == 48.746122 &&
           b->x_max == 8.976038 && b->y_max == 48.746420 && z->min == 477.5 &&
           z->max == 493.3 && m->min == 0.0 && m->max == 2.02;
}

static int
test_has_two_parts(void)
{
    return polylinez->num_parts == 2;
}

static int
test_has_sixteen_points(void)
{
    return polylinez->num_points == 16;
}

static int
test_points_match(void)
{
    size_t i, j, n;
    shp_pointz_t p;
    const shp_pointz_t points[16] = {
        {8.975817, 48.746274, 493.2, 0.0},
        {8.975824, 48.746279, 493.3, 0.0},
        {8.975824, 48.746269, 491.1, 0.15},
        {8.975806, 48.746263, 488.8, 0.45},
        {8.975681, 48.746227, 485.6, 2.02},
        {8.975677, 48.746213, 485.2, 1.53},
        {8.975675, 48.746135, 482.3, 1.36},
        {8.975675, 48.746122, 482.1, 1.36},
        {8.975819, 48.746283, 480.3, 0.0},
        {8.975821, 48.746283, 480.1, 0.26},
        {8.975826, 48.746284, 479.3, 0.62},
        {8.975833, 48.746284, 478.1, 0.0},
        {8.975848, 48.746289, 478.8, 0.6},
        {8.975943, 48.746341, 478.1, 1.4},
        {8.975954, 48.746351, 477.5, 1.39},
        {8.976038, 48.746420, 478.9, 1.43},
    };

    j = 0;
    shp_polylinez_points(polylinez, 0, &i, &n);
    while (i < n) {
        shp_polylinez_pointz(polylinez, i, &p);
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
    ok(test_record_shape_type, "record shape type is polylinez");
    polylinez = &shp_record->shape.polylinez;
    switch (record_number) {
    case 0:
        ok(test_ranges, "bounding box and ranges match");
        ok(test_has_two_parts, "line has two parts");
        ok(test_has_sixteen_points, "line has sixteen points");
        ok(test_points_match, "points match");
        break;
    }
}

int
main(int argc, char *argv[])
{
    const char *filename = "polylinez.shp";
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
        ok(test_header_shape_type, "header shape type is polylinez");
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
