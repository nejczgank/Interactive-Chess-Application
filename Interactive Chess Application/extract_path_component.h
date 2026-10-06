#pragma once
#include "trace_path_component.h"
#include "extract_ray_type_info.h"

struct ExtractPathComponent
{
	TracePathComponent path_data{};
	ExtractRayTypeInfo::ray_type ray_type;
};