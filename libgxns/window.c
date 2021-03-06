/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   window.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: njaber <neyl.jaber@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2018/01/26 00:14:34 by njaber            #+#    #+#             */
/*   Updated: 2018/06/15 08:10:50 by njaber           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libgxns.h"

void	paint_window(t_win *win, t_kernel *opencl_kernel, char use_motion_blur)
{
	uint64_t	time;

	time = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
	if (++win->frame % 60 == 0)
		win->fps = 30000000000 / (double)(time - win->frames[0]);
	if (opencl_kernel == NULL)
		mlx_clear_window(win->mlx, win->win);
	mlx_put_image_to_window(win->mlx, win->win, win->img.link, 0, 0);
	if (opencl_kernel != NULL)
	{
		clSetKernelArg(opencl_kernel->cores[1], 1, sizeof(int),
				(int[1]){use_motion_blur ? 0xAF000000 : 0x00000000});
		clEnqueueNDRangeKernel(opencl_kernel->opencl-> gpu_command_queue,
				opencl_kernel->cores[1], 1, NULL, (size_t[1])
				{win->img.px_size / 8 * win->img.size.x * win->img.size.y / 4},
				NULL, 0, NULL, NULL);
		clFinish(opencl_kernel->opencl->gpu_command_queue);
	}
	else
		clear_img(&win->img);
	win->frames[win->frame % 30] = time;
}

int		init_new_win(t_win *win, t_ivec size, char *title)
{
	if ((win->mlx = mlx_init()) == NULL ||
		(win->win = mlx_new_window(win->mlx,
			size.x, size.y, title)) == NULL)
		return (0);
	init_new_image(win, &(win->img), size);
	win->size = size;
	win->frame = 0;
	win->fps = 0;
	ft_bzero(win->frames, 30 * sizeof(uint64_t));
	return (1);
}
