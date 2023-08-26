# Vulkan Shadow Mapping

이전 `initVulkan` 부터 `VulkanExampleBase::prepare()` 부분은 동일하다. 때문에 이전 내용을 참고하도록 한다.
 - [initVulkan](https://github.com/yunmin1130/graphics_modern_cpp/blob/main/VulkanCode/1_triangle/triangle.md#1-initvulkan)
 - [VulkanExsampleBase::prepare()](https://github.com/yunmin1130/graphics_modern_cpp/blob/main/VulkanCode/1_triangle/triangle.md#3-prepare)

이번 그림자 매핑에서는 off screen 렌더를 이용한 하나의 예제로 off screen 에서 필요한 준비 과정부터 렌더 순서까지 살펴보도록 한다.

## Shadow mapping prepare

``` c++
void prepare() {
	VulkanExampleBase::prepare();
	loadAssets();
	prepareOffscreenFramebuffer();
	prepareUniformBuffers();
	setupDescriptorSetLayout();
	preparePipelines();
	setupDescriptorPool();
	setupDescriptorSets();
	buildCommandBuffers();
	prepared = true;
}
```

위 코드는 shadow mapping 에 필요한 다른 요소들을 준비하는 과정을 나타낸 코드이다. 위 코드의 순서대로 설명을 진행하려고 한다. 이에 앞서 간략하게 해당 코드가 하는 동작을 설명하고 넘어가도록 하자.

1. `loadAssets` shadow mapping 에서 사용하는 모델은 이전과는 다르게 복잡한 모델들을 사용한다. 거기에 더해서 position, uv map, color, normal vector 정보를 제공하기 때문에 하드 코딩으로 작성하기 보다는 파일을 읽고 로드하는 방식을 사용한다. 이 함수는 그와 같은 동작을 수행한다.
2. `prepareOffscreenFrameBuffer` 이 함수는 shadow depth map 을 위한 render pass, frame buffer 를 생성하는 동작을 수행한다.
3. `prepareUniformBuffers` 이 함수는 shadow map 생성과 그 이후에 quad mesh 에 그림자 효과를 렌더하는 데 필요한 uniform buffer 들을 생성하는 동작을 수행한다.
4. `setupDescriptorSetLayout` 이 함수는 off screen 렌더에 필요한 descriptor set layout 을 생성하는 동작을 수행한다.
5. `preparePipelines` 모든 렌더에 필요한 pipeline 을 생성한다.
6. `setupDescriptorPool` 필요한 descriptor pool 생성
7. `setupDescriptorSets` descriptor set 생성
8. `buildCommandBuffers` render 함수에서 필요한 command buffer 가 정해진 루트를 따라가기에 미리 command buffer 를 생성해둔다.

### loadAssets

`loadAsset` 중 `loadFromFile` 함수가 하는 역할에 대해 살펴본다. FILE 로 부터 메모리에 로드하는 과정은 우선 생략
```c++
size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
indices.count = static_cast<uint32_t>(indexBuffer.size());
vertices.count = static_cast<uint32_t>(vertexBuffer.size());

assert((vertexBufferSize > 0) && (indexBufferSize > 0));

struct StagingBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
} vertexStaging, indexStaging;

// Create staging buffers
// Vertex data
VK_CHECK_RESULT(device->createBuffer(
  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
  vertexBufferSize,
  &vertexStaging.buffer,
  &vertexStaging.memory,
  vertexBuffer.data()));
// Index data
VK_CHECK_RESULT(device->createBuffer(
  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
  indexBufferSize,
  &indexStaging.buffer,
  &indexStaging.memory,
  indexBuffer.data()));

// Create device local buffers
// Vertex buffer
VK_CHECK_RESULT(device->createBuffer(
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memoryPropertyFlags,
  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  vertexBufferSize,
  &vertices.buffer,
  &vertices.memory));
// Index buffer
VK_CHECK_RESULT(device->createBuffer(
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | memoryPropertyFlags,
  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  indexBufferSize,
  &indices.buffer,
  &indices.memory));

// Copy from staging buffers
VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

VkBufferCopy copyRegion = {};

copyRegion.size = vertexBufferSize;
vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertices.buffer, 1, &copyRegion);

copyRegion.size = indexBufferSize;
vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indices.buffer, 1, &copyRegion);

device->flushCommandBuffer(copyCmd, transferQueue, true);

vkDestroyBuffer(device->logicalDevice, vertexStaging.buffer, nullptr);
vkFreeMemory(device->logicalDevice, vertexStaging.memory, nullptr);
vkDestroyBuffer(device->logicalDevice, indexStaging.buffer, nullptr);
vkFreeMemory(device->logicalDevice, indexStaging.memory, nullptr);
```
우선 파일 로드 한 이후 vertex buffer 와 index buffer 에 메모리를 올리도록 한다.
1. local bit 로 vertex buffer, index buffer 생성
2. staging buffer 를 생성하여 메모리 매핑, 메모리 복사
3. staging buffer 에서 vertex, index 버퍼로 메모리 복사 (커맨드 버퍼 사용)
4. 모든 커맨드가 flush 되면 staging 버퍼 해제

```c++
// Setup descriptors
uint32_t uboCount{ 0 };
uint32_t imageCount{ 0 };
for (auto node : linearNodes) {
  if (node->mesh) {
    uboCount++;
  }
}
// mesh 별로 ubo 카운트
for (auto material : materials) {
  if (material.baseColorTexture != nullptr) {
    imageCount++;
  }
}
// material 별로 image sampler 카운트
std::vector<VkDescriptorPoolSize> poolSizes = {
  { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uboCount },
};
if (imageCount > 0) {
  if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
    poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount });
  }
  if (descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
    poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount });
  }
}
// image color, normal map 사용시 combined image sampler 를 pool 에 추가
VkDescriptorPoolCreateInfo descriptorPoolCI{};
descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
descriptorPoolCI.pPoolSizes = poolSizes.data();
descriptorPoolCI.maxSets = uboCount + imageCount;
VK_CHECK_RESULT(vkCreateDescriptorPool(device->logicalDevice, &descriptorPoolCI, nullptr, &descriptorPool));

// Descriptors for per-node uniform buffers
{
  // Layout is global, so only create if it hasn't already been created before
  if (descriptorSetLayoutUbo == VK_NULL_HANDLE) {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
    };
    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
    descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    descriptorLayoutCI.pBindings = setLayoutBindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayoutCI, nullptr, &descriptorSetLayoutUbo));
  }
  for (auto node : nodes) {
	// descriptor set allocate and update descriptor sets
    //prepareNodeDescriptor(node, descriptorSetLayoutUbo);
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = descriptorPool;
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &node->mesh->uniformBuffer.descriptorSet));

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

		vkUpdateDescriptorSets(device->logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
	}
  }
}

// Descriptors for per-material images
{
  // Layout is global, so only create if it hasn't already been created before
  if (descriptorSetLayoutImage == VK_NULL_HANDLE) {
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
    if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
      setLayoutBindings.push_back(vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(setLayoutBindings.size())));
    }
    if (descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
      setLayoutBindings.push_back(vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(setLayoutBindings.size())));
    }
    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
    descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    descriptorLayoutCI.pBindings = setLayoutBindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->logicalDevice, &descriptorLayoutCI, nullptr, &descriptorSetLayoutImage));
  }
  for (auto& material : materials) {
    if (material.baseColorTexture != nullptr) {
      //material.createDescriptorSet(descriptorPool, vkglTF::descriptorSetLayoutImage, descriptorBindingFlags);
	  {
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = descriptorPool;
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device->logicalDevice, &descriptorSetAllocInfo, &descriptorSet));
		std::vector<VkDescriptorImageInfo> imageDescriptors{};
		std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
		if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
			imageDescriptors.push_back(baseColorTexture->descriptor);
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.dstSet = descriptorSet;
			writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
			writeDescriptorSet.pImageInfo = &baseColorTexture->descriptor;
			writeDescriptorSets.push_back(writeDescriptorSet);
		}
		if (normalTexture && descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
			imageDescriptors.push_back(normalTexture->descriptor);
			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.dstSet = descriptorSet;
			writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
			writeDescriptorSet.pImageInfo = &normalTexture->descriptor;
			writeDescriptorSets.push_back(writeDescriptorSet);
		}
		vkUpdateDescriptorSets(device->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
      }
	}
  }
}
```

위의 과정들은 모두 파일 로드 이후 mesh, material 에서 필요한 ubo, combined_image_sampler 들을 설정하고 있다.
1. descriptor pool 생성
2. descriptor layout 생성
3. descriptor pool 로 부터 필요한 descriptor set 할당
4. ubo 든 combined image sampler 든 할당받은 descriptor set 에 필요한 정보들 update (uniform buffer 사용하기 위한 준비)

### prepareOffscreenFramebuffer
다음으로는 shadow map 생성에 사용될 frame buffer 를 만듣도록 한다.
```c++
// Setup the offscreen framebuffer for rendering the scene from light's point-of-view to
// The depth attachment of this framebuffer will then be used to sample from in the fragment shader of the shadowing pass
#define DEPTH_FORMAT VK_FORMAT_D16_UNORM
void prepareOffscreenFramebuffer()
{
	offscreenPass.width = SHADOWMAP_DIM;
	offscreenPass.height = SHADOWMAP_DIM;

	// For shadow mapping we only need a depth attachment
	VkImageCreateInfo image = vks::initializers::imageCreateInfo();
	image.imageType = VK_IMAGE_TYPE_2D;
	image.extent.width = offscreenPass.width;
	image.extent.height = offscreenPass.height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.format = DEPTH_FORMAT;        // Depth stencil attachment
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;	// We will sample directly from the depth attachment for the shadow mapping
	VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &offscreenPass.depth.image));

	VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device, offscreenPass.depth.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &offscreenPass.depth.mem));
	VK_CHECK_RESULT(vkBindImageMemory(device, offscreenPass.depth.image, offscreenPass.depth.mem, 0));
```
우선 shadow depth map 에서 depth 정보가 기록될 depth image 를 만든다.
- shadow map 은 2d texture 로 생성하며 화면에 보여질 texture 는 아니기에 적당히 width, height 를 1:1 비율로 잡는다
- 또한 mipmap 이나 다른레이어는 사용하지 않고, device 내부에서만 사용하기에 tiling 또한 `VK_IMAGE_TILING_OPTIMAL` 로 설정한다.
- format 도 `DEPTH_FORMAT` 으로 설정하고 dpeth stencil attachment 와 sampler 용도로 사용할 예정이기에 usage 비트는 `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT` 로 설정한다.

그 이후에는 해당 image 에 필요한 메모리를 파악하여 할당 이후 이미지에 할당받은 메모리를 바인딩 시킨다.

```c++
	VkImageViewCreateInfo depthStencilView = vks::initializers::imageViewCreateInfo();
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = DEPTH_FORMAT;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = offscreenPass.depth.image;
	VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &offscreenPass.depth.view));
```
다음으로는 이미지 뷰를 생성한다. 뎁스 정보만 입력할 목적이라 aspectMask 값은 `VK_IMAGE_ASPECT_DEPTH_BIT` 로 설정한다. 나머지 설정은 특별한 부분이 없다.
```c++
	// Create sampler to sample from to depth attachment
	// Used to sample in the fragment shader for shadowed rendering
	VkFilter shadowmap_filter = vks::tools::formatIsFilterable(physicalDevice, DEPTH_FORMAT, VK_IMAGE_TILING_OPTIMAL) ?
	   DEFAULT_SHADOWMAP_FILTER : // is defined to VK_FILTER_LINEAR
	   VK_FILTER_NEAREST;
	VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
	sampler.magFilter = shadowmap_filter;
	sampler.minFilter = shadowmap_filter;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 1.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &offscreenPass.depthSampler));
```
이미지 뷰 까지 생성했으면 shadow rendering 시 fragment shader 에서 사용할 수 있도록 image sampler 도 만들어둔다.
- 사용하고자 하는 depth format 이 `VK_FILTER_LINEAR` 지원이 된다면 linaer 하게, 없다면 nearest 로 min, mag filter 를 설정한다.
- 나머지 필드들은 적당히 설정하도록 한다.

이후에는 off screen 에서 사용할 render pass 를 준비한다.

```c++
	prepareOffscreenRenderpass();
// Set up a separate render pass for the offscreen frame buffer
// This is necessary as the offscreen frame buffer attachments use formats different to those from the example render pass
void prepareOffscreenRenderpass()
{
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = DEPTH_FORMAT;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end
```
그림자 맵 생성시 color attachment 는 필요하지 않기 때문에 depth stencil attachment 만 준비한다.
- dpeth attachment load 시 clear 한다.
- depth attachment store 시 depth 정보를 store 한다.
- stencil attachment 는 생성도, 사용도 안하기 때문에 상관하지 않는다.
- depth attchment 는 device 내부에서만 사용할 목적이기 때문에 초기 initial layout 을 설정하지 않아도 된다.
- 그리고 depth attachment 사용 이후에는 fragment shader 에서만 읽을 예정이기에 final layout 은 `VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL` 로 설정한다.


```c++
	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;													// No color attachments
	subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment
```
Depth attachment 에 필요한 attachment reference 를 생성하고, 필요한 subpass description 을 생성한다. 이때 color attachment 는 사용하지 않기에 `colorAttachmentCount` 값은 0이다.

```c++
	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
```

Subpass dependency 를 정의한다. Depth attachment 같은 경우에는 외부 렌더패스와 의존성이 존재하기 때문에 두 개의 dependency 설정이 필요하다. 먼저 외부 렌더 패스에서 depth attachment read 이후에 depth attachment write 를 해야하고, 다음으로는 depth attachment write 이후에 외부 렌더패스에서 depth attachment read 작업을 해야한다. 때문에 두 가지의 dependency 가 필요한 것이다.
- 첫 번째 dependency 는 외부 렌더 패스에서의 fragment shader 에서 shader read 작업 이후 내부 렌더패스안에 eary fragment test 에서의 depth write 가 가능하도록 의존성을 정의하고 있다.
- 두 번째 dependency 는 내부 렌더패스안에 late fragment test 에서의 depth stencil write 이후 외부 렌더패스안에 fragment shader 에서 shader read 작업이 이루어지도록 의존성을 정의하고 있다.

```c++
	VkRenderPassCreateInfo renderPassCreateInfo = vks::initializers::renderPassCreateInfo();
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassCreateInfo.pDependencies = dependencies.data();

	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &offscreenPass.renderPass));
}
```

렌더 패스 생성에 필요한 attachment 설정, 의존성 정의가 완료 되었다면 render pass 를 만들도록 한다.
```c++
	// Create frame buffer
	VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
	fbufCreateInfo.renderPass = offscreenPass.renderPass;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = &offscreenPass.depth.view;
	fbufCreateInfo.width = offscreenPass.width;
	fbufCreateInfo.height = offscreenPass.height;
	fbufCreateInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offscreenPass.frameBuffer));
}
```

렌더패스까지 만들었다면 이제 off screen frame buffer 생성에 필요한 설정이 모두 되었으니 frame buffer 를 만들도록 한다.

### prepareUniformBuffers

```c++
struct {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model;
	glm::mat4 depthBiasMVP;
	glm::vec4 lightPos;
	// Used for depth map visualization
	float zNear;
	float zFar;
} uboVSscene;

struct {
	glm::mat4 depthMVP;
} uboOffscreenVS;

// Prepare and initialize uniform buffer containing shader uniforms
void prepareUniformBuffers()
{
	// Offscreen vertex shader uniform buffer block
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&uniformBuffers.offscreen,
		sizeof(uboOffscreenVS)));

	// Scene vertex shader uniform buffer block
	VK_CHECK_RESULT(vulkanDevice->createBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&uniformBuffers.scene,
		sizeof(uboVSscene)));

	// Map persistent
	VK_CHECK_RESULT(uniformBuffers.offscreen.map());
	VK_CHECK_RESULT(uniformBuffers.scene.map());

	updateLight();
	updateUniformBufferOffscreen();
	updateUniformBuffers();
}
```

Uniform buffer 생성은 이전의 uniform buffer 생성의 과정과 별반 다르지 않다. `vulkanDevice->createBuffer` 안의 내용은 다음과 같다.
1. 매개변수로 전달받은 `VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT`, struct 메모리 크기를 적용하여 목적에 맞는 버퍼를 생성
2. 해당 버퍼가 필요로 하는 memory 크기파악
3. `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` 를 만족하는 memory type index 탐색
4. 해당 memory type index 와 필요한 메모리 크기를 옵션으로 설정하여 메모리 할당
5. 이후 해당 버퍼를 bind 하여 정상적으로 바인딩 되는지 그 결과값을 반환한다.

이런 일련의 과정들을 통해 uniform buffer 를 생성한 이후 memory mapping 을 하여 이후 uniform buffer 관련 메모리들을 copy 할 수 있도록 준비해 둔다. 이후에 있는 함수들은 초기 light 위치, 그리고 matrix 정보들등 uniform buffer 에 복사할 값들을 설정하는 함수들이다.

### setupDescriptorSetLayout

```c++
void setupDescriptorSetLayout()
{
	// Shared pipeline layout for all pipelines used in this sample
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
		// Binding 0 : Vertex shader uniform buffer
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		// Binding 1 : Fragment shader image sampler (shadow map)
		vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
	};
	VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}
```
모든 pipeline 들에서 사용할 descriptor set layout 을 준비한다. 사용할 descriptor set layout 들은 두가지 인데, 하나는 v-shader와 f-shader 에서 사용할 uniform buffer 용이고, 하나는 f-shader 에서 사용할 image-sampler 용이다. 이러한 descriptor set layout 을 만들었다면 만들어진 descriptor set layout 을 이용해 pipeline layout 을 생성한다.

### preparePipelines

Pipeline 을 만들기에 앞서 여기서는 총 3개의 pipeline 을 만들것이다.
- Shadow map debug 용 quad mesh 렌더링 파이프라인
- Shadow 가 적용된 scene 렌더링 파이프라인
- Shadow depth map 을 생성할 렌더링 파이프라인


```c++
void preparePipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationStateCI = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendStateCI = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportStateCI = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleStateCI = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCI = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size(), 0);
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
	pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
	pipelineCI.pRasterizationState = &rasterizationStateCI;
	pipelineCI.pColorBlendState = &colorBlendStateCI;
	pipelineCI.pMultisampleState = &multisampleStateCI;
	pipelineCI.pViewportState = &viewportStateCI;
	pipelineCI.pDepthStencilState = &depthStencilStateCI;
	pipelineCI.pDynamicState = &dynamicStateCI;
	pipelineCI.stageCount = shaderStages.size();
	pipelineCI.pStages = shaderStages.data();
```

먼저 모든 파이프라인에서 필요한 state 설정들을 생성한다. 이중에는 공통적인 state 도 있을 것이며, 다르게 설정되어야 할 state 들도 있을 것이다.

```c++

	// Shadow mapping debug quad display
	rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
	shaderStages[0] = loadShader(getShadersPath() + "shadowmapping/quad.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(getShadersPath() + "shadowmapping/quad.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	// Empty vertex input state
	VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
	pipelineCI.pVertexInputState = &emptyInputState;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.debug));
```

먼저 만든 것은 shadow map debug 용 quad mesh 렌더링 파이프라인이다. 미리 설정한 state 들에서 크게 변경할 것 없이 파이프 라인을 생성한다. 이때 vertex buffer 를 사용하지 않고 quad 메시 렌더링을 할 것이기 때문에 vertex input state 는 비어있는 상태로 설정한다.

```c++

	// Scene rendering with shadows applied
	pipelineCI.pVertexInputState  = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal});
	rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
	shaderStages[0] = loadShader(getShadersPath() + "shadowmapping/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(getShadersPath() + "shadowmapping/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	// Use specialization constants to select between horizontal and vertical blur
	uint32_t enablePCF = 0;
	VkSpecializationMapEntry specializationMapEntry = vks::initializers::specializationMapEntry(0, 0, sizeof(uint32_t));
	VkSpecializationInfo specializationInfo = vks::initializers::specializationInfo(1, &specializationMapEntry, sizeof(uint32_t), &enablePCF);
	shaderStages[1].pSpecializationInfo = &specializationInfo;
	// No filtering
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.sceneShadow));
	// PCF filtering
	enablePCF = 1;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.sceneShadowPCF));
```

다음으로는 shadow 렌더링에 필요한 파이프 라인이다. 여기서 사용하는 vertex 들은 position, uv, color, normal 값을 사용하여 해당 값들을 사용하도록 input state 를 설정한다. 여기서 참고할 것은 `specializationInfo` 이다. 여기서 해당 변수는 `shaderStage[1]` 에서 `const` 변수를 shader 파일을 수정하지 않고 동적으로 수정하여 사용할 수 있도록 한다. 때문에 PCF 기능을 동적으로 on/off 하여 사용할 수 있도록 옵션을 부여해 두 개의 파이프 라인을 만든것을 알 수 있다.

```c++

	// Offscreen pipeline (vertex shader only)
	shaderStages[0] = loadShader(getShadersPath() + "shadowmapping/offscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	pipelineCI.stageCount = 1;
	// No blend attachment states (no color attachments used)
	colorBlendStateCI.attachmentCount = 0;
	// Disable culling, so all faces contribute to shadows
	rasterizationStateCI.cullMode = VK_CULL_MODE_NONE;
	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	// Enable depth bias
	rasterizationStateCI.depthBiasEnable = VK_TRUE;
	// Add depth bias to dynamic state, so we can change it at runtime
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
	dynamicStateCI =
		vks::initializers::pipelineDynamicStateCreateInfo(
			dynamicStateEnables.data(),
			dynamicStateEnables.size(),
			0);

	pipelineCI.renderPass = offscreenPass.renderPass;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.offscreen));
}
```

마지막으로 shadow depth map 생성용 렌더링 파이프라인이다. 여기서는 color attachment가 없고 depth attachment 만을 사용하기에 vertex shader 만 설정했다. 또한 shadow mapping 시 종종 사용하는 depth bias 를 사용하도록 설정한다. 이전에 debug 용 파이프라인과 shadow 렌더링용 파이프라인은 `VulkanExampleBase::prepare` 에서 생성한 renderpass 를 설정했다면 여기서는 미리 생성해둔 offscreen 용 renderPass 를 설정하도록 하여 파이프라인을 생성한다.

### setupDescriptorPool

```c++
void setupDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes = {
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3),
		vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3)
	};
	VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, 3);
	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
}
```

Uniform buffer 와 combined image smapler 용 descriptor pool 을 만든다.

### setupDescriptorSets

앞서 만든 3 개의 pipeline 에 필요한 descriptor set 들을 만들도록 한다.
```c++
void setupDescriptorSets()
{
	std::vector<VkWriteDescriptorSet> writeDescriptorSets;

	// Image descriptor for the shadow map attachment
	VkDescriptorImageInfo shadowMapDescriptor =
	    vks::initializers::descriptorImageInfo(
	        offscreenPass.depthSampler,
	        offscreenPass.depth.view,
	        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

	// Debug display
	VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.debug));
	writeDescriptorSets = {
		// Binding 0 : Parameters uniform buffer
		vks::initializers::writeDescriptorSet(descriptorSets.debug, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.scene.descriptor),
		// Binding 1 : Fragment shader texture sampler
	    vks::initializers::writeDescriptorSet(descriptorSets.debug, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &shadowMapDescriptor)
	};
	vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
```

먼저 debug 용 렌더링에서 사용할 descriptor 들을 만든다. 여기서 사용할 것은 v-shader 에서 사용할 uniform buffer 와 f-shader 에서 접근 할 shadow 용 combined_image_sampler 다. 이를 위해 descriptor pool 과 descriptor set layout 을 이용해 필요한 descriptor set 을 할당받고 필요한 descriptor set 정보들을 사용하여 할당받은 descriptor set 을 업데이트 하도록 한다.
    
```c++
	// Offscreen shadow map generation
	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.offscreen));
	writeDescriptorSets = {
		// Binding 0 : Vertex shader uniform buffer
		vks::initializers::writeDescriptorSet(descriptorSets.offscreen, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.offscreen.descriptor),
	};
	vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
```

Shadow map 용 렌더링에서는 combined_image_samper 가 필요하지 않기에 v-shader 에서 사용할 descriptor set 만 할당 받아, 원하는 용도의 descriptor set 으로 업데이트 한다.

```c++
	// Scene rendering with shadow map applied
	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets.scene));
	writeDescriptorSets = {
		// Binding 0 : Vertex shader uniform buffer
		vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.scene.descriptor),
		// Binding 1 : Fragment shader shadow sampler
	    vks::initializers::writeDescriptorSet(descriptorSets.scene, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &shadowMapDescriptor)
	};
	vkUpdateDescriptorSets(device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}
```

Shadow 렌더링에서는 앞선 debug 에서 만든 descriptor set 과 동일한 설정을 해두도록 한다.

### buildCommandBuffers

```c++
void buildCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	VkViewport viewport;
	VkRect2D scissor;

	for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
	{
		VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

		/*
			First render pass: Generate shadow map by rendering the scene from light's POV
		*/
		{
			clearValues[0].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
			renderPassBeginInfo.renderPass = offscreenPass.renderPass;
			renderPassBeginInfo.framebuffer = offscreenPass.frameBuffer;
			renderPassBeginInfo.renderArea.extent.width = offscreenPass.width;
			renderPassBeginInfo.renderArea.extent.height = offscreenPass.height;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			viewport = vks::initializers::viewport((float)offscreenPass.width, (float)offscreenPass.height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			scissor = vks::initializers::rect2D(offscreenPass.width, offscreenPass.height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artifacts
			vkCmdSetDepthBias(
				drawCmdBuffers[i],
				depthBiasConstant,
				0.0f,
				depthBiasSlope);

			vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
			vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.offscreen, 0, nullptr);
			scenes[sceneIndex].draw(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);
		}
```

draw cmd buffer 개수에 따라 각각 command buffer 를 미리 세팅하도록 한다.
먼저 해줘야 할 작업은 offscreen 렌더링 command 세팅이다.
Depth attachment clear value 를 설정하여 render pass 를 시작한다.
이후에 view port 세팅, scissor 세팅을 한 이후 depth bias 세팅을 하도록 한다. 여기서 사용하는 `depthBiasConstant` 는 depth bias 값, `depthBaisSlope` 는 primitive 의 기준 기울기이다. 만약 primitive 가 camera 의 z-vector 와 평행하다면 depth bias 를 적용할 이유가 없기 때문에 기준값이 필요하다.
그리고 pipeline 을 바인딩하고 descriptor set 까지 바인딩 한 이후 draw 명령을 내려주고 렌더패스를 끝내는 명령을 넣어준다.

```c++
		/*
			Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
		*/

		/*
			Second pass: Scene rendering with applied shadow map
		*/

		{
			clearValues[0].color = defaultClearColor;
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = frameBuffers[i];
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
			vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

			scissor = vks::initializers::rect2D(width, height, 0, 0);
			vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

			// Visualize shadow map
			if (displayShadowMap) {
				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.debug, 0, nullptr);
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.debug);
				vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);
			} else {
				// Render the shadows scene
				vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.scene, 0, nullptr);
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (filterPCF) ? pipelines.sceneShadowPCF : pipelines.sceneShadow);
				// 앞서 로드한 asset 정보들을 이용
				// vertex buffer bind, index buffer bind
				// 그리고 node, material 에 필요한 ubo, combined image sampler 용 descriptor set 도 바인딩
				scenes[sceneIndex].draw(drawCmdBuffers[i]);
			}
			drawUI(drawCmdBuffers[i]);

			vkCmdEndRenderPass(drawCmdBuffers[i]);
		}
```

Shadow map 생성용 command 빌드 이후에 shadow map 을 보여주는 debug 렌더링을 할지, shadow 렌더링 결과를 보여주는 씬 렌더링을 할 지에 따라 command 를 다르게 가져간다.
먼저 공통적으로 color attachment, depth attachment clear 를 하기 때문에 렌더패스 시작할 때 clear value 설정을 해준다.
그리고 차례로 viewport, scissor 세팅을 하도록 command 버퍼에 넣어준다.
이후에 debug 모드라면 debug 용 descriptor set, pipeline 을 바인딩하여 그리도록 한다.
만약 아니라면 PCF 사용 여부에 따라 해당 pipeline 을 바인딩하고 descriptor set 도 바인딩하여 scene 을 그리도록 한다.
마지막으로 end render pass commad 를 제출하도록 한다.

```c++

		VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
	}
}
```

마지막으로 command buffer 를 닫도록 한다.

## Shadow map rendering

```c++
virtual void render()
{
if (!prepared)
	return;
draw();
if (!paused || camera.updated)
{
	updateLight();
	updateUniformBufferOffscreen();
	updateUniformBuffers();
}


void draw()
{
	VulkanExampleBase::prepareFrame();

	// Command buffer to be submitted to the queue
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

	// Submit to queue
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

	VulkanExampleBase::submitFrame();
}
```

준비가 끝났다면 렌더링시에는 특별한 점이 없다. 미리 command buffer 를 세팅 해두었기 때문에 매 프레임마다 light, uniform buffer 업데이트를 해주는 작업, swapchain 이 준비되어 한 frame 을 그릴수 있다면 미리 세팅한 command buffer 를 queue 에 제출하여 그리도록 해주는 작업만 해주면 된다.