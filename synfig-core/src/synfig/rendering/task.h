/* === S Y N F I G ========================================================= */
/*!	\file synfig/rendering/task.h
**	\brief Task Header
**
**	$Id$
**
**	\legal
**	......... ... 2015 Ivan Mahonin
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === S T A R T =========================================================== */

#ifndef __SYNFIG_RENDERING_TASK_H
#define __SYNFIG_RENDERING_TASK_H

/* === H E A D E R S ======================================================= */

#include "renderer.h"
#include "surface.h"
#include "transformation.h"
#include "blending.h"
#include "primitive.h"

/* === M A C R O S ========================================================= */

/* === T Y P E D E F S ===================================================== */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace synfig
{
namespace rendering
{

class Task: public etl::shared_object
{
public:
	typedef etl::handle<Task> Handle;
	typedef std::vector<const Renderer::DependentObject::Handle *> ParamList;

private:
	ParamList params;
	Surface::Handle target_surface;

protected:
	void register_param(const Renderer::DependentObject::Handle &param);
	void unregister_param(const Renderer::DependentObject::Handle &param);

public:
	//! List of sub-tasks and primitives, uses for optimization
	const ParamList& get_params() const { return params; }

	const Surface::Handle& get_target_surface() const { return target_surface; }

	virtual bool run(Renderer &renderer) const;
};

} /* end namespace rendering */
} /* end namespace synfig */

/* -- E N D ----------------------------------------------------------------- */

#endif