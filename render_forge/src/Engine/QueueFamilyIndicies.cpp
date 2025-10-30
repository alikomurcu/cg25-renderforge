#include "QueueFamilyIndicies.h"

bool QueueFamilyIndicies::is_complete() const {
	return graphics_and_compute_families.has_value() && present_family.has_value();
}