#include "frg_game_object.hpp"

namespace frg {
std::vector<VkDescriptorImageInfo> FrgGameObject::get_descriptors() {
    return model->get_descriptors();
}
} // namespace frg