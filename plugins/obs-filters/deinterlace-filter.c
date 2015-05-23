#include <obs-module.h>
#include <graphics/matrix4.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>
#include <util/dstr.h>

struct deinterlace_filter_data {
	obs_source_t                   *context;

	gs_effect_t                    *effect;

	gs_eparam_t                    *previous_image_param;
	gs_eparam_t                    *field_order_param;
	gs_eparam_t                    *dimensions_param;

	const char                     *deinterlacer;
	struct dstr                    matrix;
	gs_texture_t                   *previous_image;
	bool                           field_order;
};

static const char *deinterlace_name(void)
{
	return obs_module_text("Deinterlacing");
}

static void deinterlace_update(void *data, obs_data_t *settings)
{
	struct deinterlace_filter_data *filter = data;

	filter->deinterlacer = obs_data_get_string(settings, "deinterlacer");
	filter->field_order = obs_data_get_bool(settings, "field_order") ?
		0 : 1;
	dstr_printf(&filter->matrix, "%sMatrix", filter->deinterlacer);
}

static void deinterlace_destroy(void *data)
{
	struct deinterlace_filter_data *filter = data;

	dstr_free(&filter->matrix);
	if (filter->previous_image != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	if (filter->effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		obs_leave_graphics();
	}

	bfree(data);
}

static void *deinterlace_create(obs_data_t *settings, obs_source_t *context)
{
	struct deinterlace_filter_data *filter =
		bzalloc(sizeof(struct deinterlace_filter_data));

	char *effect_path = obs_module_file("deinterlace_filter.effect");

	filter->context = context;

	obs_enter_graphics();

	filter->effect = gs_effect_create_from_file(effect_path, NULL);

	if (filter->effect != NULL) {
		filter->previous_image_param = gs_effect_get_param_by_name(
				filter->effect, "previous_image");
		filter->field_order_param = gs_effect_get_param_by_name(
				filter->effect, "field_order");
		filter->dimensions_param = gs_effect_get_param_by_name(
				filter->effect, "dimensions");
	}

	obs_leave_graphics();

	bfree(effect_path);

	dstr_init(&filter->matrix);

	if (!filter->effect) {
		deinterlace_destroy(filter);
		return NULL;
	}

	deinterlace_update(filter, settings);
	return filter;
}

static void deinterlace_render(void *data, gs_effect_t *effect)
{
	struct deinterlace_filter_data *filter = data;

	obs_source_process_filter_begin(filter->context, GS_RGBA,
			OBS_ALLOW_DIRECT_RENDERING);

	float width = (float)obs_source_get_width(
			obs_filter_get_target(filter->context));
	float height = (float)obs_source_get_height(
			obs_filter_get_target(filter->context));

	struct vec2 dimensions = { 0 };
	vec2_set(&dimensions, width, height);

	gs_texture_t *texture = obs_filter_get_texture(filter->context);
	
	gs_effect_set_texture(filter->previous_image_param,  texture);
	gs_effect_set_int(filter->field_order_param,
			filter->field_order ? 0 : 1);
	gs_effect_set_vec2(filter->dimensions_param, &dimensions);

	gs_texture_t *current_texture = gs_texture_create(
			gs_texture_get_width(texture),
			gs_texture_get_height(texture),
			gs_texture_get_color_format(texture),
			1, NULL, GS_DYNAMIC);

	gs_copy_texture(current_texture, texture);

	obs_source_process_filter_tech_end(filter->context, filter->effect,
			0, 0, filter->deinterlacer, filter->matrix.array);

	if (filter->previous_image)
		gs_texture_destroy(filter->previous_image);
	filter->previous_image = current_texture;

	UNUSED_PARAMETER(effect);
}

#define DEINT_BLEND "Blend"
#define DEINT_DISCARD "Discard"
#define DEINT_LINEAR "Linear"
#define DEINT_YADIF_MODE0 "YadifMode0"
#define DEINT_YADIF_MODE2 "YadifMode2"
#define DEINT_YADIF_DISCARD "YadifDiscard"
#define DEINTER(x) "Draw" x
#define TEXT(x) obs_module_text("Deinterlace." x)

static obs_properties_t *deinterlace_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	obs_property_t *prop = obs_properties_add_list(props, "deinterlacer",
			TEXT("DeinterlaceMethod"), OBS_COMBO_TYPE_LIST,
			OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(prop, TEXT(DEINT_BLEND),
			DEINTER(DEINT_BLEND));
	obs_property_list_add_string(prop, TEXT(DEINT_DISCARD),
			DEINTER(DEINT_DISCARD));
	obs_property_list_add_string(prop, TEXT(DEINT_LINEAR),
			DEINTER(DEINT_LINEAR));
	obs_property_list_add_string(prop, TEXT(DEINT_YADIF_MODE0),
			DEINTER(DEINT_YADIF_MODE0));
	obs_property_list_add_string(prop, TEXT(DEINT_YADIF_MODE2),
			DEINTER(DEINT_YADIF_MODE2));
	obs_property_list_add_string(prop, TEXT(DEINT_YADIF_DISCARD),
			DEINTER(DEINT_YADIF_DISCARD));

	obs_properties_add_bool(props, "field_order", 
			TEXT("FieldOrder"));

	UNUSED_PARAMETER(data);
	return props;
}

static void deinterlace_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "deinterlacer",
			DEINTER(DEINT_YADIF_MODE0));
	obs_data_set_default_bool(settings, "field_order", false);
}

struct obs_source_info deinterlace_filter = {
	.id                            = "deinterlace_filter",
	.type                          = OBS_SOURCE_TYPE_FILTER,
	.output_flags                  = OBS_SOURCE_VIDEO,
	.get_name                      = deinterlace_name,
	.create                        = deinterlace_create,
	.destroy                       = deinterlace_destroy,
	.video_render                  = deinterlace_render,
	.update                        = deinterlace_update,
	.get_properties                = deinterlace_properties,
	.get_defaults                  = deinterlace_defaults
};
