#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace frg {
class FrgSsbo {
  public:
    FrgSsbo() = default;

  private:
    VkBuffer buffer;
    VkDeviceMemory memory;
};
} // namespace frg
