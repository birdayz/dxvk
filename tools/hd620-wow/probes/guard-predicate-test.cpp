#include <cassert>
#include <cstdio>

#include "src/dxvk/dxvk_kbl_guard.h"

#define CHECK(condition) do { assert(condition); checks++; } while (0)

int main() {
  using namespace dxvk::kbl;
  unsigned checks = 0;
  CHECK(isWowHd620("Wow.exe", 0x8086, 0x5916));
  CHECK(!isWowHd620("Battle.net.exe", 0x8086, 0x5916));
  CHECK(!isWowHd620("WowClassic.exe", 0x8086, 0x5916));
  CHECK(!isWowHd620("Wow.exe", 0x10de, 0x5916));
  CHECK(!isWowHd620("Wow.exe", 0x8086, 0x5917));

  CHECK(isTypedStorageRead(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_ACCESS_SHADER_READ_BIT));
  CHECK(isTypedStorageRead(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT));
  CHECK(!isTypedStorageRead(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_ACCESS_SHADER_WRITE_BIT));
  CHECK(!isTypedStorageRead(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_ACCESS_SHADER_READ_BIT));
  CHECK(isTypedStorageRead(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_ACCESS_SHADER_READ_BIT));
  CHECK(!isTypedStorageRead(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, VK_ACCESS_SHADER_WRITE_BIT));
  CHECK(!isTypedStorageRead(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_ACCESS_SHADER_READ_BIT));

  constexpr auto image = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  constexpr auto buffer = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
  constexpr VkFormatFeatureFlags2 imageStorage = VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT;
  constexpr VkFormatFeatureFlags2 bufferStorage = VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT;
  constexpr VkFormatFeatureFlags2 read = VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT;
  constexpr VkFormatFeatureFlags2 write = VK_FORMAT_FEATURE_2_STORAGE_WRITE_WITHOUT_FORMAT_BIT;
  CHECK(supportsTypedStorageRead(image, VK_FORMAT_R16G16B16A16_SFLOAT, imageStorage | read));
  CHECK(supportsTypedStorageRead(image, VK_FORMAT_R16G16_SFLOAT, imageStorage | read));
  CHECK(!supportsTypedStorageRead(image, VK_FORMAT_R16G16B16A16_SFLOAT, imageStorage | write));
  CHECK(!supportsTypedStorageRead(image, VK_FORMAT_R8G8B8A8_UNORM, imageStorage | write));
  CHECK(!supportsTypedStorageRead(image, VK_FORMAT_B8G8R8A8_UNORM, imageStorage | write));
  CHECK(!supportsTypedStorageRead(image, VK_FORMAT_R8_UNORM, imageStorage));
  CHECK(supportsTypedStorageRead(image, VK_FORMAT_R32_SFLOAT, imageStorage));
  CHECK(supportsTypedStorageRead(image, VK_FORMAT_R32_UINT, imageStorage));
  CHECK(supportsTypedStorageRead(image, VK_FORMAT_R32_SINT, imageStorage));

  CHECK(supportsTypedStorageRead(buffer, VK_FORMAT_R16G16B16A16_SFLOAT, bufferStorage | read));
  CHECK(supportsTypedStorageRead(buffer, VK_FORMAT_R8G8B8A8_UNORM, bufferStorage | read));
  CHECK(!supportsTypedStorageRead(buffer, VK_FORMAT_R8G8B8A8_UNORM, bufferStorage | write));
  CHECK(!supportsTypedStorageRead(buffer, VK_FORMAT_R16G16B16A16_SFLOAT, imageStorage | read));
  CHECK(supportsTypedStorageRead(buffer, VK_FORMAT_R32_UINT, bufferStorage));

  UavAliasAccess aliases;
  aliases.merge(VK_ACCESS_SHADER_WRITE_BIT, 1u);
  CHECK(aliases.access == VK_ACCESS_SHADER_WRITE_BIT);
  CHECK(aliases.descriptorCount == 1u);
  aliases.merge(VK_ACCESS_SHADER_READ_BIT, 8u);
  CHECK(aliases.access == (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT));
  CHECK(aliases.descriptorCount == 8u);
  UavAliasAccess reversed;
  reversed.merge(VK_ACCESS_SHADER_READ_BIT, 8u);
  reversed.merge(VK_ACCESS_SHADER_WRITE_BIT, 1u);
  CHECK(reversed.access == aliases.access && reversed.descriptorCount == aliases.descriptorCount);
  UavAliasAccess writeOnly;
  writeOnly.merge(VK_ACCESS_SHADER_WRITE_BIT, 3u);
  writeOnly.merge(VK_ACCESS_SHADER_WRITE_BIT, 2u);
  CHECK(!(writeOnly.access & VK_ACCESS_SHADER_READ_BIT) && writeOnly.descriptorCount == 3u);
  std::printf("%u guard predicate checks passed\n", checks);
}
