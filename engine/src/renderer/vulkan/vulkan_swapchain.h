#pragma once

#include "vulkan_types.inl"



void vulkan_swapchain_create(
    vulkan_context* context,
    u32 width,
    u32 height,
    vulkan_swapchain* swapchain);