```c
void on_render()
{
    // 현재 출력 가능한 스왑체인 이미지의 인덱스를 가져오기 위한 변수를 선언합니다.
    uint32_t swapchain_index;

    // 현재 출력 가능한 스왑체인 이미지의 인덱스를 가져옵니다.
    vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &swapchain_index);

    auto& swapchain_image = swapchain_images_[swapchain_index];

    // 커맨드 버퍼를 재사용하기 위해서 커맨드 버퍼를 리셋합니다.
    vkResetCommandBuffer(command_buffer_, 0);

    // 커맨드 버퍼에 기록하는 커맨드들은 변하지 않기 때문에 다시 기록할 필요는 없습니다.
    // 하지만 일반적인 경우에 매 프레임마다 필요한 커맨드들이 다르기 때문에 리셋하고 다시 기록합니다.

    // 커맨드를 기록하기 위해 커맨드 버퍼를 기록중 상태로 전이할 때 필요한 정보를 정의합니다.
    VkCommandBufferBeginInfo begin_info {};

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // 커맨드 버퍼를 기록중 상태로 전이합니다.
    vkBeginCommandBuffer(command_buffer_, &begin_info);

    {
        // 이미지 배리어를 정의하기 위한 변수를 선언합니다.
        VkImageMemoryBarrier barrier {};

        // 이미지 배리어를 정의합니다. 이미지를 클리어하기 위해서는
        // 이미지 레이아웃이 반드시 아래 레이아웃 중에 한 레이아웃이어야 합니다.
        // - SHARED_PRESENT_KHR
        // - GENERAL
        // - TRANSFER_DST_OPTIMAL
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = queue_family_index_;
        barrier.dstQueueFamilyIndex = queue_family_index_;
        barrier.image = swapchain_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;

        // 이미지 레이아웃 변경을 위한 파이프라인 배리어 커맨드를 기록합니다.
        vkCmdPipelineBarrier(command_buffer_,
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);
    }

    // 클리어 색상을 정의하기 위한 변수를 선언합니다.
    VkClearColorValue clear_color;

    // 클리어 색상을 정의합니다.
    clear_color.float32[0] = 1.0f; // R
    clear_color.float32[1] = 0.0f; // G
    clear_color.float32[2] = 1.0f; // B
    clear_color.float32[3] = 1.0f; // A

    // 클리어할 이미지 영역을 정의하기 위한 변수를 선언합니다.
    VkImageSubresourceRange subresource_range {};

    // 클리어할 이미지 영역을 정의합니다.
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.levelCount = 1;
    subresource_range.layerCount = 1;

    // 이미지를 클리어하는 커맨드를 기록합니다.
    vkCmdClearColorImage(command_buffer_,
                        swapchain_image,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        &clear_color,
                        1,
                        &subresource_range);

    {
        // 이미지 배리어를 정의하기 위한 변수를 선언합니다.
        VkImageMemoryBarrier barrier {};

        // 이미지 배리어를 정의합니다. 이미지를 화면에 출력하기 위해서는
        // 이미지 레이아웃이 반드시 아래 레이아웃 중에 한 레이아웃이어야 합니다.
        // - PRESENT_SRC_KHR
        // - SHARED_PRESENT_KHR
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = 0;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex = queue_family_index_;
        barrier.dstQueueFamilyIndex = queue_family_index_;
        barrier.image = swapchain_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;

        // 이미지 레이아웃 변경을 위한 파이프라인 배리어 커맨드를 기록합니다.
        vkCmdPipelineBarrier(command_buffer_,
                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            0,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);
    }

    // 필요한 모든 커맨드들을 기록했기 때문에 커맨드 버퍼의 커맨드 기록을 끝마칩니다.
    // 커맨드 버퍼의 상태는 실행 가능 상태입니다.
    vkEndCommandBuffer(command_buffer_);

    // 큐에 제출할 커맨드 버퍼와 동기화를 정의하기 위한 변수를 선언합니다.
    VkSubmitInfo submit_info {};

    // 큐에 제출할 커맨드 버퍼를 정의합니다.
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;

    // 커맨드 버퍼를 큐에 제출합니다.
    vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);

    // 제출된 커맨드 버퍼들이 모두 처리될 때까지 기다립니다.
    vkDeviceWaitIdle(device_);

    // 화면에 출력되는 이미지를 정의하기 위한 변수를 선언합니다.
    VkPresentInfoKHR present_info {};

    // 화면에 출력되는 이미지를 정의합니다.
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain_;
    present_info.pImageIndices = &swapchain_index;

    // 화면에 이미지를 출력합니다.
    vkQueuePresentKHR(queue_, &present_info);
}
```