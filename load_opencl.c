/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   load_opencl.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: njaber <neyl.jaber@gmail.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2018/03/16 20:23:49 by njaber            #+#    #+#             */
/*   Updated: 2018/06/15 07:39:37 by njaber           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "fdf.h"

static void		generate_vbo(t_ptr *p)
{
	size_t		i;

	if ((p->vbo = (float*)ft_memalloc(p->map->x * p->map->y *
			sizeof(float[4]))) == NULL)
		ft_error("[Erreur] Echec d'allocation mémoire.");
	i = 0;
	while (i < p->map->x * p->map->y)
	{
		p->vbo[i * 4] = i % p->map->x;
		p->vbo[i * 4 + 1] = i / p->map->x;
		p->vbo[i * 4 + 2] = p->map->height[i / p->map->x][i % p->map->x];
		p->vbo[i * 4 + 3] = 1.0;
		i++;
	}
}

static void		generate_idx(t_ptr *p)
{
	size_t		i;
	size_t		tmp;

	if ((p->vbo_idx = (unsigned int*)ft_memalloc(((p->map->x - 1) * p->map->y +
			p->map->x * (p->map->y - 1)) * sizeof(unsigned int[2]))) == NULL)
		ft_error("[Erreur] Echec d'allocation mémoire.");
	i = 0;
	while (i < (p->map->x - 1) * p->map->y + p->map->x * (p->map->y - 1))
	{
		tmp = i / (p->map->x * 2 - 1) * p->map->x;
		p->vbo_idx[i * 2] = tmp + (i + p->map->x) % (p->map->x * 2 - 1)
			% p->map->x;
		p->vbo_idx[i * 2 + 1] = tmp + ((i + p->map->x) % (p->map->x * 2 - 1)
				% p->map->x) + (i % (p->map->x * 2 - 1) < p->map->x - 1 ?
					1 : p->map->x);
		i++;
	}
	p->vbo_size = (p->map->x - 1) * p->map->y + p->map->x * (p->map->y - 1);
}

static int		create_memobjs(t_ptr *p, t_ocl *opencl,
		t_kernel *kernel, t_img *img)
{
	int err;

	generate_vbo(p);
	generate_idx(p);
	if ((kernel->memobjs = (cl_mem*)ft_memalloc(sizeof(cl_mem) * 8)) == NULL)
		ft_error("[Erreur] Echec d'allocation mémoire.");
	kernel->memobjs[0] = clCreateBuffer(opencl->gpu_context, CL_MEM_READ_WRITE |
			CL_MEM_COPY_HOST_PTR, img->line * img->size.y, img->buf, &err);
	kernel->memobjs[1] = clCreateBuffer(opencl->gpu_context,
			CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			p->map->x * p->map->y * sizeof(float[4]), p->vbo, &err);
	kernel->memobjs[2] = clCreateBuffer(opencl->gpu_context,
			CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			((p->map->x - 1) * p->map->y + p->map->x * (p->map->y - 1))
			* sizeof(unsigned int[2]), p->vbo_idx, &err);
	err |= clSetKernelArg(kernel->cores[0], 0,
			sizeof(cl_mem), (void*)&kernel->memobjs[0]);
	err |= clSetKernelArg(kernel->cores[0], 1,
			sizeof(cl_mem), (void*)&kernel->memobjs[1]);
	err |= clSetKernelArg(kernel->cores[0], 2,
			sizeof(cl_mem), (void*)&kernel->memobjs[2]);
	err |= clSetKernelArg(kernel->cores[0], 3, sizeof(size_t), &p->vbo_size);
	err |= clSetKernelArg(kernel->cores[1], 0,
			sizeof(cl_mem), (void*)&kernel->memobjs[0]);
	return (err);
}

static int		build_program(t_ocl *opencl, t_kernel *kernel)
{
	char		*tmp;
	size_t		tmp2;
	int			err;

	kernel->program = create_program_from_file(opencl->gpu_context,
			"libgxns/kernel.cl");
	err = clBuildProgram(kernel->program, 0, NULL,
					"-cl-unsafe-math-optimizations", NULL, NULL);
	tmp = (char*)ft_memalloc(4096);
	clGetProgramBuildInfo(kernel->program, opencl->gpus[0],
			CL_PROGRAM_BUILD_LOG, 4096, tmp, &tmp2);
	ft_printf("Build log :\n%.*s\n", tmp2, tmp);
	free(tmp);
	if (err != CL_SUCCESS)
		return (err);
	kernel->cores[0] = clCreateKernel(kernel->program, "draw_vbo", &err);
	if (err != CL_SUCCESS)
		return (err);
	kernel->cores[1] = clCreateKernel(kernel->program, "clear_buf", &err);
	return (err);
}

void			create_kernel(t_ptr *p)
{
	t_kernel	*kernel;
	t_img		*img;
	int			err;

	p->draw_vbo = NULL;
	if ((kernel = (t_kernel*)ft_memalloc(sizeof(t_kernel))) == NULL)
		ft_error("[Erreur] Echec d'allocation mémoire.");
	img = &p->win->img;
	kernel->opencl = p->opencl;
	if ((err = (build_program(p->opencl, kernel) ||
			(err = create_memobjs(p, p->opencl, kernel, img)))) != CL_SUCCESS)
	{
		ft_printf("[Error] Could not build kernel program"
				"%<R>  (Error code: %<i>%2d)%<0>\n", err);
		if (kernel->program != NULL)
			clReleaseProgram(kernel->program);
		free(kernel);
		return ;
	}
	clSetKernelArg(kernel->cores[0], 4, sizeof(unsigned int), &img->px_size);
	clSetKernelArg(kernel->cores[0], 5, sizeof(unsigned int), &img->line);
	clSetKernelArg(kernel->cores[0], 6, sizeof(int[2]), &img->size);
	clSetKernelArg(kernel->cores[0], 8,
			sizeof(int[1]), (int[1]){p->is_perspective_active});
	p->draw_vbo = kernel;
}
