#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_SMD_UNITS 100 
#define MAX_SMD_LENGTH 20000

#define MAX_TAG_LENGTH 15 
#define MAX_DATA_LENGTH 2000

typedef struct {
	char 	*tag;
	char 	*data;
	int 	level;
} unit_t;

typedef struct {
	unit_t 	*units;
	int		count;
} unit_array_t;

typedef struct {
	int open;
	int data;
	int close;
	int level;

} unit_points_t;

typedef struct {
	unit_points_t 	*points;
	int 			count;

} unit_points_array_t;

typedef struct {
	char 				*smd;
	unit_points_array_t	*points_array; 
} pointed_smd_t;

int parse_smd_points(pointed_smd_t *pointed_smd) {
	char *smd = pointed_smd->smd;
	unit_points_array_t *points_array = pointed_smd->points_array;

	int level = -1; 
	int index = -1;

	int unclosed_count = -1;
	int *unclosed_indexes = malloc(MAX_SMD_UNITS * sizeof(int));
	bool accept_closing = false;

	int c;
	int length = 0;
    while ((c = fgetc(stdin)) != EOF) {

		if (c == '\n' || c == '\t' ) {
			continue;
		}

		smd[length] = c;
		
		if (c == '<') {
			index++;
			level++;
			unclosed_count++;

			points_array->points[index].open = length;
			points_array->points[index].level = level;
			unclosed_indexes[unclosed_count] = index;
		} else if (c == '#') {
			points_array->points[index].data = length;
		} else if (c == '>') {
			if (accept_closing == false){
				continue;
			}
			accept_closing = false;

			const int unclosed_index = unclosed_indexes[unclosed_count];
			points_array->points[unclosed_index].close = length;

			unclosed_count--;
			level--;
		} else if (c == '$') {
			accept_closing = true;
		}

		length++;
    }

	free(unclosed_indexes);

	smd[length] = '\0';

	points_array->count = index + 1;
	points_array->points = realloc(points_array->points, points_array->count * sizeof(unit_points_t));
}

void sub_smd(char *smd, char *start, int *length, char *sub){
	memcpy(sub, start, *length);
	sub[*length] = '\0';
}

void get_tag(char *smd, unit_points_t *points, char *tag) {
	int tag_length = points->data - points->open - 1;
	sub_smd(smd, &smd[points->open + 1], &tag_length, tag);
}

void get_data(char *smd, unit_points_t *points, char *data) {
	int data_length = points->close - points->data - 2;
	sub_smd(smd, &smd[points->data + 1], &data_length, data);
}

void make_unit_array(char *path, unit_array_t *unit_array) {
	int count = 0;
	unit_t *units = malloc(MAX_SMD_UNITS * sizeof(unit_t));

	int tag_index = 0;	
	char *tag = malloc(MAX_TAG_LENGTH * sizeof(char));

	char c; 
	int index = 0;
	while (1) {
		c = path[index];

		if (c == ':' || c == '\0') {
			tag = realloc(tag, tag_index + 2);
			tag[tag_index + 1] = '\0';

			units[count].tag = tag;
			units[count].level = count;

			tag_index = 0;
			tag = malloc(MAX_TAG_LENGTH * sizeof(char));
			count++;
		} else {
			tag[tag_index] = c;
			tag_index++;
		}

		if (c == '\0') {
			break;
		}

		index++;
	}

	unit_array->units = units;
	unit_array->count = count; 
}

void find_points(pointed_smd_t *pointed_smd, unit_array_t *unit_array, unit_points_t *points) {
	char *smd = pointed_smd->smd;
	unit_points_array_t *points_array = pointed_smd->points_array;

	int unit_index = 0;

	for (int i = 0; i < points_array->count; i++) {
		unit_points_t unit_points = points_array->points[i];
		unit_t unit = unit_array->units[unit_index];
		
		if (unit_points.level != unit.level) {
			continue;
		}

		char *tag = malloc(MAX_TAG_LENGTH * sizeof(char));
		get_tag(smd, &unit_points, tag);

		if (strcmp(tag, unit.tag)) {
			continue;
		}

		free(tag);

		if (unit_index + 1 < unit_array->count) {
			unit_index++;
			continue;
		}

		*points = unit_points;
		return;
	}
}

void free_unit_array(unit_array_t *unit_array) {
	for (int i = 0; i < unit_array->count; i++) {
		free(unit_array->units[i].data);
		free(unit_array->units[i].tag);
		unit_array->units[i].tag = NULL;
		unit_array->units[i].data = NULL;
	}

	free(unit_array->units);
	unit_array->units = NULL;

	free(unit_array);
	unit_array = NULL;
}

int main(int argc, char *argv[]) {
	char *smd = malloc(MAX_SMD_LENGTH * sizeof(char));
	unit_points_t *smd_unit_points = malloc(MAX_SMD_LENGTH * sizeof(unit_points_t));

	char *path = argv[1];

	int unit_points_count = 0;
	unit_points_array_t unit_points_array = {smd_unit_points, unit_points_count};

	pointed_smd_t pointed_smd = {smd, &unit_points_array};

	parse_smd_points(&pointed_smd);


	unit_array_t *unit_array = malloc(MAX_SMD_UNITS * sizeof(unit_array_t));

	make_unit_array(path, unit_array);
	unit_points_t found_points;
	find_points(&pointed_smd, unit_array, &found_points);

	char data[MAX_DATA_LENGTH];
	get_data(smd, &found_points, data);


	fprintf(stdout, "%s", data);

	free_unit_array(unit_array);
	free(smd);
	free(smd_unit_points);

    return 0;
}

