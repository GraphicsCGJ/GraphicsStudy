# SSAO
## prepare함수
### base prepare
진행했던 것이니 스킵
* 스왑체인 만들기
* 커맨드버퍼 만들기
* 뎁스스텐실 새팅
* 렌더패스 세팅
* 프레임버퍼 설정 등


### loadAssets
스폰자 gltf 파일 로드.
```vkglTF```클래스 내  에서 gltf 로드하는 ```loadFromFile``` 메소드 사용.

```vkglTF``` 클래스는 base/VulkanglTFModel.h 에 저장되어있다.

상세 내용은 스킵하고, 로드 시 vertices를 좌표계 flip을 한다거나 등의 작업을 할 수 있도록
 개발자가 만들어 놓은 것 같다.

### prepareOffscreenFramebuffers
오프스크린을 위한 준비단계 함수.

```cpp
struct FrameBuffer {
		int32_t width, height;
		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
		void setSize(int32_t w, int32_t h)
		{
			this->width = w;
			this->height = h;
		}
		void destroy(VkDevice device)
		{
			vkDestroyFramebuffer(device, frameBuffer, nullptr);
			vkDestroyRenderPass(device, renderPass, nullptr);
		}
	};

  struct FrameBufferAttachment {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
		void destroy(VkDevice device)
		{
			vkDestroyImage(device, image, nullptr);
			vkDestroyImageView(device, view, nullptr);
			vkFreeMemory(device, mem, nullptr);
		}
	};

struct {
		struct Offscreen : public FrameBuffer {
			FrameBufferAttachment position, normal, albedo, depth;
		} offscreen;
		struct SSAO : public FrameBuffer {
			FrameBufferAttachment color;
		} ssao, ssaoBlur;
	} frameBuffers;
```

오프스크린을 위한 프레임버퍼들의 설정은 위와 같이 선언되어있다.

오프스크린을 위한 버퍼 1개와 ssao 버퍼 1개, 옵션에 따라 키고 끌 수 있는 ssao blur
 가 포함되어있다.

이 세 프레임 버퍼에 대해 setSize로 크기 설정을 해준다.

```c++
frameBuffers.offscreen.setSize(width, height);
frameBuffers.ssao.setSize(ssaoWidth, ssaoHeight);
frameBuffers.ssaoBlur.setSize(width, height);
```

그 뒤, 적절한 depth format을 찾아주고, (```VkBool32``` 타입의 변수)

G-buffer(normal, depth) 에 대한 attachment와 SSAO에 해당하는 attachment를 만든다.

> 내가 기억이 안나서 본거
> [G-Buffer란?](https://agh2o.tistory.com/13)
> 우리 디퍼트 쉐이딩 할 때 사용하던 depth / normal 버퍼
> 색깔까지 가져와서 마지막에 합칠 때 사용 (최소 2번 렌더)
> 다만 반투명한 object에 대해선 한계가 있음. 뒤에있는 obj의 데이터 셋(color, depth, normal) 저장이 어려움.

```c++
createAttachment(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.offscreen.position, width, height);	// Position + Depth
createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.offscreen.normal, width, height);			// Normals
createAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.offscreen.albedo, width, height);			// Albedo (color)
createAttachment(attDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &frameBuffers.offscreen.depth, width, height);			// Depth

// SSAO
createAttachment(VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.ssao.color, ssaoWidth, ssaoHeight);				// Color

// SSAO blur
createAttachment(VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &frameBuffers.ssaoBlur.color, width, height);					// Color
```

여기서 ```createAttachment```는 뭘까?
 위에 보이는 ```Framebuffer``` 구조체의 ```FrameBufferAttachment``` 를 채워주는 과정이다.

그리고 Vk이미지(리소스)와 Vk이미지 뷰(리소스 접근자)를 만든다.
1. 입력받은 ```VkFormat``` 과 ```VkImageUsageFlagBits``` 에 맞추어 ```device```를 통해 이미지 info를 만들고 (디바이스는 전처리 과정에서 이미 만들어짐)
2. 이 이미지에 대한 memory requirement를 받아와, 이 사이즈를 가지고 실제 메모리를 할당하고 ```attachment->image```에 바인드한다.
3. 이제 리소스에 대한 접근자인 이미지뷰(```attachment->view```)를 만들고, 이어준다.

* frameBuffers.offscreen.position
  * 포맷: VK_FORMAT_R32G32B32A32_SFLOAT
  * usage: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
* frameBuffers.offscreen.normal
  * 포맷: VK_FORMAT_R8G8B8A8_UNORM (양수 노말벡터용. -1~+1 범위를 0~1로 캘리브레이션 하는 로직이 있다고 하는데, 이부분은 까묵.)
  * usage: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
* frameBuffers.offscreen.albedo
  * +) 알베도는 물체가 빛을 얼마나 반사하는 지에 대한 척도.
  * 포맷: VK_FORMAT_R8G8B8A8_UNORM (0~1 사이 값일 것이므로)
  * usage: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
* frameBuffers.ssao.color, blur (둘은 사실상 동일. 누굴 쓰느냐만 다름)
  * 포맷: VK_FORMAT_R8_UNORM (흑백 컬러니까)
  * usage: VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT

이미지는 만들었으니 이에 대한 어태치먼트를 만들어야 하고, 어태치먼트를 통해 서브페스/렌더패스를 정의해야 한다. 그 후 프레임버퍼가 생성이 된다.

#### G-BUFFER 설정
```c++
std::array<VkAttachmentDescription, 4> attachmentDescs = {};
// Init attachment properties
	for (uint32_t i = 0; i < static_cast<uint32_t>(attachmentDescs.size()); i++)
	{
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescs[i].finalLayout = (i == 3) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
```

처음은 위와같이 설정한다. 로드될때 clear 해주고, 디바이스 메모리 내에 저장되도록 ```VK_ATTACHMENT_STORE_OP_STORE``` 를 설정한다. 스텐실은 안쓰니 무시.

포맷은 아래와 같이 이미지 생성 시 사용했던 것을 그대로 쓴다.

```c++
attachmentDescs[0].format = frameBuffers.offscreen.posiation.format;
attachmentDescs[1].format = frameBuffers.offscreen.norml.format;
attachmentDescs[2].format = frameBuffers.offscreen.albedo.format;
attachmentDescs[3].format = frameBuffers.offscreen.depth.format;
```

또한 컬러 사용에 필요한 attachment reference와 depth에 대한 reference를 설정하고
```c++
std::vector<VkAttachmentReference> colorReferences;
colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

VkAttachmentReference depthReference = {};
depthReference.attachment = 3;
depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
```
( 위에서 포맷과 상관없이 usage를 모두 ```VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT``` 로 설정했으므로 이렇게 만든다. )

이를 서브패스에 달아준다.

```c++
VkSubpassDescription subpass = {};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpass.pColorAttachments = colorReferences.data();
subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
subpass.pDepthStencilAttachment = &depthReference;
```

이제 디펜던시를 만들어 준다. 디펜던시는 VkSubpassDependency 로,

```c++
// Use subpass dependencies for attachment layout transitions
std::array<VkSubpassDependency, 2> dependencies;

dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
dependencies[0].dstSubpass = 0;
dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

dependencies[1].srcSubpass = 0;
dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
```

와 같이 설정한다.

```VK_SUBPASS_EXTERNAL``` 는 서브패스에 대한 특수 인덱스로 시작/끝을 동시에 나타낸다.

따라서 서브패스 시작 시에 프래그먼트쉐이더에서 어태치먼트들의 데이터를 서브패스 마지막에 사용하도록 디펜던시를 설정한 것이라 볼 수 있다.

따라서 서브패스 1개에 디펜던시 2개로 렌더패스를 만들었고, 이 렌더패스는 G Buffer의 렌더패스가 된다.

```c++
VkRenderPassCreateInfo renderPassInfo = {};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.pAttachments = attachmentDescs.data();
renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;
renderPassInfo.dependencyCount = 2;
renderPassInfo.pDependencies = dependencies.data();
VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &frameBuffers.offscreen.renderPass));
```

이제 이 렌더패스를 통해 프레임버퍼를 만들면 끝이다.

```c++
std::array<VkImageView, 4> attachments;
attachments[0] = frameBuffers.offscreen.position.view;
attachments[1] = frameBuffers.offscreen.normal.view;
attachments[2] = frameBuffers.offscreen.albedo.view;
attachments[3] = frameBuffers.offscreen.depth.view;

VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
fbufCreateInfo.renderPass = frameBuffers.offscreen.renderPass;
fbufCreateInfo.pAttachments = attachments.data();
fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
fbufCreateInfo.width = frameBuffers.offscreen.width;
fbufCreateInfo.height = frameBuffers.offscreen.height;
fbufCreateInfo.layers = 1;
VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBuffers.offscreen.frameBuffer));
```

어태치먼트를 통해 뷰를 연결해주고 이 어태치먼트에 대한 포인터는 프레임버퍼 생성 시 달린다.
앞에 만든 렌더패스도 여기서 달리게 되며, 버퍼의 width, height는 이 때 설정된다.

#### SSAO 설정
blur랑 그냥이랑 2개가 있다. 근데 다를게 없으니 SSAO 1개만 보도록 한다.

어태치먼트에 대한 상세정보를 설정한다.
```c++
VkAttachmentDescription attachmentDescription{};
attachmentDescription.format = frameBuffers.ssao.color.format;
attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
```

멀티샘플링 없고, 로드시 clear, 디바이스 메모리에 저장, 스텐실 없음, 레이아웃 설정 등이 들어간다. 컬러 래퍼런스도 동일하다.

```c++
VkSubpassDescription subpass = {};
subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
subpass.pColorAttachments = &colorReference;
subpass.colorAttachmentCount = 1;
```
이후 subpass에 대한 상세정보도 작성해준다. (뎁스는 없어서 설정 안하는 듯)


서브패스에 대한 디펜던시도 아까와 마찬가지로 2개 만들어준다.
```c++
std::array<VkSubpassDependency, 2> dependencies;

dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
dependencies[0].dstSubpass = 0;
dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

dependencies[1].srcSubpass = 0;
dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
```
아까와는 다르게, src/dst에 ```VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT``` 가 있는데 이건 파이프라인의 마지막 단계를 의미한다. src에 있으면 현재 서브패스 이전 단계에서 어떤 작업도 하지 않음을 의미한다. dst에 있으면 그냥 파이프라인 마지막 단계를 의미한다.

또한 서브패스 종료(dst)시점에 메모리를 읽을 수 있게 ```VK_ACCESS_MEMORY_READ_BIT``` 로 변경이 되어있다. 이 서브패스를 지나게 될 때 frag에서 데이터를 읽고 써서 수정을 하고, 읽어갈 수 있도록 변경한다는 의미가 된다.

이후 렌더패스를 만들고 프레임버퍼를 만드는건 동일하다.
```c++
VkRenderPassCreateInfo renderPassInfo = {};
renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
renderPassInfo.pAttachments = &attachmentDescription;
renderPassInfo.attachmentCount = 1;
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;
renderPassInfo.dependencyCount = 2;
renderPassInfo.pDependencies = dependencies.data();
VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &frameBuffers.ssao.renderPass));

VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
fbufCreateInfo.renderPass = frameBuffers.ssao.renderPass;
fbufCreateInfo.pAttachments = &frameBuffers.ssao.color.view;
fbufCreateInfo.attachmentCount = 1;
fbufCreateInfo.width = frameBuffers.ssao.width;
fbufCreateInfo.height = frameBuffers.ssao.height;
fbufCreateInfo.layers = 1;
VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &frameBuffers.ssao.frameBuffer));
```

마지막으로 color attachment 들에 대해 샘플링할 때 사용할 샘플러를 만들어준다.
```c++
VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
sampler.magFilter = VK_FILTER_NEAREST;
sampler.minFilter = VK_FILTER_NEAREST;
sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
sampler.addressModeV = sampler.addressModeU;
sampler.addressModeW = sampler.addressModeU;
sampler.mipLodBias = 0.0f;
sampler.maxAnisotropy = 1.0f;
sampler.minLod = 0.0f;
sampler.maxLod = 1.0f;
sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &colorSampler));
```

### prepareUniformBuffers
Uniform 버퍼를 만드는 곳이다. GUI를 통해 조작되는 파라미터들을 넘겨주거나 카메라 정보가 될 것이다.

1. Scene 매트릭스관련
```c++
struct UBOSceneParams {
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		float nearPlane = 0.1f;
		float farPlane = 64.0f;
} uboSceneParams;

// Scene matrices
vulkanDevice->createBuffer(
VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
&uniformBuffers.sceneParams,
sizeof(uboSceneParams));
```
매트릭스와 같은 크기에 해당하는 버퍼를 생성한다.

+) ```createBuffer``` 함수는 ```vkCreateBuffer``` 와 ```vkAllocateMemory``` 를 사용하여 실제 메모리를 gpu에 할당하는 함수인데,
 usage, property까지 받아 생성할 수 있도록 함수화를 잘 해놨다. 버퍼는 할당할 공간에 대한 메타데이터, 실제 데이터는 ```vkAllocateMemoryh```를 수행할 때 할당된다.

2. SSAO 파라미터 관련 메모리 할당.
```c++
struct UBOSSAOParams {
   glm::mat4 projection;
   int32_t ssao = true;
   int32_t ssaoOnly = false;
   int32_t ssaoBlur = true;
} uboSSAOParams;

// SSAO parameters
vulkanDevice->createBuffer(
   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
   &uniformBuffers.ssaoParams,
   sizeof(uboSSAOParams));
```

3. 초기값 매핑
```c++
// Update
updateUniformBufferMatrices();
updateUniformBufferSSAOParams();
```
내부에서 특별히 하는건 없고, base에 만든 해더인 vks::Buffer를 통해서
* map()
  * vkMapMemory(device, memory, offset, size, 0, &mapped);
* copyTo()
  * memcpy
* unmap()
  * vkUnmapMemory(device, memory);

을 수행한다. 당연히 ```render()``` 에서도 호출된다.

4. SSAO Kernel
```c++
// SSAO
std::default_random_engine rndEngine(benchmark.active ? 0 : (unsigned)time(nullptr));
std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

// Sample kernel
std::vector<glm::vec4> ssaoKernel(SSAO_KERNEL_SIZE);
for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i)
{
glm::vec3 sample(rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine) * 2.0 - 1.0, rndDist(rndEngine));
sample = glm::normalize(sample);
sample *= rndDist(rndEngine);
float scale = float(i) / float(SSAO_KERNEL_SIZE);
scale = lerp(0.1f, 1.0f, scale * scale);
ssaoKernel[i] = glm::vec4(sample * scale, 0.0f);
}

// Upload as UBO
vulkanDevice->createBuffer(
VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
&uniformBuffers.ssaoKernel,
ssaoKernel.size() * sizeof(glm::vec4),
ssaoKernel.data());
```
SSAO를 너무 오랜만에 봐서 정확하진 않지만, 랜덤한 결과를 0/64 ~ 63/64까지 샘플링 해서 ao의 정도를 조절하려고 사용하는 것 같다.
이 데이터도 유니폼하게 저장해준다.


5. SSAO Noise
```c++
// Random noise
std::vector<glm::vec4> ssaoNoise(SSAO_NOISE_DIM * SSAO_NOISE_DIM);
for (uint32_t i = 0; i < static_cast<uint32_t>(ssaoNoise.size()); i++)
{
ssaoNoise[i] = glm::vec4(rndDist(rndEngine) * 2.0f - 1.0f, rndDist(rndEngine) * 2.0f - 1.0f, 0.0f, 0.0f);
}
// Upload as texture
textures.ssaoNoise.fromBuffer(ssaoNoise.data(), ssaoNoise.size() * sizeof(glm::vec4), VK_FORMAT_R32G32B32A32_SFLOAT, SSAO_NOISE_DIM, SSAO_NOISE_DIM, vulkanDevice, queue, VK_FILTER_NEAREST);
```
SSAO를 자연스럽게 하기위한 노이즈값을 텍스쳐로 저장해준다. 마찬가지로 랜덤하게 생성하며,
```fromBuffer``` 라는 메소드로 저장되는데, 이것도 버퍼만들고 메모리 할당하고 메모리 매핑하고 이미지 카피하는 일련의 과정이 들어간다.


### setupDescriptorPool
```c++
std::vector<VkDescriptorPoolSize> poolSizes = {
	vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10),
	vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 12)
};
VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes,  descriptorSets.count);
VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
```
적절한 크기의 디스크립터 풀을 생성한다.


### setupLayoutsAndDescriptors
descriptorSetLayout을 바인딩하는 곳이다.

```c++
struct {
		VkDescriptorSetLayout gBuffer;
		VkDescriptorSetLayout ssao;
		VkDescriptorSetLayout ssaoBlur;
		VkDescriptorSetLayout composition;
	} descriptorSetLayouts;
```
만들어야 하는 디스크립터셋 레이아웃은 위 4개이다.
```c++
std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo;
VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo();
VkDescriptorSetAllocateInfo descriptorAllocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, nullptr, 1);
std::vector<VkWriteDescriptorSet> writeDescriptorSets;
std::vector<VkDescriptorImageInfo> imageDescriptors;
```
위 데이터를 재사용해서 계속해서 디스크립터셋을 allocate할 것이다.


#### G-Buffer
렌더링 과정 중 크게 다른 정보가 필요 없다.

```c++
setLayoutBindings ={
   vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),	// VS + FS Parameter UBO
};
setLayoutCreateInfo = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));
VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &setLayoutCreateInfo, nullptr, &descriptorSetLayouts.gBuffer));

const std::vector<VkDescriptorSetLayout> setLayouts = { descriptorSetLayouts.gBuffer, vkglTF::descriptorSetLayoutImage };
pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
pipelineLayoutCreateInfo.setLayoutCount = 2;
VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.gBuffer));
descriptorAllocInfo.pSetLayouts = &descriptorSetLayouts.gBuffer;
VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorAllocInfo, &descriptorSets.floor));
writeDescriptorSets = {
vks::initializers::writeDescriptorSet(descriptorSets.floor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.sceneParams.descriptor),
};
vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
pipelineLayoutCreateInfo.setLayoutCount = 1;
```
디스크립터 셋 레이아웃 상세정보를 만들고,

