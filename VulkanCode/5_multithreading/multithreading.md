# Vulkan Multithreading

## `Multithreading prepare()`

```c++
void prepare()
{
	VulkanExampleBase::prepare();
	// Create a fence for synchronization
	VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	vkCreateFence(device, &fenceCreateInfo, nullptr, &renderFence);
	loadAssets();
	setupPipelineLayout();
	preparePipelines();
	prepareMultiThreadedRenderer();
	updateMatrices();
	prepared = true;
}
```

### `VulkanExampleBase::prepare();`

이전과 동일하여 스킵

### `vkCreateFence`

### `loadAssets()`

loadFromFile 를 사용하여 필요한  ufo, star 오브젝트 로드

1. Vertex / Index 를 위한 staging buffer 생성
2. VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT 로  Vertex / Index buffer 생성
3. staging buffer -> Vertex / Index Buffer 카피
4.  staging buffer 해제
5. UBO / Image Sampler를 위한 셋팅
6. Descriptor pool 생성 -> Descriptor layout 생성 -> Descriptor set 할당 -> Descriptor set Update

 ### `setupPipelineLayout()` 
 
```c++
void setupPipelineLayout()
{
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(nullptr, 0);

	// Push constants for model matrices
	VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(ThreadPushConstantBlock), 0);

	// Push constant ranges are part of the pipeline layout
	pPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}
```
1. 파이프라인 레이아웃 생성
2. PushConstant 를 사용할 것이라고 설정
		- 커맨드 버퍼를 통해 쉐이더에 상수 값들을 전달
		- 동일한 오브젝트에 다른 상수를 셋팅 가능 / ufo 그릴 때 사용

### `preparePipelines()` 
```c++
void preparePipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationState;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleState;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.pDepthStencilState = &depthStencilState;
	pipelineCI.pDynamicState = &dynamicState;
	pipelineCI.stageCount = shaderStages.size();
	pipelineCI.pStages = shaderStages.data();
	pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::Color});

	// Object rendering pipeline
	shaderStages[0] = loadShader(getShadersPath() + "multithreading/phong.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(getShadersPath() + "multithreading/phong.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.phong));

	// Star sphere rendering pipeline
	rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
	depthStencilState.depthWriteEnable = VK_FALSE;
	shaderStages[0] = loadShader(getShadersPath() + "multithreading/starsphere.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = loadShader(getShadersPath() + "multithreading/starsphere.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.starsphere));
}
```

1. 파이프라인 2개를 생성
	- UFO 용 렌더링 파이프라인
	-  Star Sphere 용 렌더링 파이프라인 
2. 두 개의 파이프라인에서 필요한 기본적인 State Info 생성
	- pVertexInputState 에서는 position / normal / color 사용한다고 설정
3. 각 렌더링 파이프라인에 필요한 쉐이더 설정
4. 렌더링 파이프라인 생성


### `prepareMultiThreadedRenderer`

```c++
void prepareMultiThreadedRenderer()
{
	// Since this demo updates the command buffers on each frame
	// we don't use the per-framebuffer command buffers from the
	// base class, and create a single primary command buffer instead
	VkCommandBufferAllocateInfo cmdBufAllocateInfo =
		vks::initializers::commandBufferAllocateInfo(
			cmdPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1);
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &primaryCommandBuffer));

	// Create additional secondary CBs for background and ui
	cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &secondaryCommandBuffers.background));
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &secondaryCommandBuffers.ui));
```

1. `VulkanExampleBase::prepare();`에서 생성한 CommandPool 에서 CommandBuffer 할당
2. VkCommandBufferAllocateInfo 의 level 을 VK_COMMAND_BUFFER_LEVEL_PRIMARY 로 설정하여 primaryCommandBuffer 생성
3. VkCommandBufferAllocateInfo 의 level 을 VK_COMMAND_BUFFER_LEVEL_SECONDARY 로 설정하여 secondaryCommandBuffers.background / secondaryCommandBuffers.ui 생성

- VK_COMMAND_BUFFER_LEVEL_PRIMARY : 큐에 submit 되는 커맨드 버퍼 
- VK_COMMAND_BUFFER_LEVEL_SECONDARY  : primary 커맨드 버퍼에서 실행될 수 있는 버퍼, 직접 submit 불가능


```c++
struct ThreadPushConstantBlock {
	glm::mat4 mvp;
	glm::vec3 color;
};

struct ObjectData {
	glm::mat4 model;
	glm::vec3 pos;
	glm::vec3 rotation;
	float rotationDir;
	float rotationSpeed;
	float scale;
	float deltaT;
	float stateT = 0;
	bool visible = true;
};

struct ThreadData {
	VkCommandPool commandPool;
	// One command buffer per render object
	std::vector<VkCommandBuffer> commandBuffer;
	// One push constant block per render object
	std::vector<ThreadPushConstantBlock> pushConstBlock;
	// Per object information (position, rotation, etc.)
	std::vector<ObjectData> objectData;
};

VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
{
	...
	numThreads = std::thread::hardware_concurrency(); // 최대 concurrent threads 수
	threadPool.setThreadCount(numThreads);
	numObjectsPerThread = 512 / numThreads; // 512 : ufo 수
	...
}
```

### `prepareMultiThreadedRenderer` 계속
```c++
	threadData.resize(numThreads);

	float maxX = std::floor(std::sqrt(numThreads * numObjectsPerThread));
	uint32_t posX = 0;
	uint32_t posZ = 0;

	for (uint32_t i = 0; i < numThreads; i++) 
	{
		ThreadData *thread = &threadData[i];

		// Create one command pool for each thread
		VkCommandPoolCreateInfo cmdPoolInfo = vks::initializers::commandPoolCreateInfo();
		cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &thread->commandPool));

		// One secondary command buffer per object that is updated by this thread
		thread->commandBuffer.resize(numObjectsPerThread);
		// Generate secondary command buffers for each thread
		VkCommandBufferAllocateInfo secondaryCmdBufAllocateInfo =
			vks::initializers::commandBufferAllocateInfo(
				thread->commandPool,
				VK_COMMAND_BUFFER_LEVEL_SECONDARY,
				thread->commandBuffer.size());
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &secondaryCmdBufAllocateInfo, thread->commandBuffer.data()));

		thread->pushConstBlock.resize(numObjectsPerThread);
		thread->objectData.resize(numObjectsPerThread);

		for (uint32_t j = 0; j < numObjectsPerThread; j++)
		{
			float theta = 2.0f * float(M_PI) * rnd(1.0f);
			float phi = acos(1.0f - 2.0f * rnd(1.0f));
			thread->objectData[j].pos = glm::vec3(sin(phi) * cos(theta), 0.0f, cos(phi)) * 35.0f;

			thread->objectData[j].rotation = glm::vec3(0.0f, rnd(360.0f), 0.0f);
			thread->objectData[j].deltaT = rnd(1.0f);
			thread->objectData[j].rotationDir = (rnd(100.0f) < 50.0f) ? 1.0f : -1.0f;
			thread->objectData[j].rotationSpeed = (2.0f + rnd(4.0f)) * thread->objectData[j].rotationDir;
			thread->objectData[j].scale = 0.75f + rnd(0.5f);

			thread->pushConstBlock[j].color = glm::vec3(rnd(1.0f), rnd(1.0f), rnd(1.0f));
		}
	}
}
```
1. 각 thread 마다 commandPool 생성
2. numObjectsPerThread 갯수만큼 VK_COMMAND_BUFFER_LEVEL_SECONDARY 로 commandBuffer 할당 
3. numObjectsPerThread 갯수만큼 objectData 와 pushConstBlock 값들 설정


### `updateMatrices()`
```c++
void updateMatrices()
{
	matrices.projection = camera.matrices.perspective;
	matrices.view = camera.matrices.view;
	frustum.update(matrices.projection * matrices.view);
}
```
1. view, projection matrix 설정 및 frustum 업데이트



### `draw()`
```c++
void draw()
{
	// Wait for fence to signal that all command buffers are ready
	VkResult fenceRes;
	do {
		fenceRes = vkWaitForFences(device, 1, &renderFence, VK_TRUE, 100000000);
	} while (fenceRes == VK_TIMEOUT);
	VK_CHECK_RESULT(fenceRes);
	vkResetFences(device, 1, &renderFence);

	VulkanExampleBase::prepareFrame();

	updateCommandBuffers(frameBuffers[currentBuffer]);

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &primaryCommandBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, renderFence));

	VulkanExampleBase::submitFrame();
}
```

1. fence (`prepare()` 에서 생성해둔) 가 시그널 상태가 될 때까지 Wait
2. `vkResetFences`로 fence 를 언시그널 상태로 변경
3. `updateCommandBuffers()` 호출


### `updateCommandBuffers`
```c++
void updateCommandBuffers(VkFramebuffer frameBuffer)
{
	// Contains the list of secondary command buffers to be submitted
	std::vector<VkCommandBuffer> commandBuffers;

	VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

	VkClearValue clearValues[2];
	clearValues[0].color = defaultClearColor;
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = width;
	renderPassBeginInfo.renderArea.extent.height = height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = frameBuffer;

	// Set target frame buffer
	VK_CHECK_RESULT(vkBeginCommandBuffer(primaryCommandBuffer, &cmdBufInfo));

	// The primary command buffer does not contain any rendering commands
	// These are stored (and retrieved) from the secondary command buffers
	vkCmdBeginRenderPass(primaryCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFE

	// Inheritance info for the secondary command buffers
	VkCommandBufferInheritanceInfo inheritanceInfo = vks::initializers::commandBufferInheritanceInfo();
	inheritanceInfo.renderPass = renderPass;
	// Secondary command buffer also use the currently active framebuffer
	inheritanceInfo.framebuffer = frameBuffer;

	// Update secondary sene command buffers
	updateSecondaryCommandBuffers(inheritanceInfo);

```

1. comman buffer 기록을 위해 필요한 정보 설정 후 기록 시작
2.  secondary 커맨드 버퍼에 기록 되도록 설정후 렌더패스 시작
	- VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
3. secondary 커맨드 버퍼일 경우 VkCommandBufferInheritanceInfo 로 설정
	- secondary command buffer 가 실행될 프레임 버퍼를 현재 활성화된 프레임 버퍼로 설정하여 렌더링되도록 함


### `updateSecondaryCommandBuffers` 
```c++
void updateSecondaryCommandBuffers(VkCommandBufferInheritanceInfo inheritanceInfo)
{
	// Secondary command buffer for the sky sphere
	VkCommandBufferBeginInfo commandBufferBeginInfo = vks::initializers::commandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

	VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
	VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);

	/*
		Background
	*/

	VK_CHECK_RESULT(vkBeginCommandBuffer(secondaryCommandBuffers.background, &commandBufferBeginInfo));

	vkCmdSetViewport(secondaryCommandBuffers.background, 0, 1, &viewport);
	vkCmdSetScissor(secondaryCommandBuffers.background, 0, 1, &scissor);

	vkCmdBindPipeline(secondaryCommandBuffers.background, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.starsphere);

	glm::mat4 mvp = matrices.projection * matrices.view;
	mvp[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	mvp = glm::scale(mvp, glm::vec3(2.0f));

	vkCmdPushConstants(secondaryCommandBuffers.background, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvp), &mvp);

	models.starSphere.draw(secondaryCommandBuffers.background);
	
	VK_CHECK_RESULT(vkEndCommandBuffer(secondaryCommandBuffers.background));

	/*
		User interface
		With VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS, the primary command buffer's content has to be defined
		by secondary command buffers, which also applies to the UI overlay command buffer
	*/
	VK_CHECK_RESULT(vkBeginCommandBuffer(secondaryCommandBuffers.ui, &commandBufferBeginInfo));

	vkCmdSetViewport(secondaryCommandBuffers.ui, 0, 1, &viewport);
	vkCmdSetScissor(secondaryCommandBuffers.ui, 0, 1, &scissor);
	vkCmdBindPipeline(secondaryCommandBuffers.ui, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.starsphere);
	drawUI(secondaryCommandBuffers.ui);
	
	VK_CHECK_RESULT(vkEndCommandBuffer(secondaryCommandBuffers.ui));
}
```

1. 배경인 별을 렌더링할 secondaryCommandBuffers 기록 시작 
	- VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : secondary 커맨드 버퍼에 기록시 설정
2. 파이프라인 바인딩 
3. matrix 업데이트 후 vkCmdPushConstants 로 상수 값 넘기고 Draw 후 커맨드 기록 종료
4. UI 렌더링할 커맨드 버퍼도 비슷하게 기록


### `updateCommandBuffers` 계속
```c++
	if (displayStarSphere) {
		commandBuffers.push_back(secondaryCommandBuffers.background);
	}
	// Add a job to the thread's queue for each object to be rendered
	for (uint32_t t = 0; t < numThreads; t++)
	{
		for (uint32_t i = 0; i < numObjectsPerThread; i++)
		{
			threadPool.threads[t]->addJob([=] { threadRenderCode(t, i, inheritanceInfo); });
		}
	}
	threadPool.wait();
```
1. threadPool 의 thread 에 오브젝트를 그리기 위한 커맨드 버퍼를 기록할 `threadRenderCode`  job 추가
2. thread 들이 커맨드 버퍼 기록이 다 될 때까지 Wait

### `Thread`
```c++
class Thread
{
private:
	bool destroying = false;
	std::thread worker;
	std::queue<std::function<void()>> jobQueue;
	std::mutex queueMutex;
	std::condition_variable condition;

	// Loop through all remaining jobs
	void queueLoop() {
		while (true) {
			std::function<void()> job;
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				condition.wait(lock, [this] { return !jobQueue.empty() || destroying; });
				if (destroying){
					break;
				}
				job = jobQueue.front();
			}
			job();
			{
				std::lock_guard<std::mutex> lock(queueMutex);
				jobQueue.pop();
				condition.notify_one();
			}
		}
	}

public:
	Thread() {
		worker = std::thread(&Thread::queueLoop, this);
	}
	~Thread() {
		if (worker.joinable()) {
			wait();
			queueMutex.lock();
			destroying = true;
			condition.notify_one();
			queueMutex.unlock();
			worker.join();
		}
	}

	// Add a new job to the thread's queue
	void addJob(std::function<void()> function) {
		std::lock_guard<std::mutex> lock(queueMutex);
		jobQueue.push(std::move(function));
		condition.notify_one();
	}

	// Wait until all work items have been finished
	void wait() {
		std::unique_lock<std::mutex> lock(queueMutex);
		condition.wait(lock, [this]() { return jobQueue.empty(); });
	}
};

class ThreadPool
{
public:
	std::vector<std::unique_ptr<Thread>> threads;
	void setThreadCount(uint32_t count) {
		threads.clear();
		for (auto i = 0; i < count; i++) {
			threads.push_back(make_unique<Thread>());
		}
	}
	// Wait until all threads have finished their work items
	void wait() {
		for (auto &thread : threads) {
			thread->wait();
		}
	}
};
```
- setThreadCount 로 Thread 생성 후 queueLoop 실행 -> addJob 호출되어 notify_one 가 실행 -> queueLoop 의 condition.wait 풀림 -> job 실행 ->  jobQueue.pop / notify_one 실행 -> ....
- 	threadPool.wait() 호출하여 다 끝날 때까지 대기 


### `threadRenderCode`
```c++
// Builds the secondary command buffer for each thread
void threadRenderCode(uint32_t threadIndex, uint32_t cmdBufferIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
{
	ThreadData *thread = &threadData[threadIndex];
	ObjectData *objectData = &thread->objectData[cmdBufferIndex];

	// Check visibility against view frustum using a simple sphere check based on the radius of the mesh
	objectData->visible = frustum.checkSphere(objectData->pos, models.ufo.dimensions.radius * 0.5f);

	if (!objectData->visible){
		return;
	}

	VkCommandBufferBeginInfo commandBufferBeginInfo = vks::initializers::commandBufferBeginInfo();
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

	VkCommandBuffer cmdBuffer = thread->commandBuffer[cmdBufferIndex];

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo));

	VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.phong);

	// Update
	if (!paused) {
		objectData->rotation.y += 2.5f * objectData->rotationSpeed * frameTimer;
		if (objectData->rotation.y > 360.0f) {
			objectData->rotation.y -= 360.0f;
		}
		objectData->deltaT += 0.15f * frameTimer;
		if (objectData->deltaT > 1.0f)
			objectData->deltaT -= 1.0f;
		objectData->pos.y = sin(glm::radians(objectData->deltaT * 360.0f)) * 2.5f;
	}

	objectData->model = glm::translate(glm::mat4(1.0f), objectData->pos);
	objectData->model = glm::rotate(objectData->model, -sinf(glm::radians(objectData->deltaT * 360.0f)) * 0.25f, glm::vec3(objectData->rotationDir, 0.0f, 0.0f));
	objectData->model = glm::rotate(objectData->model, glm::radians(objectData->rotation.y), glm::vec3(0.0f, objectData->rotationDir, 0.0f));
	objectData->model = glm::rotate(objectData->model, glm::radians(objectData->deltaT * 360.0f), glm::vec3(0.0f, objectData->rotationDir, 0.0f));
	objectData->model = glm::scale(objectData->model, glm::vec3(objectData->scale));

	thread->pushConstBlock[cmdBufferIndex].mvp = matrices.projection * matrices.view * objectData->model;

	// Update shader push constant block
	// Contains model view matrix
	vkCmdPushConstants(
		cmdBuffer,
		pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(ThreadPushConstantBlock),
		&thread->pushConstBlock[cmdBufferIndex]);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &models.ufo.vertices.buffer, offsets);
	vkCmdBindIndexBuffer(cmdBuffer, models.ufo.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmdBuffer, models.ufo.indices.count, 1, 0, 0, 0);

	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
}
```

각 thread 들의 커맨드 기록하는 함수
1. frustum 안에 존재하는지 체크 후 VkCommandBufferBeginInfo 의 flags 를 아래 비트로 설정
	- VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : secondary 커맨드 버퍼에 기록시 설정
2. thread 의 커맨드 버퍼를 가져와 기록 시작 -> 뷰폿/시저 설정 -> phong 파이프라인 바인딩 -> mvp matrix 업데이트
3. vertex 와 index 바인딩 후 Draw -> 커맨드 기록 종료


### `updateCommandBuffers` 계속
```c++
	// Only submit if object is within the current view frustum
	for (uint32_t t = 0; t < numThreads; t++)
	{
		for (uint32_t i = 0; i < numObjectsPerThread; i++)
		{
			if (threadData[t].objectData[i].visible)
			{
				commandBuffers.push_back(threadData[t].commandBuffer[i]);
			}
		}
	}
	// Render ui last
	if (UIOverlay.visible) {
		commandBuffers.push_back(secondaryCommandBuffers.ui);
	}

	// Execute render commands from the secondary command buffer
	vkCmdExecuteCommands(primaryCommandBuffer, commandBuffers.size(), commandBuffers.data());

	vkCmdEndRenderPass(primaryCommandBuffer);

	VK_CHECK_RESULT(vkEndCommandBuffer(primaryCommandBuffer));
}
```
3. frustum 안에 존재하는 오브젝트의 커맨드 버퍼만 추가
4. vkCmdExecuteCommands 로 Star, ufo, ui 커맨드버퍼를 primaryCommandBuffer 에 추가
5. 렌더패스 종료 -> 커맨드 버퍼 기록 종료


### `draw()` 계속
```c++
void draw()
{
	// Wait for fence to signal that all command buffers are ready
	VkResult fenceRes;
	do {
		fenceRes = vkWaitForFences(device, 1, &renderFence, VK_TRUE, 100000000);
	} while (fenceRes == VK_TIMEOUT);
	VK_CHECK_RESULT(fenceRes);
	vkResetFences(device, 1, &renderFence);

	VulkanExampleBase::prepareFrame();

	updateCommandBuffers(frameBuffers[currentBuffer]);

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &primaryCommandBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, renderFence));

	VulkanExampleBase::submitFrame();
}
```

1. 큐에 submit 은 primaryCommandBuffer 만 제출
