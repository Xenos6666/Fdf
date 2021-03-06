/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: njaber <neyl.jaber@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2018/01/19 20:17:07 by njaber            #+#    #+#             */
/*   Updated: 2018/04/12 14:51:35 by njaber           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libgxns/libgxns.h"
#include "fdf.h"

static char		**read_file(int fd, t_map *map)
{
	char	**ret;
	char	*tmp;
	size_t	b_size;
	size_t	i;
	int		rd;

	b_size = 16;
	if ((ret = (char**)ft_memalloc(b_size * sizeof(char*))) == NULL)
		ft_error("[Erreur] Echec d'allocation mémoire\n");
	i = 0;
	while ((rd = get_next_line(fd, &tmp)) > 0)
	{
		if (i >= b_size && (b_size *= 4))
			ft_realloc((void**)&ret, b_size / 4 * sizeof(char*),
					b_size * sizeof(char*));
		ret[i] = tmp;
		i++;
	}
	map->y = i;
	if ((map->height = (int**)ft_memalloc(sizeof(int*) * map->y)) == NULL)
		ft_error("[Erreur] Echec d'allocation mémoire\n");
	return (ret);
}

static size_t	count_col(char *line)
{
	size_t	ret;

	ret = 0;
	while (*line != '\0')
	{
		if ((ret == 0 || ft_isblank(*(line - 1))) && !ft_isblank(*line))
			ret++;
		line++;
	}
	return (ret);
}

static void		get_map(int fd, t_map *map)
{
	char	**buf;
	int		i;
	int		j;
	size_t	digit;

	buf = read_file(fd, map);
	i = 0;
	map->x = 0;
	while (i < (int)map->y)
		if ((j = count_col(buf[i++])) > (int)map->x)
			map->x = j;
	i = -1;
	while (++i < (int)map->y && (digit = 0) == 0)
	{
		j = -1;
		if ((map->height[i] = (int*)ft_memalloc(sizeof(int) * map->x)) == NULL)
			ft_error("[Erreur] Echec d'allocation mémoire\n");
		while (buf[i][++j] != '\0' && digit < map->x)
		{
			if (!ft_isblank(buf[i][j]) && (j == 0 || ft_isblank(buf[i][j - 1])))
				map->height[i][digit++] = ft_atoi(buf[i] + j);
		}
		free(buf[i]);
	}
	free(buf);
}

void			init_struct(int fd, t_ptr *p)
{
	get_map(fd, p->map);
	p->rot = (t_vec2){0, 0};
	p->dest_rot = (t_vec2){50, 45};
	p->z_size = 0.1;
	p->zoom = 10;
	p->dest_zoom = ft_max(p->map->x, p->map->y);
	p->max_zoom = ft_max(50, p->dest_zoom);
	p->button = -1;
	p->pos = (t_vec3){0, 0, 0};
	p->dest_pos = (t_vec3){0, 0, 0};
	p->near = 1;
	p->far = 100;
	p->fov = 90;
	p->is_perspective_active = 1;
	p->use_opencl = 1;
	p->use_motion_blur = 1;
	p->aliasing = 2;
	p->fog = 3.0;
	ft_memcpy(p->colors, (unsigned int[5]){0x444444, 0xDDDDDD,
			0x0000FF, 0xFF7700, 0x00FF00}, sizeof(unsigned int[5]));
	p->opencl = init_opencl();
	if ((init_new_win(p->win, (t_ivec){1200, 800}, "FdF")) == 0)
		ft_error("[Erreur] Echec de l'intialization de la fenètre\n");
	create_kernel(p);
	ft_bzero(p->keys, 512);
}

void			set_hooks(t_ptr *p)
{
	mlx_loop_hook(p->win->mlx, loop_hook, (void*)p);
	mlx_expose_hook(p->win->win, loop_hook, (void*)p);
	mlx_hook(p->win->win, 6, 0, motion_hook, (void*)p);
	mlx_hook(p->win->win, 4, 0, button_press_hook, (void*)p);
	mlx_hook(p->win->win, 5, 0, button_release_hook, (void*)p);
	mlx_hook(p->win->win, 2, 0, key_press_hook, (void*)p);
	mlx_hook(p->win->win, 3, 0, key_release_hook, (void*)p);
}
