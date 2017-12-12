/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2016 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 * Copyright (C) 2014 Amlogic, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * The full GNU General Public License is included in this distribution
 * in the file called COPYING.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2016 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 * Copyright (C) 2014 Amlogic, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/clk/clk-conf.h>
#include <linux/pm_runtime.h>

#include <linux/mali/mali_utgard.h>

#define MESON_MALI_MAX_PP	3

struct clk *clk_core;

static const struct of_device_id meson_mali_matches[] = {
	{ .compatible = "amlogic,meson-gxbb-mali" },
	{ .compatible = "amlogic,meson-gxl-mali" },
	{},
};
MODULE_DEVICE_TABLE(of, meson_mali_matches);

struct resource *mali450_create_mp3_resources(unsigned long address,
					int irq_gp, int irq_gpmmu,
					int irq_pp_bcast, int irq_pmu,
					int *irq_pp, int *irq_ppmmu,
					int *len)
{
	struct resource target[] = {
		MALI_GPU_RESOURCES_MALI450_MP3_PMU(address, irq_gp, irq_gpmmu,
						   irq_pp[0], irq_ppmmu[0],
						   irq_pp[1], irq_ppmmu[1],
						   irq_pp[2], irq_ppmmu[2],
						   irq_pp_bcast)
	};
	struct resource *res;

	res = kzalloc(sizeof(target), GFP_KERNEL);
	if (!res)
		return NULL;

	memcpy(res, target, sizeof(target));

	*len = ARRAY_SIZE(target);

	return res;
}

static struct mali_gpu_device_data mali_gpu_data = {
	.fb_start = 0x0,
	.fb_size = 0xFFFFF000,
	.shared_mem_size = 256 * 1024 * 1024,
	.control_interval = 200, /* 1000ms */
	.pmu_domain_config = {
		0x1, 0x2, 0x4, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x2, 0x0
	},
	.pmu_switch_delay = 0xFFFF,
};

int mali_platform_device_register(void)
{
	struct platform_device *dev;
	struct resource *mali_res;
	struct device_node *np;
	struct resource res;
	struct clk *core;
	int irq_gp, irq_gpmmu, irq_pp_bcast, irq_pmu;
	int irq_pp[MESON_MALI_MAX_PP], irq_ppmmu[MESON_MALI_MAX_PP];
	int ret, len;

	np = of_find_matching_node(NULL, meson_mali_matches);
	if (!np) {
		pr_err("Couldn't find the mali node\n");
		return -ENODEV;
	}

	dev = platform_device_alloc("mali-utgard", 0);
	if (!dev) {
		pr_err("Couldn't create platform device\n");
		ret = -EINVAL;
		goto err_put_node;
	}

	dev_set_name(&dev->dev, "mali-utgard");
	dev->dev.of_node = np;
	dev->dev.dma_mask = &dev->dev.coherent_dma_mask;
	dev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	dev->dev.bus = &platform_bus_type;

	core = of_clk_get_by_name(np, "core");
	if (IS_ERR(core)) {
		pr_err("Couldn't retrieve our module clock\n");
		ret = PTR_ERR(core);
		goto err_pdev;
	}

	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		pr_err("Couldn't retrieve our base address\n");
		goto err_put_clk;
	}

	irq_gp = of_irq_get_byname(np, "gp");
	if (irq_gp < 0) {
		pr_err("Couldn't get 'gp' interrupt\n");
		goto err_put_clk;
	}

	irq_gpmmu = of_irq_get_byname(np, "gpmmu");
	if (irq_gpmmu < 0) {
		pr_err("Couldn't get 'gpmmu' interrupt\n");
		goto err_put_clk;
	}

	irq_pp_bcast = of_irq_get_byname(np, "pp");
	if (irq_pp_bcast < 0) {
		pr_err("Couldn't get 'pp' interrupt\n");
		goto err_put_clk;
	}

	irq_pmu = of_irq_get_byname(np, "pmu");
	if (irq_pmu < 0) {
		pr_err("Couldn't get 'pmu' interrupt\n");
		goto err_put_clk;
	}

	irq_pp[0] = of_irq_get_byname(np, "pp0");
	if (irq_pp[0] < 0) {
		pr_err("Couldn't get 'pp0' interrupt\n");
		goto err_put_clk;
	}

	irq_ppmmu[0] = of_irq_get_byname(np, "ppmmu0");
	if (irq_ppmmu[0] < 0) {
		pr_err("Couldn't get 'ppmmu0' interrupt\n");
		goto err_put_clk;
	}

	irq_pp[1] = of_irq_get_byname(np, "pp1");
	if (irq_pp[1] < 0) {
		pr_err("Couldn't get 'pp1' interrupt\n");
		goto err_put_clk;
	}

	irq_ppmmu[1] = of_irq_get_byname(np, "ppmmu1");
	if (irq_ppmmu[1] < 0) {
		pr_err("Couldn't get 'ppmmu1' interrupt\n");
		goto err_put_clk;
	}

	irq_pp[2] = of_irq_get_byname(np, "pp2");
	if (irq_pp[2] < 0) {
		pr_err("Couldn't get 'pp2' interrupt\n");
		goto err_put_clk;
	}

	irq_ppmmu[2] = of_irq_get_byname(np, "ppmmu2");
	if (irq_ppmmu[2] < 0) {
		pr_err("Couldn't get 'ppmmu2' interrupt\n");
		goto err_put_clk;
	}

	mali_res = mali450_create_mp3_resources(res.start,
						irq_gp, irq_gpmmu,
						irq_pp_bcast, irq_pmu,
						irq_pp, irq_ppmmu,
						&len);
	if (!mali_res) {
		pr_err("Couldn't create target resources\n");
		ret = -EINVAL;
		goto err_put_clk;
	}

	ret = platform_device_add_resources(dev, mali_res, len);
	if (ret) {
		pr_err("Couldn't add our resources\n");
		goto err_free_resources;
	}

	ret = platform_device_add_data(dev, &mali_gpu_data, sizeof(mali_gpu_data));
	if (ret) {
		pr_err("Couldn't add platform data\n");
		goto err_free_resources;
	}

	of_dma_configure(&dev->dev, np);

	of_clk_set_defaults(np, false);

	clk_prepare_enable(core);

	pm_runtime_enable(&dev->dev);

	ret = platform_device_add(dev);
	if (ret) {
		pr_err("Couldn't add our device\n");
		goto err_unprepare_clk;
	}

	pr_info("Amlogic Mali glue initialized\n");

	of_node_put(np);

	clk_core = core;

	return 0;

err_unprepare_clk:
	clk_disable_unprepare(core);
err_free_resources:
	kfree(mali_res);
err_put_clk:
	clk_put(core);
err_pdev:
	platform_device_put(dev);
err_put_node:
	of_node_put(np);

	return ret;
}

int mali_platform_device_unregister(void)
{
	clk_put(clk_core);
	clk_core = NULL;

	return 0;
}
