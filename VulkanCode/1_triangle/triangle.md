# Triangle Code 분석

## 1. initVulkan
### 1.1 Instance 생성
```c++
VulkanExampleBase::createInstance(bool enableValidation);
```
#### 1.1.1. applicationInfo 설정
```VkApplicationInfo``` 에 간단한 name 정보 담아서 구조체 생성

#### 1.1.2. Extension 설정
사용하고자 하는 벌칸 익스텐션 이름들의 배열인 ```instanceExtensions``` 에
VK_KHR_SURFACE + OS별 SURFACE_EXTENSION_NAME을 붙인다.

지원가능한 익스텐션 목록을 가져오고
내가 사용하고자 하는 익스텐션(SURFACE관련 2개) 와 맞지 않으면 에러를 뱉어준다.

디버그 유틸이 포함될 경우, 해당 익스텐션이름도 받아와 추가한다.

#### 1.1.3. Layer 설정

디버그 유틸이 포함될 경우 큐에 제출하기 전에 디버그 레이어를 타야 하므로, 레이어도 하나 추가해준다.

#### 1.1.4. Instance 생성
모아놓은 정보 applicationInfo, ExtensionInfos, LayersInfos 를 가지고 인스턴스를 만든다.
+) 생성 직후, 디버그 익스텐션 사용시, 이를 위한 디버그 유틸 매신저 CreationInfo(콜백) 을 달아준다.


### 1.2 Physical Device 설정
#### 1.2.1. 사용할 GPU 선정
별 일 없으면 0번 GPU를 사용한다. (내가 argv로 지정하지 않을 경우)

#### 1.2.2. physicalDevice 가져오기
vkEnumeratePhysicalDevices 를 통해 GPU목록을 가져오고, 사용하고자 하는 GPU의
```vkPhysicalDevice``` 를 저장해놓는다.

디바이스 Properties (속성)
```c++
typedef struct vkGetPhysicalDeviceProperties {
    uint32_t                            apiVersion;
    uint32_t                            driverVersion;
    uint32_t                            vendorID;
    uint32_t                            deviceID;
    VkPhysicalDeviceType                deviceType;
    char                                deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    uint8_t                             pipelineCacheUUID[VK_UUID_SIZE];
    VkPhysicalDeviceLimits              limits;
    VkPhysicalDeviceSparseProperties    sparseProperties;
} vkGetPhysicalDeviceProperties;
```

디바이스 Features. 내가 당장 이해하는 플래그는 아래 두개 뿐
```c++
typedef struct VkPhysicalDeviceFeatures {
    ...
    VkBool32    geometryShader;
    VkBool32    tessellationShader;
    ...
} VkPhysicalDeviceFeatures;

```

메모리 관련 정보.
```c++
typedef struct VkPhysicalDeviceMemoryProperties {
    uint32_t        memoryTypeCount;
    VkMemoryType    memoryTypes[VK_MAX_MEMORY_TYPES];
    uint32_t        memoryHeapCount;
    VkMemoryHeap    memoryHeaps[VK_MAX_MEMORY_HEAPS];
} VkPhysicalDeviceMemoryProperties;

typedef struct VkMemoryHeap {
    VkDeviceSize         size;
    VkMemoryHeapFlags    flags;
} VkMemoryHeap;

typedef uint64_t VkDeviceSize;
```
로 되어있으며, 디바이스 내의 메모리 정보를 알 수 있다.

#### 1.2.3. VulkanDevice 초기화
예제 코드 내에서 만들어 놓은 Device 추상화의 최종 버전.
```VulkanDevice``` 객체로 PhisicalDevice 및 Logical Device를 추상화하여 이 객체에 Device관련 데이터를 저장한다.

```c++
struct VulkanDevice
{
	VkPhysicalDevice physicalDevice; // Physical
	VkDevice logicalDevice; // Logical

    // Physical
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceFeatures enabledFeatures;
	VkPhysicalDeviceMemoryProperties memoryProperties;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	std::vector<std::string> supportedExtensions;

    // Logical
	VkCommandPool commandPool = VK_NULL_HANDLE;

	struct
	{
		uint32_t graphics;
		uint32_t compute;
		uint32_t transfer;
	} queueFamilyIndices;

    // Methods.. 디바이스 정보들을 가져오거나, pool 생성 및 커맨드 버퍼의 커맨드들 제출 등의 작업 진행
    VkResult createLogicalDevice(
        VkPhysicalDeviceFeatures enabledFeatures, // 본 예에선 empty
        std::vector<const char *> enabledExtensions, // 본 예에선 empty
        void *pNextChain,
        bool useSwapChain = true,
        VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT
    );
};
```

### 1.3 Logical Device 설정
#### 1.3.1. 큐패밀리 설정
이제 사용할 큐패밀리들을 설정해야 한다.
위 ```reateLogicalDevice``` 를 보면 팔수 파라미터로
* device의 Feature정보
* 사용할 수 있는 extensions
* pNextChain 정보

를 받고있음을 알 수 있다.
위 함수가 호출되면 각 큐 타입에 해당하는 큐패밀리 idx를 받아오도록 되어있고,
 이 정보들이 각각 queueCreateInfos 벡터에 저장되도록 되어있다.
 비트 설정을 안할 경우 **graphics** 와 동일한 큐패밀리 인덱스를 사용한다.


#### 1.3.2. Extensions 설정
enabledExtensions 라는 벡터 (이 예제에선 빈벡터) 에 추가로 swapchain extension을 추가한다.

#### 1.3.3. pNextChain
이 예제에선 사용하지 않는다.
사용하고자 하는 feature도 따로 정의가 되지 않는다.
device에서 우리가 긁어온 fearture말고, 추가적으로 익스텐션에 의해 정의된 feature들을 추가하고 싶다면 아래와 같이
 구조체 타입을 달아준다.

```c++
if (pNextChain) {
  physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  physicalDeviceFeatures2.features = enabledFeatures;
  physicalDeviceFeatures2.pNext = pNextChain;
  deviceCreateInfo.pEnabledFeatures = nullptr;
  deviceCreateInfo.pNext = &physicalDeviceFeatures2;
}
```

#### 1.3.4. deviceCreateInfo 생성 및 deviec생성
이제 실제 논리 device를 만든다.
* queueCreateInfos
* enabledFeatures
* deviceExtensions // swapchain 추가된 enabledExtensions

#### 1.3.5. graphics queue를 위한 commandPool도 만들어준다.
```c++
commandPool = createCommandPool(queueFamilyIndices.graphics);
```

### 1.4. 자잘한 설정들
#### 1.4.1. swapchain에 만든 객체 저장

swapChain.connect(instance, physicalDevice, device);
swapChain은 ```VulkanSwapChain``` 의 유저타입
이후 initSurface 시에 실제 동작이 시작된다.

#### 1.4.2. 세마포어 생성
```c++
VK_CHECK_RESULT(vkCreateSemaphore(device,
                                  &semaphoreCreateInfo,
                                  nullptr,
                                  &semaphores.presentComplete));

VK_CHECK_RESULT(vkCreateSemaphore(device,
                                  &semaphoreCreateInfo,
                                  nullptr,
                                  &semaphores.renderComplete));
```
로 세마포어 2개를 만든다.

```semaphores.presentComplete``` 와 ```semaphores.renderComplete``` 는 VkSemaphore

#### 1.4.3. submit Info 초기화
큐 제출에 쓰이는 VkSubmitInfo를 여기서 초기화 해준다.
present와 render에 대한 세마포어를 디바이스에서 대기해야한다는 걸 명시한다.


## 2. setWindow
윈도우 생성은 스킵한다. (OS별로 너무 다름)

## 3. prepare
렌더링에 쓰이는 자원들이 할당되는 곳이다.
```VulkanExampleBase::prepare()``` 와 application각각이 설정해놓은 ```prepare()``` 함수들을 호출한다.

```VulkanExampleBase::prepare()``` 이후에 동기화가 ```prepare()``` 가 수행된다.

### 3.1 VulkanExampleBase::prepare()
#### 3.1.1. swapchain 초기화
컬러 포맷 / 컬러스페이스 / device와 OS별 surface 등을 고려하여 swapchain을 생성한다.
세부 로직은 ```initSurface``` 함수를 보면 되는데, 앞에 Device부분 초기화 하는 (큐패밀리 관련) 부분이 많이 겹친다.

#### 3.1.2. Command pool 생성
커맨드 풀 생성. (위에도 만들긴 했어서 중복이다. 최종적으로 사용하는 Pool은 1개)

#### 3.1.3. setupSwapchain
```c++
swapChain.create(&width, &height, settings.vsync, settings.fullscreen);
```
위 함수를 호출하도록 되어있다.

```swapChain``` 는 추상화된 ```VulkanSwapChain``` 라는 예제에서 만든 구조체를 사용한다.

```c++
typedef struct _SwapChainBuffers {
	VkImage image;
	VkImageView view;
} SwapChainBuffer;

class VulkanSwapChain
{
private:
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
public:
	VkFormat colorFormat;
	VkColorSpaceKHR colorSpace;
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	uint32_t imageCount;
	std::vector<VkImage> images;
	std::vector<SwapChainBuffer> buffers; // 이미지 뷰 공간
	uint32_t queueNodeIndex = UINT32_MAX;

    void initSurface(void* platformHandle, void* platformWindow);
    void connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);
	void create(uint32_t* width, uint32_t* height, bool vsync = false, bool fullscreen = false);
	VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
	VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);
	void cleanup();
}
```
위와 같이 정의되어 있으며
```create``` 시
Physical Device + Surface 의 Capabilities를 가져온다. (width / height 등의 Extent, color정보 포함)
가져온 정보를 토대로 이미지 갯수, presentMode (메일박스 / FIFO) 등의 설정을 마친 뒤
스왑체인을 생성한다.

이후 이미지뷰를 만든다.
> 이미지 뷰?
>
> 실제 이미지를 뷰를 통해 한 번 더 필터링할 수 있다. (밉맵핑, 포맷팅 등)
>
> 이미지가 실제 디바이스 capability에 맞춰서 생성이 되었을 텐데,
>
> 여기에 내가 픽셀 수를 줄인다거나 하는 등의 작업이 가능한 것

생성된 이미지들과 이미지뷰를 연결하는 buffer라는 구조체를
VulkanSwapchain에 들고있는다.

#### 3.1.4. createCommandBuffers

```VulkanExampleBase->drawCmdBuffers``` 벡터는
 swapchain의 이미지 별 draw 커맨드버퍼이다.

따라서 ```VulkanExampleBase->cmdPool``` 을 통해 이미지 갯수만큼의 커맨드 버퍼를 생성한다.
1. info 정보를 cmdPool을 통해 만들고,
2. 해당 info를 통해 위 벡터의 ```data()``` 영역에 실제 할당을 받는다.


#### 3.1.5. createSynchronizationPrimitives

동기화 관련 (fence / semaphore) 정보들을 설정한다.

> fence ?
>
> 시그널 / 언시그널 상태가 있으며,
>
> 보통 초기에 시그널 상태로 두어 로직을 간편화 한다.

버퍼의 갯수만큼 waitFence들을 만든다.

이녀석들도 ```VulkanExampleBase->waitFences``` 에 저장되어있다.

#### 3.1.6. setupDepthStencil

뎁스 / 스텐실용 이미지 및 이미지뷰를 생성하는 곳이다.
이미지 생성 정보를 통해 실제 이미지를 만들고,

이번엔 실제 메모리도 allocate 받아 해당 메모리와 이미지를 우선 바인드 시킨다.

여기까지가 이미지 관련 정보를 준비한 것이고, 이전 swapchain 이미지들과 마찬가지로 이미지 뷰를 생성하여
 두 정보를 연결해 VulkanExampleBase 클래스 내 멤버변수에 저장해 놓는다.

```c++
struct {
   VkImage image;
   VkDeviceMemory mem;
   VkImageView view;
} depthStencil;
```

#### 3.1.7. setupRenderPass

> 랜더패스?
>
> attachment간 디펜던시를 두어 매번 비용이 큰 메모리 (프레임버퍼) 에 데이터를 적재하는 것이 아닌
>
> 타일 메모리와 같은 영역에 두어 처리하도록 함으로써 최적화를 이루어낼 수 있다.
>
> [타일메모리](https://3dmpengines.tistory.com/2045)
>
> 보통은 칩 내부에 타일 메모리를 위한 버퍼가 존재함 (비용 저렴)

어태치먼트 두 개를 만든다.

하나는 Color고, 하나는 Deptn이다.

loadOp에는 둘 다 ```VK_ATTACHMENT_LOAD_OP_CLEAR``` 를 사용하고
storeOp도 둘 다 ```VK_ATTACHMENT_STORE_OP_STORE``` 를 사용한다.

즉 두 어태치 먼트 (color / depth) 는 유니폼변수로 초기값을 로드할 것이고
타일메모리가 아닌 프레임버퍼 영역에 데이터를 저장하게 될 것임을 의미한다.

이후 ```VkSubpassDependency``` 를 생성하는데, 책에서 배운 ```VkImageMemoryBarrier``` 와 유사한 녀석이다.

subpass가 1개라 서브패스간 디펜던시는 없고,
 랜더패스가 시작할 때 이 디팬던시들이 걸린다.

뎁스/스텐실 테스트 전에 ```VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT``` 로 설정해주고
 설정 후에 READ|WRITE로 변경해준다.

```VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT``` 는 컬러 어태치먼트에 데이터를 붓기 직전의 상태이다.

해당 상태를 기점으로 ```VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_COLOR_ATTACHMENT_READ_BIT``` 를 주어 프레임버퍼의 R/W 권한을 준다.

이제 렌더패스 정보를 달아 렌더패스를 만들어준다.

#### 3.1.8. createPipelineCache
일단 스킵

#### 3.1.9. setupFrameBuffer
프레임버퍼를 생성한다.

뎁스관련 버퍼 1개, swapchain 이미지 갯수만큼 버퍼를 생성한다.

### 3.2 triangle의 prepare
#### 3.2.1 createSynchronizationPrimitives
```MAX_CONCURRENT_FRAMES``` 은 2로 설정되어있다.

각 프레임 당 세마포어 2개를 만든다.
```c++
std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> presentCompleteSemaphores;
std::array<VkSemaphore, MAX_CONCURRENT_FRAMES> renderCompleteSemaphores;
```

화면 출력 / 렌더링 두 경우에 대한 세마포어가 한 프레임에 걸리게 된다.

또한 해당 프레임에 대한 펜스도 만들어준다.

#### 3.2.2 createCommandBuffers
커맨드 풀 / 커맨드 버퍼를 만들어준다.

프레임을 2개 사용하므로, 버퍼는 풀을 통해 2개만 생성한다.

> 커맨드 풀 / 버퍼가 벌써 3번째 생성이 되고 있는데, 결국 device의 큐는 정해져 있고,
>
> 해당 큐에 어떤 커맨드 풀을 통해 밀어 넣을 지는 사용자 선택이기 때문에 큰 상관은 없는 것 같다.


#### 3.2.3 createVertexBuffer
```c++
auto location = glGetUniformLocation(program, "PVM");
glUniformMatrix4fv(location, 1, GL_FALSE, pvm);
```

원래는 위와 같이만 설정하면 디바이스/OpenGL 드라이버가 알아서
* 내부에서 유니폼 버퍼도 할당하고
* 값도 업데이트하고
* 파이프라인에 유니폼 버퍼를 바인딩

의 작업들을 수행해준다. 하지만 벌칸은 오픈 지엘 드라이버에서 대신 해주던 것들을 개발자가 직접 처리해야 한다.

우선 함수 내에선 버택스 버퍼/인덱스를 설정한다.

```c++
// Setup vertices
std::vector<Vertex> vertexBuffer{
	{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
	{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
};
uint32_t vertexBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(Vertex);

// Setup indices
std::vector<uint32_t> indexBuffer{ 0, 1, 2 };
indices.count = static_cast<uint32_t>(indexBuffer.size());
uint32_t indexBufferSize = indices.count * sizeof(uint32_t);
```

또한 아래와 같이 버퍼들을 추상화해준다.
```c++
struct StagingBuffer {
	VkDeviceMemory memory;
	VkBuffer buffer;
};

struct {
	StagingBuffer vertices;
	StagingBuffer indices;
} stagingBuffers;

// Vertex buffer and attributes
struct {
  VkDeviceMemory memory; // Handle to the device memory for this buffer
  VkBuffer buffer;       // Handle to the Vulkan buffer object that the memory is bound to
} vertices;

// Index buffer
struct {
  VkDeviceMemory memory;
  VkBuffer buffer;
  uint32_t count;
} indices;
```



순서는 아래와 같다.
* Host가 볼 수 있는 형태의 버퍼를 만든다.
  * VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
* 데이터들을 버퍼로 복사한다.
* device안에 또다른 버퍼공간을 만든다.
  * VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
* 위 버퍼 내용을 device안으로 복사한다.
* 기존 버퍼를 제거한다.

호스트가 볼 버퍼가 **Staging** 이란 이름을 붙여서 관리되고 있다.

Copy는 커맨드를 통해 제출된다.

펜스를 만들어서 해당 큐에서 커맨드가 처리될 때 까지 기다린다.


#### 3.2.4 createUniformBuffers
```c++
struct ShaderData {
  glm::mat4 projectionMatrix;
  glm::mat4 modelMatrix;
  glm::mat4 viewMatrix;
};
```
위 매트릭스 3개에 대한 정보는 Host Visible하게 관리가 되어야 하는 디바이스 메모리이다.
프레임 버퍼별로 존재해야한다. (파이프라인이 별도이므로 파이프라인 별로 유니폼 버퍼가 필요하다.)

각 버퍼는 **uniformBuffers[i].mapped** 위치에 매핑시켜놓는다.

#### 3.2.5 createDescriptorSetLayout
* 레이아웃 1개 (MVP매트릭스 공간)
* 레이아웃 당 바인딩되는 변수공간 1개 (MVP매트릭스 공간)

위 두 가지 정보를 설정하여 디스크립터셋 레이아웃을 만들어 놓는다.
해당 레이아웃은 실제 디스크립터 셋을 만들 때 사용된다.

> 디스크립터 셋은 커맨드 버퍼와 비슷하게 디스크립터풀에서 받아오도록 되어있다.
>
> 따라서 풀을 먼저 만든 뒤, 디스크립터 셋을 획독하는 구조이다.

파이프라인에 대해서도 레이아웃을 설정하게 되는데,
 사용하는 레이아웃이 단 1개 뿐이므로, 설정할 레이아웃도 1개라 그대로 들어간다.

#### 3.2.6 createDescriptorPool
풀의 maxSet은 프레임버퍼 갯수를 따라간다. (2개)

풀을 생성하여 ```descriptorPool``` 에 넣어둔다.

#### 3.2.7 createDescriptorSets
각 프레임 별로 디스크립터 풀에서 셋을 allocate 받아온다.

각 디스크립터 셋에 대해 버퍼(VkBuffer) 정보를 달아주어 디바이스에 업데이트 시켜놓는다.

이렇게 하면 호스트 메모리에 업데이트 시 버퍼가 갱신되고, 갱신된 정보는 파이프라인 및 쉐이더에서 사용된다.

#### 3.2.8 createPipelines

파이프라인 Information을 설정하는 함수.

* 레이아웃 설정
  * 앞선 디스크립터 레이아웃 설정
* VkPipelineInputAssemblyStateCreateInfo 설정
  * 삼각형 형태로 vertex를 받도록 assembly 구성
* VkPipelineRasterizationStateCreateInfo
  * 레스터화 관련 정보들 입력
* VkPipelineColorBlendAttachmentState & VkPipelineColorBlendStateCreateInfo
  * 컬러블랜딩 설정
  * 여기선 하지 않는다.
* VkPipelineViewportStateCreateInfo
  * 뷰폿 관련 설정.
  * VkDynamicState에서 더 설정이 되며, 여기선 뷰폿 갯수가 몇개인지만 설정한다.
* VkDynamicState
  * 뷰폿 / Scissor 관련 State를 1개씩 넣어준다.
  * 커맨드 버퍼를 통해
* VkPipelineDepthStencilStateCreateInfo
  * 뎁스 스텐실 관련 정보. 여기선 Depth 관련된 VK_TRUE로 설정된다.
* VkPipelineMultisampleStateCreateInfo
  * 멀티 샘플링 정보 (안티 애일리어싱에 쓰인다.)
* VkVertexInputBindingDescription
  * 버텍스 정보가 어떻게 들어오고, 어떻게 바인딩 될껀지 (stride 등) 정보를 저장해준다.
  * 코드 참조
* 쉐이더 설정
  * 코드 참조



### 4. render()
준비가 모두 끝났다면 render함수가 매 프레임 호출이 된다.

카메라 관련 정보는 **nextFrame()** 에서 갱신이 된다.

#### 4.1. Fence
현재 프레임의 Fence를 기다린다.

#### 4.2. acquire image Index
현재 프레임의 Image index를 가져온다.

#### 4.3 shaderData 설정
카메라 관련 정보들을 통해 mvp 관련 데이터를 유니폼 버퍼에 넘긴다.
```c++
memcpy(uniformBuffers[currentBuffer].mapped, &shaderData, sizeof(ShaderData));
```

앞 단계에서 메모리 상에 매핑되고 있는 위치를 지정해주었기 때문에 memcpy만 해주어도 알아서
 넘어가도록 되어있다.

#### 4.4 Fence 리셋
시그널된 Fence를 언시그널시켜준다.

#### 4.5 커맨드 버퍼 리셋
현재 버퍼의 커맨드 버퍼를 리셋시켜 비워버린다.

#### 4.6 커맨드 버퍼 만들기
커맨드 버퍼를 만든다.

#### 4.7 랜더패스정보 초기화
렌더패스 정보에는 초기화할 Uniform값이 포함된다. (load_op_clear)

clear할 어태치먼트는 color / depth 2개이다.

어떤 프레임버퍼의 어태치먼트들인지도 설정해준다.

그리고 랜더패스 begin 커맨드를 버퍼에 제출한다.

#### 4.8 뷰폿 설정 / Scissor 정보 커맨드 설정

뷰폿 설정 관련 커맨드를 보내고 Scissor 정보를 보낸다.
다만 본 코드는 뷰폿과 동일한 Rect2D가 가므로 큰 영향이 없어보인다.

#### 4.9 descriptorSet 바인딩 커맨드 설정
디스크립터 셋을 바인딩 해준다.
쉐이더는 이 디스크립터 셋을 통해 draw를 수행하게 된다.

#### 4.10 파이프라인 바인딩 커맨드 설정
바인딩할 파이프라인을 설정한다.

#### 4.11 Vertex 관련 커맨드 설정
버퍼 및 Index 정보를 담은 커맨드를 설정한다.

#### 4.12 렌더패스 종료 커맨드 설정 및 커맨드 버퍼 종료

```c++
vkCmdEndRenderPass(commandBuffers[currentBuffer]);
VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers[currentBuffer]));
```

#### 4.13 커맨드를 큐에 제출

 ```VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT``` 스테이지에서 present 세마포어는 기다리고 rendering 세마포어는 설정하도록 제출 정보를 설정하여 제출한다.

#### 4.14 Present

앞선 커맨드 버퍼가 끝날 경우 rendering 세마포어가 시그널 될 것이므로 rendernig 세마포어를 기다리는 Present 명령을 큐에 제출한다.