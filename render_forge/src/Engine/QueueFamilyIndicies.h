#pragma once
#include <optional>

struct QueueFamilyIndicies {
    std::optional<uint32_t> graphics_and_compute_families;
    std::optional<uint32_t> present_family;

    bool is_complete() const;
};