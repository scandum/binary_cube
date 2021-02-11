/*
	Copyright (C) 2014-2021 Igor van den Hoven ivdhoven@gmail.com
*/

/*
	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the
	"Software"), to deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to
	permit persons to whom the Software is furnished to do so, subject to
	the following conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
	Binary Search Cube v1.1
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define BSC_M 8

#define BSC_Z_MAX 32

#define BSC_Z_MIN 8

struct cube
{
	int *w_floor;
	struct w_node **w_axis;
	unsigned short *x_size;
	int *w_volume;
	int volume;
	unsigned short w_size;
	unsigned short m_size;
};

struct w_node
{
	int *x_floor;
	struct x_node **x_axis;
	unsigned short *y_size;
	unsigned short *x_volume;
};

struct x_node
{
	int *y_floor;
	struct y_node **y_axis;
	unsigned char *z_size;
};

struct y_node
{
	int z_keys[BSC_Z_MAX];
	void *z_vals[BSC_Z_MAX];
};

inline void *find_key(struct cube *cube, int key, unsigned short *w, unsigned short *x, unsigned short *y, unsigned short *z);

void split_w_node(struct cube *cube, unsigned short w);
void merge_w_node(struct cube *cube, unsigned short w1, unsigned short w2);

void split_x_node(struct cube *cube, unsigned short w, unsigned short x);
void merge_x_node(struct cube *cube, unsigned short w, unsigned short x1, unsigned short x2);

void split_y_node(struct cube *cube, unsigned short w, unsigned short x, unsigned short y);
void merge_y_node(struct cube *cube, unsigned short w, unsigned short x, unsigned short y1, unsigned short y2);

void insert_z_node(struct cube *cube, unsigned short w, unsigned short x, unsigned short y, unsigned short z, int key, void *val);
void *remove_z_node(struct cube *cube, unsigned short w, unsigned short x, unsigned short y, unsigned short z);

void *find_index(struct cube *cube, int index, unsigned short *w_index, unsigned short *x_index, unsigned short *y_index, unsigned short *z_index);

struct cube *create_cube(void)
{
	struct cube *cube;

	cube = (struct cube *) calloc(1, sizeof(struct cube));

	return cube;
}

void destroy_cube(struct cube *cube)
{
	if (cube->w_size)
	{
		struct w_node *w_node;
		struct x_node *x_node;
		struct y_node *y_node;

		register unsigned short w, x, y;

		for (w = 0 ; w < cube->w_size ; w++)
		{
			w_node = cube->w_axis[w];

			for (x = 0 ; x < cube->x_size[w] ; x++)
			{
				x_node = w_node->x_axis[x];

				for (y = 0 ; y < w_node->y_size[x] ; y++)
				{
					y_node = x_node->y_axis[y];

					free(y_node);
				}
				free(x_node);
			}
			free(w_node);
		}
		free(cube->w_floor);
		free(cube->w_axis);
		free(cube->w_volume);
		free(cube->x_size);
	}
	free(cube);
}

void *get_index(struct cube *cube, int index)
{
	unsigned short w, x, y, z;

	return find_index(cube, index, &w, &x, &y, &z);
}

void *del_index(struct cube *cube, int index)
{
	unsigned short w, x, y, z;

	if (find_index(cube, index, &w, &x, &y, &z))
	{
		return remove_z_node(cube, w, x, y, z);
	}
	return NULL;
}

void set_index(struct cube *cube, int index, void *val)
{
	unsigned short w, x, y, z;

	if (find_index(cube, index, &w, &x, &y, &z))
	{
		cube->w_axis[w]->x_axis[x]->y_axis[y]->z_vals[z] = val;
	}
}

void *get_key(struct cube *cube, int key)
{
	unsigned short w, x, y, z;

	return find_key(cube, key, &w, &x, &y, &z);
}

void *del_key(struct cube *cube, int key)
{
	unsigned short w, x, y, z;

	if (find_key(cube, key, &w, &x, &y, &z))
	{
		return remove_z_node(cube, w, x, y, z);
	}
	return NULL;
}

void set_key(struct cube *cube, int key, void *val)
{
	struct w_node *w_node;
	struct x_node *x_node;
	struct y_node *y_node;

	unsigned short mid, w, x, y, z;

	if (cube->w_size == 0)
	{
		cube->m_size = BSC_M;

		cube->w_floor = (int *) malloc(BSC_M * sizeof(int));
		cube->w_axis = (struct w_node **) malloc(BSC_M * sizeof(struct w_node *));
		cube->w_volume = (int *) malloc(BSC_M * sizeof(int));
		cube->x_size = (unsigned short *) malloc(BSC_M * sizeof(unsigned short));

		w_node = cube->w_axis[0] = (struct w_node *) malloc(sizeof(struct w_node));

		w_node->x_floor = (int *) malloc(BSC_M * sizeof(int));
		w_node->x_axis = (struct x_node **) malloc(BSC_M * sizeof(struct x_node *));
		w_node->y_size = (unsigned short *) malloc(BSC_M * sizeof(unsigned short));
		w_node->x_volume = (unsigned short *) malloc(BSC_M * sizeof(unsigned short));

		x_node = w_node->x_axis[0] = (struct x_node *) malloc(sizeof(struct x_node));

		x_node->y_floor = (int *) malloc(BSC_M * sizeof(int));
		x_node->y_axis = (struct y_node **) malloc(BSC_M * sizeof(struct y_node *));
		x_node->z_size = (unsigned char *) malloc(BSC_M * sizeof(unsigned char));

		y_node = x_node->y_axis[0] = (struct y_node *) malloc(sizeof(struct y_node));

		x_node->z_size[0] = 0;

		cube->w_size = cube->x_size[0] = w_node->y_size[0] = 1;
		cube->volume = cube->w_volume[0] = w_node->x_volume[0] = 0;

		w = x = y = z = 0;

		cube->w_floor[0] = w_node->x_floor[0] = x_node->y_floor[0] = key;

		goto insert;
	}

	if (key < cube->w_floor[0])
	{
		w_node = cube->w_axis[0];
		x_node = w_node->x_axis[0];
		y_node = x_node->y_axis[0];

		w = x = y = z = 0;

		cube->w_floor[0] = w_node->x_floor[0] = x_node->y_floor[0] = key;

		goto insert;
	}

	// w

	mid = w = cube->w_size - 1;

	while (mid > 3)
	{
		mid /= 2;

		if (key < cube->w_floor[w - mid]) w -= mid;
	}
	while (key < cube->w_floor[w]) --w;

	w_node = cube->w_axis[w];

	// x

	mid = x = cube->x_size[w] - 1;

	while (mid > 3)
	{
		mid /= 2;

		if (key < w_node->x_floor[x - mid]) x -= mid;
	}
	while (key < w_node->x_floor[x]) --x;

	x_node = w_node->x_axis[x];

	// y

	mid = y = w_node->y_size[x] - 1;

	while (mid > 7)
	{
		mid /= 4;

		if (key < x_node->y_floor[y - mid])
		{
			y -= mid;
			if (key < x_node->y_floor[y - mid])
			{
				y -= mid;
				if (key < x_node->y_floor[y - mid])
				{
					y -= mid;
				}
			}
		}
	}
	while (key < x_node->y_floor[y]) --y;

	y_node = x_node->y_axis[y];

	// z

	mid = z = x_node->z_size[y] - 1;

	while (mid > 7)
	{
		mid /= 4;

		if (key < y_node->z_keys[z - mid])
		{
			z -= mid;
			if (key < y_node->z_keys[z - mid])
			{
				z -= mid;
				if (key < y_node->z_keys[z - mid])
				{
					z -= mid;
				}
			}
		}
	}
	while (key < y_node->z_keys[z]) --z;

	if (key == y_node->z_keys[z])
	{
		y_node->z_vals[z] = val;

		return;
	}

	++z;

	insert:

	++cube->volume;
	++cube->w_volume[w];
	++w_node->x_volume[x];

	++x_node->z_size[y];

	if (z + 1 != x_node->z_size[y])
	{
		memmove(&y_node->z_keys[z + 1], &y_node->z_keys[z], (x_node->z_size[y] - z - 1) * sizeof(int));
		memmove(&y_node->z_vals[z + 1], &y_node->z_vals[z], (x_node->z_size[y] - z - 1) * sizeof(void *));
	}

	y_node->z_keys[z] = key;
	y_node->z_vals[z] = val;

	if (x_node->z_size[y] == BSC_Z_MAX)
	{
		split_y_node(cube, w, x, y);

		if (cube->w_axis[w]->y_size[x] == cube->m_size)
		{
			split_x_node(cube, w, x);

			if (cube->x_size[w] == cube->m_size)
			{
				split_w_node(cube, w);
			}
		}
	}
}

inline void *find_key(struct cube *cube, int key, unsigned short *w_index, unsigned short *x_index, unsigned short *y_index, unsigned short *z_index)
{
	struct w_node *w_node;
	struct x_node *x_node;
	struct y_node *y_node;

	unsigned short mid, w, x, y, z;

	if (cube->w_size == 0 || key < cube->w_floor[0])
	{
		*w_index = *x_index = *y_index = *z_index = 0;

		return NULL;
	}

	// w

	mid = w = cube->w_size - 1;

	while (mid > 3)
	{
		mid /= 2;

		if (key < cube->w_floor[w - mid]) w -= mid;
	}
	while (key < cube->w_floor[w]) --w;

	w_node = cube->w_axis[w];

	// x

	mid = x = cube->x_size[w] - 1;

	while (mid > 3)
	{
		mid /= 2;

		if (key < w_node->x_floor[x - mid]) x -= mid;
	}
	while (key < w_node->x_floor[x]) --x;

	x_node = w_node->x_axis[x];

	// y

	mid = y = w_node->y_size[x] - 1;

	while (mid > 7)
	{
		mid /= 4;

		if (key < x_node->y_floor[y - mid])
		{
			y -= mid;
			if (key < x_node->y_floor[y - mid])
			{
				y -= mid;
				if (key < x_node->y_floor[y - mid])
				{
					y -= mid;
				}
			}
		}
	}
	while (key < x_node->y_floor[y]) --y;

	y_node = x_node->y_axis[y];

	// z

	mid = z = x_node->z_size[y] - 1;

	while (mid > 7)
	{
		mid /= 4;

		if (key < y_node->z_keys[z - mid])
		{
			z -= mid;
			if (key < y_node->z_keys[z - mid])
			{
				z -= mid;
				if (key < y_node->z_keys[z - mid])
				{
					z -= mid;
				}
			}
		}
	}
	while (key < y_node->z_keys[z]) --z;

	*w_index = w;
	*x_index = x;
	*y_index = y;

	if (key == y_node->z_keys[z])
	{
		*z_index = z;

		return y_node->z_vals[z];
	}

	*z_index = z + 1;

	return NULL;
}

inline void *find_index(struct cube *cube, int index, unsigned short *w_index, unsigned short *x_index, unsigned short *y_index, unsigned short *z_index)
{
	struct w_node *w_node;
	struct x_node *x_node;
	struct y_node *y_node;
	register unsigned short w, x, y;
	int total;

	if (index < 0 || index >= cube->volume)
	{
		return NULL;
	}

	if (index < cube->volume / 2)
	{
		total = 0;

		for (w = 0 ; w < cube->w_size ; w++)
		{
			w_node = cube->w_axis[w];

			if (total + cube->w_volume[w] > index)
			{
				if (index > total + cube->w_volume[w] / 2)
				{
					total += cube->w_volume[w];

					goto backward_x;
				}
				forward_x:

				for (x = 0 ; x < cube->x_size[w] ; x++)
				{
					x_node = w_node->x_axis[x];

					if (total + w_node->x_volume[x] > index)
					{
						if (index > total + w_node->x_volume[x] / 2)
						{
							total += w_node->x_volume[x];

							goto backward_y;
						}
						forward_y:

						for (y = 0 ; y < w_node->y_size[x] ; y++)
						{
							y_node = x_node->y_axis[y];

							if (total + x_node->z_size[y] > index)
							{
								*w_index = w;
								*x_index = x;
								*y_index = y;
								*z_index = index - total;

								return y_node->z_vals[index - total];
							}
							total += x_node->z_size[y];
						}
					}
					total += w_node->x_volume[x];
				}
			}
			total += cube->w_volume[w];
		}
	}
	else
	{
		total = cube->volume;

		for (w = cube->w_size - 1 ; w >= 0 ; w--)
		{
			w_node = cube->w_axis[w];

			if (total - cube->w_volume[w] <= index)
			{
				if (index < total - cube->w_volume[w] / 2)
				{
					total -= cube->w_volume[w];

					goto forward_x;
				}
				backward_x:

				for (x = cube->x_size[w] - 1 ; x >= 0 ; x--)
				{
					x_node = w_node->x_axis[x];

					if (total - w_node->x_volume[x] <= index)
					{
						if (index < total - w_node->x_volume[x] / 2)
						{
							total -= w_node->x_volume[x];

							goto forward_y;
						}
						backward_y:

						for (y = w_node->y_size[x] - 1 ; y >= 0 ; y--)
						{
							y_node = x_node->y_axis[y];

							if (total - x_node->z_size[y] <= index)
							{
								*w_index = w;
								*x_index = x;
								*y_index = y;
								*z_index = x_node->z_size[y] - (total - index);

								return y_node->z_vals[x_node->z_size[y] - (total - index)];
							}
							total -= x_node->z_size[y];
						}
					}
					total -= w_node->x_volume[x];
				}
			}
			total -= cube->w_volume[w];
		}
	}
	return NULL;
}

inline void insert_w_node(struct cube *cube, unsigned short w)
{
	struct w_node *w_node;

	++cube->w_size;

	if (cube->w_size == cube->m_size)
	{
		cube->m_size += BSC_M;

		cube->w_floor = (int *) realloc(cube->w_floor, cube->m_size * sizeof(int));
		cube->w_axis = (struct w_node **) realloc(cube->w_axis, cube->m_size * sizeof(struct w_node *));
		cube->w_volume = (int *) realloc(cube->w_volume, cube->m_size * sizeof(int));
		cube->x_size = (unsigned short *) realloc(cube->x_size, cube->m_size * sizeof(unsigned short));
	}

	if (w + 1 != cube->w_size)
	{
		memmove(&cube->w_floor[w + 1], &cube->w_floor[w], (cube->w_size - w - 1) * sizeof(int));
		memmove(&cube->w_axis[w + 1], &cube->w_axis[w], (cube->w_size - w - 1) * sizeof(struct w_node *));
		memmove(&cube->w_volume[w + 1], &cube->w_volume[w], (cube->w_size - w - 1) * sizeof(int));
		memmove(&cube->x_size[w + 1], &cube->x_size[w], (cube->w_size - w - 1) * sizeof(unsigned short));
	}

	w_node = cube->w_axis[w] = (struct w_node *) malloc(sizeof(struct w_node));

	w_node->x_floor = (int *) malloc(cube->m_size * sizeof(int));
	w_node->x_axis = (struct x_node **) malloc(cube->m_size * sizeof(struct x_node *));
	w_node->y_size = (unsigned short *) malloc(cube->m_size * sizeof(unsigned short));
	w_node->x_volume = (unsigned short *) malloc(cube->m_size * sizeof(unsigned short));
}

void remove_w_node(struct cube *cube, unsigned short w)
{
	cube->w_size--;

	free(cube->w_axis[w]);

	if (cube->w_size < cube->m_size - BSC_M)
	{
		cube->m_size -= BSC_M;
	}

	if (cube->w_size)
	{
		if (cube->w_size != w)
		{
			memmove(&cube->w_floor[w], &cube->w_floor[w + 1], (cube->w_size - w) * sizeof(int));
			memmove(&cube->w_axis[w], &cube->w_axis[w + 1], (cube->w_size - w) * sizeof(struct w_node *));
			memmove(&cube->w_volume[w], &cube->w_volume[w + 1], (cube->w_size - w) * sizeof(int));
			memmove(&cube->x_size[w], &cube->x_size[w + 1], (cube->w_size - w) * sizeof(unsigned short));
		}
	}
	else
	{
		free(cube->w_floor);
		free(cube->w_axis);
		free(cube->w_volume);
		free(cube->x_size);
	}
}

inline void insert_x_node(struct cube *cube, unsigned short w, unsigned short x)
{
	struct w_node *w_node = cube->w_axis[w];
	struct x_node *x_node;

	unsigned short x_size = ++cube->x_size[w];

	if (x_size % BSC_M == 0 && x_size < cube->m_size)
	{
		w_node->x_floor = (int *) realloc(w_node->x_floor, cube->m_size * sizeof(int));
		w_node->x_axis = (struct x_node **) realloc(w_node->x_axis, cube->m_size * sizeof(struct x_node *));
		w_node->x_volume = (unsigned short *) realloc(w_node->x_volume, cube->m_size * sizeof(unsigned short));
		w_node->y_size = (unsigned short *) realloc(w_node->y_size, cube->m_size * sizeof(unsigned short));
	}

	if (x_size != x + 1)
	{
		memmove(&w_node->x_floor[x + 1], &w_node->x_floor[x], (x_size - x - 1) * sizeof(int));
		memmove(&w_node->x_axis[x + 1], &w_node->x_axis[x], (x_size - x - 1) * sizeof(struct x_node *));
		memmove(&w_node->x_volume[x + 1], &w_node->x_volume[x], (x_size - x - 1) * sizeof(unsigned short));
		memmove(&w_node->y_size[x + 1], &w_node->y_size[x], (x_size - x - 1) * sizeof(unsigned short));
	}

	x_node = w_node->x_axis[x] = (struct x_node *) malloc(sizeof(struct x_node));

	x_node->y_floor = (int *) malloc(cube->m_size * sizeof(int));
	x_node->y_axis = (struct y_node **) malloc(cube->m_size * sizeof(struct y_node *));
	x_node->z_size = (unsigned char *) malloc(cube->m_size * sizeof(unsigned char));
}

void remove_x_node(struct cube *cube, unsigned short w, unsigned short x)
{
	struct w_node *w_node = cube->w_axis[w];

	cube->x_size[w]--;

	free(w_node->x_axis[x]);

	if (cube->x_size[w])
	{
		if (cube->x_size[w] != x)
		{
			memmove(&w_node->x_floor[x], &w_node->x_floor[x + 1], (cube->x_size[w] - x ) * sizeof(int));
			memmove(&w_node->x_axis[x], &w_node->x_axis[x + 1], (cube->x_size[w] - x ) * sizeof(struct x_node *));
			memmove(&w_node->x_volume[x], &w_node->x_volume[x + 1], (cube->x_size[w] - x ) * sizeof(unsigned short));
			memmove(&w_node->y_size[x], &w_node->y_size[x + 1], (cube->x_size[w] - x ) * sizeof(unsigned short));
		}

		if (x == 0)
		{
			cube->w_floor[w] = w_node->x_floor[0];
		}
	}
	else
	{
		remove_w_node(cube, w);
	}
}

inline void insert_y_node(struct cube *cube, unsigned short w, unsigned short x, unsigned short y)
{
	struct x_node *x_node = cube->w_axis[w]->x_axis[x];

	unsigned short y_size = ++cube->w_axis[w]->y_size[x];

	if (y_size % BSC_M == 0 && y_size < cube->m_size)
	{
		x_node->y_floor = (int *) realloc(x_node->y_floor, cube->m_size * sizeof(int));
		x_node->y_axis = (struct y_node **) realloc(x_node->y_axis, cube->m_size * sizeof(struct y_node *));
		x_node->z_size = (unsigned char *) realloc(x_node->z_size, cube->m_size * sizeof(unsigned char));
	}

	if (y_size != y + 1)
	{
		memmove(&x_node->y_floor[y + 1], &x_node->y_floor[y], (y_size - y - 1) * sizeof(int));
		memmove(&x_node->y_axis[y + 1], &x_node->y_axis[y], (y_size - y - 1) * sizeof(struct y_node *));
		memmove(&x_node->z_size[y + 1], &x_node->z_size[y], (y_size - y - 1) * sizeof(unsigned char));
	}

	x_node->y_axis[y] = (struct y_node *) malloc(sizeof(struct y_node));
}

void remove_y_node(struct cube *cube, unsigned short w, unsigned short x, unsigned short y)
{
	struct w_node *w_node = cube->w_axis[w];
	struct x_node *x_node = w_node->x_axis[x];

	w_node->y_size[x]--;

	free(x_node->y_axis[y]);

	if (w_node->y_size[x])
	{
		if (w_node->y_size[x] != y)
		{
			memmove(&x_node->y_floor[y], &x_node->y_floor[y + 1], (w_node->y_size[x] - y ) * sizeof(int));
			memmove(&x_node->y_axis[y], &x_node->y_axis[y + 1], (w_node->y_size[x] - y ) * sizeof(struct y_node *));
			memmove(&x_node->z_size[y], &x_node->z_size[y + 1], (w_node->y_size[x] - y ) * sizeof(unsigned char));
		}

		if (y == 0)
		{
			cube->w_axis[w]->x_floor[x] = x_node->y_floor[0];

			if (x == 0)
			{
				cube->w_floor[w] = x_node->y_floor[0];
			}
		}
	}
	else
	{
		remove_x_node(cube, w, x);
	}
}

inline void *remove_z_node(struct cube *cube, unsigned short w, unsigned short x, unsigned short y, unsigned short z)
{
	struct w_node *w_node = cube->w_axis[w];
	struct x_node *x_node = w_node->x_axis[x];
	struct y_node *y_node = x_node->y_axis[y];
	void *val;

	cube->volume--;

	cube->w_volume[w]--;
	w_node->x_volume[x]--;

	x_node->z_size[y]--;

	val = y_node->z_vals[z];

	if (x_node->z_size[y] != z)
	{
		memmove(&y_node->z_keys[z], &y_node->z_keys[z + 1], (x_node->z_size[y] - z) * sizeof(int));
		memmove(&y_node->z_vals[z], &y_node->z_vals[z + 1], (x_node->z_size[y] - z) * sizeof(void *));
	}

	if (x_node->z_size[y])
	{
		if (z == 0)
		{
			x_node->y_floor[y] = y_node->z_keys[z];

			if (y == 0)
			{
				w_node->x_floor[x] = y_node->z_keys[z];

				if (x == 0)
				{
					cube->w_floor[w] = y_node->z_keys[z];
				}
			}
		}

		if (y && x_node->z_size[y] < BSC_Z_MIN && x_node->z_size[y - 1] < BSC_Z_MIN)
		{
			merge_y_node(cube, w, x, y - 1, y);

			if (x && w_node->y_size[x] < cube->m_size / 4 && w_node->y_size[x - 1] < cube->m_size / 4)
			{
				merge_x_node(cube, w, x - 1, x);

				if (w && cube->x_size[w] < cube->m_size / 4 && cube->x_size[w - 1] < cube->m_size / 4)
				{
					merge_w_node(cube, w - 1, w);
				}
			}
		}
	}
	else
	{
		remove_y_node(cube, w, x, y);
	}
	return val;
}

void split_w_node(struct cube *cube, unsigned short w)
{
	struct w_node *w_node1, *w_node2;
	unsigned short x;
	int volume;

	insert_w_node(cube, w + 1);

	w_node1 = cube->w_axis[w];
	w_node2 = cube->w_axis[w + 1];

	cube->x_size[w + 1] = cube->x_size[w] / 2;
	cube->x_size[w] -= cube->x_size[w + 1];

	memcpy(&w_node2->x_floor[0], &w_node1->x_floor[cube->x_size[w]], cube->x_size[w + 1] * sizeof(int));
	memcpy(&w_node2->x_axis[0], &w_node1->x_axis[cube->x_size[w]], cube->x_size[w + 1] * sizeof(struct x_node *));
	memcpy(&w_node2->x_volume[0], &w_node1->x_volume[cube->x_size[w]], cube->x_size[w + 1] * sizeof(unsigned short));
	memcpy(&w_node2->y_size[0], &w_node1->y_size[cube->x_size[w]], cube->x_size[w + 1] * sizeof(unsigned short));

	for (x = volume = 0 ; x < cube->x_size[w] ; x++)
	{
		volume += w_node1->x_volume[x];
	}

	cube->w_volume[w + 1] = cube->w_volume[w] - volume;
	cube->w_volume[w] = volume;

	cube->w_floor[w + 1] = w_node2->x_floor[0];
}

void merge_w_node(struct cube *cube, unsigned short w1, unsigned short w2)
{
	struct w_node *w_node1 = cube->w_axis[w1];
	struct w_node *w_node2 = cube->w_axis[w2];

	w_node1->x_floor = (int *) realloc(w_node1->x_floor, cube->m_size * sizeof(int));
	w_node1->x_axis = (struct x_node **) realloc(w_node1->x_axis, cube->m_size * sizeof(struct x_node *));
	w_node1->x_volume = (unsigned short *) realloc(w_node1->x_volume, cube->m_size * sizeof(unsigned short));
	w_node1->y_size = (unsigned short *) realloc(w_node1->y_size, cube->m_size * sizeof(unsigned short));

	memcpy(&w_node1->x_floor[cube->x_size[w1]], &w_node2->x_floor[0], cube->x_size[w2] * sizeof(int));
	memcpy(&w_node1->x_axis[cube->x_size[w1]], &w_node2->x_axis[0], cube->x_size[w2] * sizeof(struct x_node *));
	memcpy(&w_node1->x_volume[cube->x_size[w1]], &w_node2->x_volume[0], cube->x_size[w2] * sizeof(unsigned short));
	memcpy(&w_node1->y_size[cube->x_size[w1]], &w_node2->y_size[0], cube->x_size[w2] * sizeof(unsigned short));

	cube->x_size[w1] += cube->x_size[w2];

	cube->w_volume[w1] += cube->w_volume[w2];

	remove_w_node(cube, w2);
}

void split_x_node(struct cube *cube, unsigned short w, unsigned short x)
{
	struct w_node *w_node;
	struct x_node *x_node1, *x_node2;
	unsigned short y;
	int volume;

	insert_x_node(cube, w, x + 1);

	w_node = cube->w_axis[w];

	x_node1 = w_node->x_axis[x];
	x_node2 = w_node->x_axis[x + 1];

	w_node->y_size[x + 1] = w_node->y_size[x] / 2;
	w_node->y_size[x] -= w_node->y_size[x + 1];

	memcpy(&x_node2->y_floor[0], &x_node1->y_floor[w_node->y_size[x]], w_node->y_size[x + 1] * sizeof(int));
	memcpy(&x_node2->y_axis[0], &x_node1->y_axis[w_node->y_size[x]], w_node->y_size[x + 1] * sizeof(struct y_node *));
	memcpy(&x_node2->z_size[0], &x_node1->z_size[w_node->y_size[x]], w_node->y_size[x + 1] * sizeof(unsigned char));

	for (y = volume = 0 ; y < w_node->y_size[x] ; y++)
	{
		volume += x_node1->z_size[y];
	}

	w_node->x_volume[x + 1] = w_node->x_volume[x] - volume;
	w_node->x_volume[x] = volume;

	cube->w_axis[w]->x_floor[x + 1] = x_node2->y_floor[0];
}

void merge_x_node(struct cube *cube, unsigned short w, unsigned short x1, unsigned short x2)
{
	struct w_node *w_node = cube->w_axis[w];
	struct x_node *x_node1 = w_node->x_axis[x1];
	struct x_node *x_node2 = w_node->x_axis[x2];

	x_node1->y_floor = (int *) realloc(x_node1->y_floor, cube->m_size * sizeof(int));
	x_node1->y_axis = (struct y_node **) realloc(x_node1->y_axis, cube->m_size * sizeof(struct y_node *));
	x_node1->z_size = (unsigned char *) realloc(x_node1->z_size, cube->m_size * sizeof(unsigned char));

	memcpy(&x_node1->y_floor[w_node->y_size[x1]], &x_node2->y_floor[0], w_node->y_size[x2] * sizeof(int));
	memcpy(&x_node1->y_axis[w_node->y_size[x1]], &x_node2->y_axis[0], w_node->y_size[x2] * sizeof(struct y_node *));
	memcpy(&x_node1->z_size[w_node->y_size[x1]], &x_node2->z_size[0], w_node->y_size[x2] * sizeof(unsigned char));

	w_node->y_size[x1] += w_node->y_size[x2];

	w_node->x_volume[x1] += w_node->x_volume[x2];

	remove_x_node(cube, w, x2);
}


void split_y_node(struct cube *cube, unsigned short w, unsigned short x, unsigned short y)
{
	struct x_node *x_node;
	struct y_node *y_node1, *y_node2;

	insert_y_node(cube, w, x, y + 1);

	x_node = cube->w_axis[w]->x_axis[x];

	y_node1 = x_node->y_axis[y];
	y_node2 = x_node->y_axis[y + 1];

	x_node->z_size[y + 1] = x_node->z_size[y] / 2;
	x_node->z_size[y] -= x_node->z_size[y + 1];

	memcpy(&y_node2->z_keys[0], &y_node1->z_keys[x_node->z_size[y]], x_node->z_size[y + 1] * sizeof(int));
	memcpy(&y_node2->z_vals[0], &y_node1->z_vals[x_node->z_size[y]], x_node->z_size[y + 1] * sizeof(void *));

	x_node->y_floor[y + 1] = y_node2->z_keys[0];
}

void merge_y_node(struct cube *cube, unsigned short w, unsigned short x, unsigned short y1, unsigned short y2)
{
	struct x_node *x_node = cube->w_axis[w]->x_axis[x];

	struct y_node *y_node1 = x_node->y_axis[y1];
	struct y_node *y_node2 = x_node->y_axis[y2];

	memcpy(&y_node1->z_keys[x_node->z_size[y1]], &y_node2->z_keys[0], x_node->z_size[y2] * sizeof(int));
	memcpy(&y_node1->z_vals[x_node->z_size[y1]], &y_node2->z_vals[0], x_node->z_size[y2] * sizeof(void *));

	x_node->z_size[y1] += x_node->z_size[y2];

	remove_y_node(cube, w, x, y2);
}

void show_cube(struct cube *cube, unsigned short depth)
{
	struct w_node *w_node;
	struct x_node *x_node;
	struct y_node *y_node;
	unsigned short w, x, y, z;

	for (w = 0 ; w < cube->w_size ; w++)
	{
		w_node = cube->w_axis[w];

		if (depth == 1)
		{
			printf("w index [%3d] x size [%3d] volume [%10d]\n", w, cube->x_size[w], cube->w_volume[w]);

			continue;
		}

		for (x = 0 ; x < cube->x_size[w] ; x++)
		{
			x_node = w_node->x_axis[x];

			if (depth == 2)
			{
				printf("w [%3d] x [%3d] s [%3d] v [%10d]\n", w, x, w_node->y_size[x], w_node->x_volume[x]);

				continue;
			}

			for (y = 0 ; y < w_node->y_size[x] ; y++)
			{
				y_node = x_node->y_axis[y];

				if (depth == 3)
				{
					printf("w [%3d] x [%3d] y [%3d] s [%3d]\n", w, x, y, x_node->z_size[y]);

					continue;
				}

				for (z = 0 ; z < x_node->z_size[y] ; z++)
				{
					printf("w [%3d] x [%3d] y [%3d] z [%3d] [%010d] (%s)\n", w, x, y, z, y_node->z_keys[z], (char *) y_node->z_vals[z]);
				}
			}
		}
	}
}

void check_integrity(struct cube *cube, char *msg)
{
	struct y_node *y_node;
	unsigned short w, x, y, z;
	int last;

	return;

	if (cube->w_size == 0)
	{
		return;
	}

	last = cube->w_floor[0];

	for (w = 0 ; w < cube->w_size ; w++)
	{
		for (x = 0 ; x < cube->x_size[w] ; x++)
		{
			for (y = 0 ; y < cube->w_axis[w]->y_size[x] ; y++)
			{
				for (z = 0 ; z < cube->w_axis[w]->x_axis[x]->z_size[y] ; z++)
				{
					y_node = cube->w_axis[w]->x_axis[x]->y_axis[y];

					if (last > y_node->z_keys[z])
					{
						printf("\e[1;31mcheck integrity: corruption %s.\e[0m\n", msg);
						return;
					}
					last = y_node->z_keys[z];
				}
			}
		}
	}
}

long long utime()
{
	struct timeval now_time;

	gettimeofday(&now_time, NULL);

	return now_time.tv_sec * 1000000LL + now_time.tv_usec;
}

int main(int argc, char **argv)
{
	static int max = 1000000;
	int cnt, loop;
	long long start, end;
	void *val;
	struct cube *cube;

	if (argv[1] && *argv[1])
	{
		printf("%s\n", argv[1]);
	}

	val = strdup("value");

	cube = create_cube();
	start = utime();
	srand(10);

	for (loop = 0 ; loop < 1 ; loop++)
	{
		destroy_cube(cube);
		cube = create_cube();
		for (cnt = 1 ; cnt <= max ; cnt++)
		{
			set_key(cube, 0 + rand(), val);
		}
	}
	end = utime();
	printf("Time to insert %d elements: %f seconds. (random order) (w_size %d)\n", max, (end - start) / 1000000.0, cube->w_size);

	check_integrity(cube, "rnd order");

	srand(10);

	while (cube->volume)
	{
		del_index(cube, cube->volume - 1);
	}
	end = utime();
	printf("Time to delete %d elements: %f seconds. (random order) (w_size %d)\n", max, (end - start) / 1000000.0, cube->w_size);

	destroy_cube(cube);

	cube = create_cube();
	start = utime();
	for (cnt = 1 ; cnt <= max ; cnt++)
	{
		set_key(cube, 0 + cnt, "fwd order");
	}
	end = utime();
	printf("Time to insert %d elements: %f seconds. (forward order)\n", max, (end - start) / 1000000.0);

	check_integrity(cube, "fwd order");

	destroy_cube(cube);

	cube = create_cube();
	start = utime();
	for (cnt = 1 ; cnt <= max ; cnt++)
	{
		set_key(cube, max - cnt, "rev order");
	}
	end = utime();

	check_integrity(cube, "rev order");

	printf("Time to insert %d elements: %f seconds. (reverse order)\n", max, (end - start) / 1000000.0);
	destroy_cube(cube);

	return 0;
}
