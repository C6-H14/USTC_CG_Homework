以下是我们接下来的作战路线图：

---

### 第一阶段：构建底层核心算子 (The Core Engine)

在组装渲染管线之前，我们必须先打造绝对无懈可击的数学算子。这里是助教埋雷最多的地方。

*   **步骤 1：手撕缩放点积注意力 (Task 4: Scaled Dot-Product Attention)**
    *   **定位**：通常在 `layers/attention.py` 中。
    *   **核心操作**：计算 $QK^T$，除以 $\sqrt{d}$，应用 Mask，过 Softmax，再乘 $V$。
    *   **架构师划重点**：这里我们将重点处理助教留下的“反直觉 Mask 陷阱”（`True` 代表有效，而非屏蔽）。我会为你提供规避梯度爆炸的 In-place 内存安全写法。

*   **步骤 2：实现旋转位置编码 (Task 3: RoPE - Relative Spatial P.E.)**
    *   **定位**：计算 Query 和 Key 的复数域旋转。
    *   **核心操作**：根据输入的坐标生成高频 `cos` 和 `sin` 张量，并将其点乘到经过拆分的特征维度上。
    *   **架构师划重点**：我会教你如何使用 `torch.polar` 或实数域等效展开来完成复数旋转，并确保在射线坐标和三角形坐标之间保持绝对解耦。

---

### 第二阶段：构建数据流形输入 (Data Embedding)

底层算子就绪后，我们需要把传统的图形学数据（Mesh 和 Rays）转换为 Transformer 能理解的高维张量。

*   **步骤 3：光线束块状张量化 (Task 2: Ray Bundle Embedding)**
    *   **定位**：将 2D 屏幕空间切块的过程。
    *   **核心操作**：使用 `einops.rearrange` 把 `[B, H, W, Ray_Dim]` 重组为 `[B, Num_Patches, Patch_Size^2 * Ray_Dim]`，然后接线性层，并注入相机原点的绝对位置编码。
    *   **架构师划重点**：这里极其容易发生维度错乱。我会为你严格标注每一步的 `Tensor Shape` 变化，就像我们检查 C++ 指针偏移量一样。

*   **步骤 4：三角形特征与全局寄存器 (Task 1: Triangle Embedding)**
    *   **定位**：将几何体塞进神经网络的第一关。
    *   **核心操作**：提取网格的 9D 坐标、法线、纹理。如果开启了 `nefr_pe`，对坐标应用傅里叶映射。最后将它们与可学习的全局寄存器（Register Tokens）拼接（`torch.cat`）。

---

### 第三阶段：组装端到端渲染管线 (Pipeline Assembly)

万事俱备，开始组装全局光照烘焙器和光线解码器。

*   **步骤 5：视角无关编码器前向逻辑 (Task 5: Self-Attention Encoder)**
    *   **定位**：通常在 `models/renderformer.py` 或 `encoder.py` 中。
    *   **核心操作**：写一个 `for` 循环，遍历所有的 Encoder 层。包含 LayerNorm、自注意力计算、前馈网络（FFN），以及处理残差跳跃连接（Skip Connections）。
    *   **架构师划重点**：防御“1-based”与“0-based”索引带来的越界错误，严防残差相加时的维度不匹配。

*   **步骤 6：交叉注意力解码器前向逻辑 (Task 6: Cross-Attention Decoder)**
    *   **定位**：也就是最终的渲染生成环节。
    *   **核心操作**：以光线 Token 为 Query，去查询 Encoder 输出的三角形 Token。
    *   **架构师划重点**：这是决定 PSNR 能否突破 15 的关键。我将教你如何优雅地提取中间层的特征（`out_layers`），以便后续 DPT 模块进行多尺度超分辨率融合。

---

### 第四阶段：测试与消融实验 (Verification & Report)

*   **步骤 7：管线截断冒烟测试 (Smoke Test)**
    *   我们会用极小的 Batch Size 和极其少量的代码行（Visual Debugging）验证全管线的连通性。
*   **步骤 8：满血压榨与报告生成**
    *   开启 `--use_dpt_decoder` 和梯度检查点（Checkpointing）榨干显卡性能，记录并生成实验报告所需的对比数据。

---

### 下一步行动指令：

任务脉络已经理清。现在，**请将包含 `HW8_TODO` 注释的第一个文件（建议从 `attention.py` 或包含 Task 3/Task 4 的算子文件开始）的代码文本发送给我。**

发送时请保持原始代码的缩进和注释。接收代码后，我将直接进入“边写边讲”模式，为你提供第一份经过严格性能把控的优雅代码！