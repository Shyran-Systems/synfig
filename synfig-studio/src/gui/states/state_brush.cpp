/* === S Y N F I G ========================================================= */
/*!	\file state_brush.cpp
**	\brief Template File
**
**	$Id$
**
**	\legal
**	......... ... 2014 Ivan Mahonin
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

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>
#include <glibmm/timeval.h>

#include <synfig/layer_switch.h>

#include "state_brush.h"
#include "state_normal.h"
#include "canvasview.h"
#include "workarea.h"
#include "app.h"
#include <ETL/hermite>
#include <ETL/calculus>
#include <utility>
#include "event_mouse.h"
#include "event_layerclick.h"
#include "docks/dock_toolbox.h"

#include <synfigapp/blineconvert.h>
#include <synfigapp/wplistconverter.h>
#include <synfigapp/main.h>
#include <synfigapp/actions/layerpaint.h>

#include <ETL/gaussian>
#include "docks/dialog_tooloptions.h"

#include "ducktransform_matrix.h"

#include "general.h"

#endif

/* === U S I N G =========================================================== */

using namespace std;
using namespace etl;
using namespace synfig;
using namespace studio;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

StateBrush studio::state_brush;

/* === C L A S S E S & S T R U C T S ======================================= */

class studio::StateBrush_Context : public sigc::trackable
{
	etl::handle<CanvasView> canvas_view_;
	CanvasView::IsWorking is_working;

	WorkArea::PushState push_state;

	bool prev_table_status;

	Gtk::Menu menu;

	Glib::TimeVal time;
	etl::handle<synfigapp::Action::LayerPaint> action;
	TransformStack transform_stack;

	void refresh_ducks();

	synfigapp::Settings& settings;

public:
	void load_settings();
	void save_settings();

	Smach::event_result event_stop_handler(const Smach::event& x);
	Smach::event_result event_refresh_handler(const Smach::event& x);
	Smach::event_result event_mouse_down_handler(const Smach::event& x);
	Smach::event_result event_mouse_up_handler(const Smach::event& x);
	Smach::event_result event_mouse_draw_handler(const Smach::event& x);
	Smach::event_result event_refresh_tool_options(const Smach::event& x);

	static bool build_transform_stack(
		Canvas::Handle canvas,
		Layer::Handle layer,
		CanvasView::Handle canvas_view,
		TransformStack& transform_stack );

	void refresh_tool_options();

	StateBrush_Context(CanvasView* canvas_view);
	~StateBrush_Context();

	const etl::handle<CanvasView>& get_canvas_view()const{return canvas_view_;}
	etl::handle<synfigapp::CanvasInterface> get_canvas_interface()const{return canvas_view_->canvas_interface();}
	synfig::Time get_time()const { return get_canvas_interface()->get_time(); }
	synfig::Canvas::Handle get_canvas()const{return canvas_view_->get_canvas();}
	WorkArea * get_work_area()const{return canvas_view_->get_work_area();}
};	// END of class StateBrush_Context


/* === M E T H O D S ======================================================= */

StateBrush::StateBrush():
	Smach::state<StateBrush_Context>("brush")
{
	insert(event_def(EVENT_STOP,&StateBrush_Context::event_stop_handler));
	insert(event_def(EVENT_REFRESH,&StateBrush_Context::event_refresh_handler));
	insert(event_def(EVENT_WORKAREA_MOUSE_BUTTON_DOWN,&StateBrush_Context::event_mouse_down_handler));
	insert(event_def(EVENT_WORKAREA_MOUSE_BUTTON_UP,&StateBrush_Context::event_mouse_up_handler));
	insert(event_def(EVENT_WORKAREA_MOUSE_BUTTON_DRAG,&StateBrush_Context::event_mouse_draw_handler));
	insert(event_def(EVENT_REFRESH_TOOL_OPTIONS,&StateBrush_Context::event_refresh_tool_options));
}

StateBrush::~StateBrush()
{
}


void
StateBrush_Context::load_settings()
{
	try
	{
		synfig::ChangeLocale change_locale(LC_NUMERIC, "C");
		String value;

		// TODO: load something
	}
	catch(...)
	{
		synfig::warning("State Brush: Caught exception when attempting to load settings.");
	}
}

void
StateBrush_Context::save_settings()
{
	try
	{
		synfig::ChangeLocale change_locale(LC_NUMERIC, "C");

		// TODO: save something
	}
	catch(...)
	{
		synfig::warning("State Brush: Caught exception when attempting to save settings.");
	}
}

StateBrush_Context::StateBrush_Context(CanvasView* canvas_view):
	canvas_view_(canvas_view),
	is_working(*canvas_view),
	push_state(get_work_area()),
	settings(synfigapp::Main::get_selected_input_device()->settings())
{
	load_settings();

	refresh_tool_options();
	App::dialog_tool_options->present();

	// Hide all tangent and width ducks
	get_work_area()->set_type_mask(get_work_area()->get_type_mask()-Duck::TYPE_TANGENT-Duck::TYPE_WIDTH);
	get_canvas_view()->toggle_duck_mask(Duck::TYPE_NONE);

	// Turn off layer clicking
	get_work_area()->set_allow_layer_clicks(false);

	// Turn off duck clicking
	get_work_area()->set_allow_duck_clicks(false);

	// Hide the tables if they are showing
	prev_table_status=get_canvas_view()->tables_are_visible();

	get_work_area()->set_cursor(Gdk::PENCIL);

	App::dock_toolbox->refresh();
	refresh_ducks();
}

void
StateBrush_Context::refresh_tool_options()
{
	App::dialog_tool_options->clear();
	// TODO: add some widget
	App::dialog_tool_options->set_local_name(_("Brush Tool"));
	App::dialog_tool_options->set_name("brush");
}

Smach::event_result
StateBrush_Context::event_refresh_tool_options(const Smach::event& /*x*/)
{
	refresh_tool_options();
	return Smach::RESULT_ACCEPT;
}

StateBrush_Context::~StateBrush_Context()
{
	save_settings();

	App::dialog_tool_options->clear();

	get_work_area()->reset_cursor();

	// Bring back the tables if they were out before
	if(prev_table_status)get_canvas_view()->show_tables();

	// Refresh the work area
	get_work_area()->queue_draw();

	App::dock_toolbox->refresh();
}

Smach::event_result
StateBrush_Context::event_stop_handler(const Smach::event& /*x*/)
{
	if (action)
	{
		get_canvas_interface()->get_instance()->perform_action(action);
		action = NULL;
	}

	throw &state_normal;
	return Smach::RESULT_OK;
}

Smach::event_result
StateBrush_Context::event_refresh_handler(const Smach::event& /*x*/)
{
	refresh_ducks();
	return Smach::RESULT_ACCEPT;
}

bool
StateBrush_Context::build_transform_stack(
	Canvas::Handle canvas,
	Layer::Handle layer,
	CanvasView::Handle canvas_view,
	TransformStack& transform_stack )
{
	int count = 0;
	for(Canvas::iterator i = canvas->begin(); i != canvas->end() ;++i)
	{
		if(*i == layer) return true;

		if((*i)->active())
		{
			Transform::Handle trans((*i)->get_transform());
			if(trans) { transform_stack.push(trans); count++; }
		}

		// If this is a paste canvas layer, then we need to
		// descend into it
		if(etl::handle<Layer_PasteCanvas> layer_pastecanvas = etl::handle<Layer_PasteCanvas>::cast_dynamic(*i))
		{
			transform_stack.push_back(
				new Transform_Matrix(
						layer_pastecanvas->get_guid(),
					layer_pastecanvas->get_summary_transformation().get_matrix()
				)
			);
			if (build_transform_stack(layer_pastecanvas->get_sub_canvas(), layer, canvas_view, transform_stack))
				return true;
			transform_stack.pop();
		}
	}
	while(count-- > 0) transform_stack.pop();
	return false;
}


Smach::event_result
StateBrush_Context::event_mouse_down_handler(const Smach::event& x)
{
	const EventMouse& event(*reinterpret_cast<const EventMouse*>(&x));
	switch(event.button)
	{
	case BUTTON_LEFT:
		{
			// Enter the stroke state to get the stroke
			Layer::Handle selected_layer = canvas_view_->get_selection_manager()->get_selected_layer();
			etl::handle<Layer_Bitmap> layer = etl::handle<Layer_Bitmap>::cast_dynamic(selected_layer);
			if (!layer)
			{
				etl::handle<Layer_Switch> layer_switch = etl::handle<Layer_Switch>::cast_dynamic(selected_layer);
				if (layer_switch) layer = etl::handle<Layer_Bitmap>::cast_dynamic(layer_switch->get_current_layer());
			}

			if (layer)
			{
				transform_stack.clear();
				if (build_transform_stack(get_canvas(), layer, get_canvas_view(), transform_stack))
				{
					action = new synfigapp::Action::LayerPaint();
					action->set_param("canvas",get_canvas());
					action->set_param("canvas_interface",get_canvas_interface());

					action->stroke.set_layer(layer);

					// configure brush
					brushlib::Brush &brush = action->stroke.brush();
					brush.set_base_value(BRUSH_OPAQUE, 					 0.85f );
					brush.set_mapping_n(BRUSH_OPAQUE_MULTIPLY, INPUT_PRESSURE, 4);
					brush.set_mapping_point(BRUSH_OPAQUE_MULTIPLY, INPUT_PRESSURE, 0, 0.f, 0.f);
					brush.set_mapping_point(BRUSH_OPAQUE_MULTIPLY, INPUT_PRESSURE, 1, 0.222222f, 0.f);
					brush.set_mapping_point(BRUSH_OPAQUE_MULTIPLY, INPUT_PRESSURE, 2, 0.324074f, 1.f);
					brush.set_mapping_point(BRUSH_OPAQUE_MULTIPLY, INPUT_PRESSURE, 2, 1.f, 1.f);
					brush.set_base_value(BRUSH_OPAQUE_LINEARIZE,			 0.9f  );
					brush.set_base_value(BRUSH_RADIUS_LOGARITHMIC,			 2.6f  );
					brush.set_base_value(BRUSH_HARDNESS,					 0.69f );
					brush.set_base_value(BRUSH_DABS_PER_ACTUAL_RADIUS,		 6.0f  );
					brush.set_base_value(BRUSH_DABS_PER_SECOND, 			54.45f );
					brush.set_base_value(BRUSH_SPEED1_SLOWNESS, 			 0.04f );
					brush.set_base_value(BRUSH_SPEED2_SLOWNESS, 			 0.08f );
					brush.set_base_value(BRUSH_SPEED1_GAMMA, 				 4.0f  );
					brush.set_base_value(BRUSH_SPEED2_GAMMA, 				 4.0f  );
					brush.set_base_value(BRUSH_OFFSET_BY_SPEED_SLOWNESS,	 1.0f  );
					brush.set_base_value(BRUSH_SMUDGE,						 0.9f  );
					brush.set_base_value(BRUSH_SMUDGE_LENGTH,				 0.12f );
					brush.set_base_value(BRUSH_STROKE_DURATION_LOGARITHMIC,  4.0f  );

					action->stroke.prepare();

					time.assign_current_time();
					return Smach::RESULT_ACCEPT;
				}
			}
			break;
		}

	default:
		break;
	}
	return Smach::RESULT_OK;
}

Smach::event_result
StateBrush_Context::event_mouse_up_handler(const Smach::event& x)
{
	const EventMouse& event(*reinterpret_cast<const EventMouse*>(&x));
	switch(event.button)
	{
	case BUTTON_LEFT:
		{
			if (action)
			{
				get_canvas_interface()->get_instance()->perform_action(action);
				action = NULL;
				transform_stack.clear();
				return Smach::RESULT_ACCEPT;
			}
			break;
		}

	default:
		break;
	}

	return Smach::RESULT_OK;
}

Smach::event_result
StateBrush_Context::event_mouse_draw_handler(const Smach::event& x)
{
	const EventMouse& event(*reinterpret_cast<const EventMouse*>(&x));
	switch(event.button)
	{
	case BUTTON_LEFT:
		{
			if (action)
			{
				Glib::TimeVal prev_time = time;
				time.assign_current_time();
				double delta_time = (time - prev_time).as_double();

				Point p = transform_stack.unperform( event.pos );
				Point tl = action->stroke.get_layer()->get_param("tl").get(Point());
				Point br = action->stroke.get_layer()->get_param("br").get(Point());
				int w = action->stroke.get_layer()->surface.get_w();
				int h = action->stroke.get_layer()->surface.get_h();

				action->stroke.add_point_and_apply(
					synfigapp::Action::LayerPaint::PaintPoint(
						(float)((p[0] - tl[0])/(br[0] - tl[0])*w),
						(float)((p[1] - tl[1])/(br[1] - tl[1])*h),
						(float)event.pressure,
						delta_time ));

				return Smach::RESULT_ACCEPT;
			}
			break;
		}

	default:
		break;
	}

	return Smach::RESULT_OK;
}

void
StateBrush_Context::refresh_ducks()
{
	get_canvas_view()->queue_rebuild_ducks();
}
