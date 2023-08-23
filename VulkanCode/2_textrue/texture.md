# Vulkan Texture


## 1. InitVulkan
Triangle 과 동일
#### 1.1 Vulkan Instance
- VkApplicationInfo 구조체 생성 및 초기화
- 필요한 Extension 및 Layer 셋팅
- 설정한 값으로 VkInstanceCreateInfo 구조체 생성 및 초기화 
- vkCreateInstance 로 인스턴스 생성

#### 1.2 Physical Device 
- vkEnumeratePhysicalDevices 로 GPU 정보 가져와 사용할 GPU 설정
- vkGetPhysicalDeviceProperties / vkGetPhysicalDeviceFeatures / vkGetPhysicalDeviceMemoryProperties
- 속성 / Features / 메모리 관련 정보 가져옴
- 프로젝트 내 VulkanDevice 객체에 디바이스 정보 저장

#### 1.3 Logical Device 
- 사용할 큐패밀리 설정 
- 필요한 Extension 설정
- VkDeviceCreateInfo 구조체 생성 및 초기화
- vkCreateDevice 로 디바이스 생성
- CommandPool 생성

#### 1.4 Depth / Stencil 설정
- 필요하다면 설정

#### 1.5 SwapChain 
- swapChain.connect(instance, physicalDevice, device);

#### 1.6 세마포어
- presentComplete / renderComplete 에 대한 세마포어 셋팅
- 큐 제출 submitInfo 셋팅

### 2. SetupWindow

### 3.  VulkanExampleBase::Prepare

#### 3.1 initSwapchain
- VkWin32SurfaceCreateInfoKHR ( Window 기준 ) 구조체 설정 / vkCreateWin32SurfaceKHR 로 surface 생성
- 컬러 포맷 / 컬러 스페이스 설정

#### 3.2 createCommandPool
- 커맨트풀 생성 

#### 3.3 setupSwapChain
- 스왑체인 설정 / 이미지 크기 및 컬러 포맷, presentMode 등
- 이미지뷰 생성

#### 3.4 	createCommandBuffers
- Draw 커맨드 버퍼 갯수 ( 스왑체인 이미지 갯수 )만큼 생성

#### 3.5 createSynchronizationPrimitives
- 버퍼의 갯수만큼 Fence 생성

#### 3.6 setupDepthStencil
- depth / stencil 용 이미지와 이미지뷰 생성

#### 3.7 ...



### 4. Texture.cpp 의 prepare

#### 4.1 loadTexture
- 스테이징 버퍼를 생성하고 할당 및 바인드 / 이미지 생성, 메모리 할당 및 바인드 
- 커맨드 버퍼 생성하고 이미지 레이아웃 변경을 위한 파이프라인 배리어 커맨드 기록
- 샘플러 필터 및 addressmode 설정 후 생성
- 이미지 뷰 생성

#### 4.2 generateQuad
- Quad 의 vertex 와 index 정보 셋팅
- 버퍼 생성, 메모리 할당, 바인드 
- Host 가 볼 수 있도록
	-  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  
- descriptor 정보 셋팅

#### 4.3 setupVertexDescriptions
- 버텍스 정보들이 어느 index 에 바인딩될 것인지 설정하는 버텍스 인풋 바인딩 생성
- 버텍스 포지션, 노말, 텍스쳐 uv 에 대한 버텍스 인풋 애트리뷰트 생성
```c++
typedef struct VkVertexInputAttributeDescription { 
	uint32_t 	location; 	// 특정 로케이션의 애트리뷰트에 데이터 전달할지 정의
	uint32_t 	binding; 	// 어떤 버텍스 버퍼로부터 데이터 읽을지 정의
	VkFormat 	format; 	// 애트리뷰트 포맷
	uint32_t 	offset; 
} VkVertexInputAttributeDescription;
```  
- 위의 값으로 버텍스 인풋 스테이지 구조체 VkPipelineVertexInputStateCreateInfo 셋팅

#### 4.4 prepareUniformBuffers
- 버텍스 쉐이더 유니폼 버퍼 생성하는 함수
- VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT 
- VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
- Uniform Buffer Object VS 사이즈만큼 버퍼 생성
```cpp
struct {
	glm::mat4 projection;
	glm::mat4 modelView;
	glm::vec4 viewPos;
	float lodBias = 0.0f;
} uboVS;
```

#### 4.4.1 updateUniformBuffers
- uboVS 정보 업데이트
- 맵핑된 uniformBufferVS.mapped 에 uboVS 카피


#### 4.5 setupDescriptorSetLayout
- 파이프라인이 어떤 리소스에 접근할지 정의
- 디스크립터 셋 레이아웃 바인딩 정의
	- 0 번 : 버텍스 쉐이더 - 유니폼 버퍼
	- 1번 : 프래그먼트 쉐이더 - 이미지 샘플러
	- VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER - 텍스쳐 X , 이미지와 샘플러만 존재
	- 샘플러의 설정이 동일하다면 여러 개의 샘플러를 만들필요 없이 한개의 샘플러로 여러 개의 샘플러를 읽음

- 생성하려는 디스크립터 셋 레이아웃 정의 및 생성
- 파이프라인 레이아웃 정의 및 생성

#### 4.6 setupDescriptorPool
- 설정한 디스크립터 셋을 Pool 에서 할당받기 위한 Pool 생성
- 디스크립터 풀 사이즈 설정 및 생성

#### 4.7 setupDescriptorSet
- 디스크립터 풀에서 디스크립터 셋 할당
- 디스크립터 셋을 정의 및 할당
- 디스크립터 타입(버퍼 및 이미지)에 따라 디스크립터 셋이 어떤 리소스를 가리킬지 정의
- vkUpdateDescriptorSets : 디스크립터 셋이 어떤 리소스를 가리킬지 정의한 정보를 디스크립터 셋에 업데이트하는 API

#### 4.8 preparePipelines
- 아래 스테이지들을 다 셋팅 후 파이프라인 생성
- InputAssemblyState 
	- VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST 로 삼각형 받도록 설정
- RasterizationState
	- 레스터화 관련 데이터 설정
- ColorBlendAttachmentState / ColorBlendState
	- 컬러 블랜드 설정
- DepthStencilState
	- 뎁스 스텐실 설정 
	- 뎁스 read/wirte - VK_TRUE 
- ViewportState
	- 뷰폿 관련 설정
- MultisampleState
	- 멀티 샘플링 1로 설정
- DynamicState
	- 동적으로 변경할 상태들이 존재할 시 설정
	- VK_DYNAMIC_STATE_VIEWPORT,  VK_DYNAMIC_STATE_SCISSOR

- 쉐이더 설정
- 파이프라인 생성
 
#### 4.9 buildCommandBuffers
- Draw 커맨드 버퍼 수만큼 
- vkBeginCommandBuffer / vkCmdBeginRenderPass
- vkCmdSetViewport / vkCmdSetScissor
- vkCmdBindDescriptorSets / vkCmdBindPipeline
- vkCmdBindVertexBuffers / vkCmdBindIndexBuffer / vkCmdDrawIndexed
- vkCmdEndRenderPass / vkEndCommandBuffer


### 5.1 render

#### 5.2 draw

- VulkanExampleBase::prepareFrame() : Swapchain 에서  사용가능한 이미지를 가져온다.
- 큐에 커맨드 버퍼를 제출한다.
- VulkanExampleBase::submitFrame() : 화면에 출력되는 이미지를 정의 및 출력
