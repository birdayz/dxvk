#pragma once

#include <cstdint>
#include <string_view>

#include <vulkan/vulkan.h>

namespace dxvk::kbl {

  struct UavAliasAccess {
    VkAccessFlags access = 0u;
    uint32_t descriptorCount = 0u;

    void merge(VkAccessFlags declaredAccess, uint32_t declaredCount) {
      access |= declaredAccess;
      if (descriptorCount < declaredCount)
        descriptorCount = declaredCount;
    }
  };

  inline bool isWowHd620(std::string_view executable, uint32_t vendor, uint32_t device) {
    return executable == "Wow.exe" && vendor == 0x8086u && device == 0x5916u;
  }

  inline bool isTypedStorageRead(VkDescriptorType type, VkAccessFlags access) {
    return (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
        && (access & VK_ACCESS_SHADER_READ_BIT);
  }

  inline bool isBasicR32(VkFormat format) {
    return format == VK_FORMAT_R32_SFLOAT
        || format == VK_FORMAT_R32_UINT
        || format == VK_FORMAT_R32_SINT;
  }

  inline bool supportsTypedStorageRead(VkDescriptorType type, VkFormat format, VkFormatFeatureFlags2 realFeatures) {
    VkFormatFeatureFlags2 storage = type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
      ? VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT : VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT;
    // Use image-tiling or buffer-specific features supplied by the caller.
    // DXVK can explicitly declare the basic R32 formats in SPIR-V.
    return (realFeatures & storage) && (isBasicR32(format)
        || (realFeatures & VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT));
  }

}
