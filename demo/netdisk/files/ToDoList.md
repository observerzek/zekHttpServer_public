# ToDoList

- [x] 重现2D Affordance的结果，框架（update），看结果值得改进
- [ ] 考虑multi-view和3D Affordance
- [ ] 同时考虑LLM模型，其他小框架快速训练的模型

# 2D复现总览

|         Methods         |         KLD$\downarrow$          |          SIM$\uparrow$           |          NSS$\uparrow$           |
| :---------------------: | :------------------------------: | :------------------------------: | :------------------------------: |
|      Cross-View-AG      |              1.787               |              0.285               |              0.829               |
| Cross-View-AG(复现结果) |              1.782               |              0.295               |              0.832               |
|         LOCATE          |              1.405               |              0.372               |              1.157               |
|    LOCATE(复现结果)     |               1423               |              0.369               |              1.135               |
|      AffordanceLLM      | <font color = blue> 1.463</font> | <font color = blue> 0.377</font> | <font color = blue> 1.070</font> |
| AffordanceLLM(复现结果) | <font color = red> 1.916</font>  | <font color = red> 0.327</font>  |  <font color = red> 1.3</font>   |



|    Methods    |         KLD$\downarrow$          |          SIM$\uparrow$           |          NSS$\uparrow$           |
| :-----------: | :------------------------------: | :------------------------------: | :------------------------------: |
| Cross-View-AG |              2.092               |              0.209               |              0.138               |
|    LOCATE     |              2.003               |              0.224               |              0.435               |
| AffordanceLLM | <font color = blue> 1.916</font> | <font color = blue> 0.327</font> | <font color = blue> 1.070</font> |



# 2025.10.21

以往通过利用图片来辅助点云解，存在一个问题，

就是当给定的图片与点云差距过大时，结果会不理想。

通过Gaussian Splatting技术可以生成多个视角下点云对应的2D图，

利用这个图片与大模型的输出结合在一起，辅助3D点云的解码过程。

后续如果希望得到HOI图片，可以考虑通过Diffusion模型来根据图片生成内容。

<img src="https://raw.githubusercontent.com/observers-zek/Images/main/image-202510212230.png" alt="image-20250422123713185" style="zoom:50%;" />







# 2025.9.25

**论文复现**：24-CVPR, Laso: Language-guided affordance segmentation on 3d object

这篇论文在3D Affordance Grouding任务上，做到了SOTA，各项实验结果远超以往的方法，

很多25年的论文都是基于这个方法来进行比较，因此它的复现比较有价值。

|         Methods          |         mIoU$\uparrow$          |          AUC$\uparrow$          |          SIM$\uparrow$           |         MAE$\downarrow$          |
| :----------------------: | :-----------------------------: | :-----------------------------: | :------------------------------: | :------------------------------: |
|  ReLA(论文里的对比方法)  |              15.2               |              78.9               |              0.532               |              0.118               |
| 3D-SPS(论文里的对比方法) |              11.4               |              76.2               |              0.433               |              0.138               |
|        PointRefer        | <font color = blue> 20.8</font> | <font color = blue> 87.3</font> | <font color = blue> 0.629</font> | <font color = blue> 0.093</font> |
|   PointRefer(复现结果)   | <font color = red> 20.5</font>  | <font color = red> 86.3</font>  | <font color = red> 0.618</font>  | <font color = red> 0.099</font>  |

|  Methods   |         mIoU$\uparrow$          |          AUC$\uparrow$          |          SIM$\uparrow$           |         MAE$\downarrow$          |
| :--------: | :-----------------------------: | :-----------------------------: | :------------------------------: | :------------------------------: |
|    IAG     |              20.41              |              84.63              |               0525               |              0.099               |
| PointRefer | <font color = blue> 20.8</font> | <font color = blue> 87.3</font> | <font color = blue> 0.629</font> | <font color = blue> 0.093</font> |





**后续计划**：

从2D Affordance Grounding任务转移到3D Affordance Grouding上。

复现更多的SOTA模型，总结如今3D任务的方法框架，基于SOTA模型进行改进。



# 2025.6.23

|              Methods              |         KLD$\downarrow$          |          SIM$\uparrow$           |          NSS$\uparrow$           |
| :-------------------------------: | :------------------------------: | :------------------------------: | :------------------------------: |
|           AffordanceLLM           | <font color = blue> 1.463</font> | <font color = blue> 0.377</font> | <font color = blue> 1.070</font> |
|      AffordanceLLM(复现结果)      | <font color = red> 1.916</font>  | <font color = red> 0.327</font>  |  <font color = red> 1.3</font>   |
| AffordanceLLM(修改数据的描述部分) |              1.825               |              0.332               |              1.213               |

按照下列改进思路修改数据集，实验结果比原复现结果要好。

但是还是没有达到论文的结果。

```json
#原始大模型输出：
"value": "You can lift the highlighted area. <seg_patch>"

#改进方向：
"value" : "物体物理结构的具体描述 + 人类如何与物体交互的内容 + 原始内容 + <seg_patch>"
```

下一步计划：

阅读更多相关论文，以及探索3D Affordancer任务的相关论文。





# **2025.5.19**

**论文复现**：AffordanceLLM: Grounding Affordance from Vision Language Models

**论文框架**：

![image-20250422123713185](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250422123713185.png)

**复现结果：**

与论文的实验结果差距过大，放弃复现。

但是引用论文提到的这种想法，并引入别的数据集**(原始数据集只有900张图片)**，尝试一下。

目前正在写实验代码。









# **2025.4.22**

补全了论文确实的预训练代码，预计花费2张卡4天的时间。

改进思路：

```json
#原始大模型输出：
"value": "You can lift the highlighted area. <seg_patch>"

#改进方向：
"value" : "物体物理结构的具体描述 + 人类如何与物体交互的内容 + 原始内容 + <seg_patch>"

扩充了文本，使模型输出更多细节信息，由于大模型是基于decoder only的结构，
那么在输出<seg_patch>时，该token对应模型输出的最后的embeding应该能捕获前文的信息，对后续的图片解码过程有帮助。
```





# **2025.3.27**

**AffordanceLLM**复现结果不理想，在排查代码

|         Methods         |         KLD$\downarrow$          |          SIM$\uparrow$           |          NSS$\uparrow$           |
| :---------------------: | :------------------------------: | :------------------------------: | :------------------------------: |
|      Cross-View-AG      |              1.787               |              0.285               |              0.829               |
| Cross-View-AG(复现结果) |               1782               |              0.295               |              0.832               |
|         LOCATE          |              1.405               |              0.372               |              1.157               |
|    LOCATE(复现结果)     |               1423               |              0.369               |              1.135               |
|      AffordanceLLM      | <font color = blue> 1.463</font> | <font color = blue> 0.377</font> | <font color = blue> 1.070</font> |
| AffordanceLLM(复现结果) | <font color = red> 1.958</font>  | <font color = red> 0.331</font>  | <font color = red> 1.234</font>  |



# 2025.3.13

**论文阅读**：LISA: Reasoning Segmentation via Large Language Model

近年来使用LLM[^1][^2][^3]来实现**2D**或者**3D** Affordance Grounding的，都是基于该论文的框架。

使用该框架生成特殊token的方法，用于提取Affordance特征，再输入到图片Decoder里，实现Affordance Grounding。

## 模型框架

![image-20250306224346152](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250306224346152.png)

输入promp和image到多模态LLM里，通过对promp的设置，诱导LLM生成\<SEG\>特殊token。

当LLM输出\<SEG>token时，获取LLM里，最后一个transformer block的输出embeding，通过一个MLP，映射成$h_{seg}$。

image也会输入到Vision Backbone里，提取特征得到$F_{enc}$。

最后将$h_{seg}$，$F_{enc}$输入到图像Decoder里，得到最后符合指令要求的的Segmentation Mask。



## 数据集结构

![image-20250313220218486](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250313220218486.png)

上图是数据集的一个例子。

通过对Question的特殊设置，以及在Answer里添加\<SEG>特殊token，诱导LLM在特殊的prompt下，能够生成\<SEG>token。

例子右侧的Ground Truth，则是与模型生成的Segmentation Mask进行对比，计算loss。



# 复现过的框架

## Cross-view-AG

论文：Learning Affordance Grounding from Exocentric Images

模型框架：

![image-20250306220943696](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250306220943696.png)



实验结果 VS 复现结果：

![image-20250306221809014](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250306221809014.png)

![image-20250306222013268](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250306222013268.png)

## LOCATE

论文：LOCATE: Localize and Transfer  Object Parts for Weakly Supervised Affordance Grounding

模型框架：

![image-20250306220534547](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250306220534547.png)

实验结果 VS 复现结果：

![image-20250306222251456](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250306222251456.png)

![image-20250306223034133](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250306223034133.png)



## AffordanceLLM

**论文复现**：AffordanceLLM: Grounding Affordance from Vision Language Models

**论文框架**：

![image-20250422123713185](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250422123713185.png)

实验结果 VS 复现结果：

复现结果与实验结果差距过大，放弃复现。

![image-20250519160850209](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250519160850209.png)

![image-20250519161121851](https://raw.githubusercontent.com/observers-zek/Images/main/image-20250519161121851.png)


---

[^1]: 2024-4-17 arXiv <AffordanceLLM: Grounding Affordance from Vision Language Models>
[^2]: 2024-11-19 arXiv <GLOVER: Generalizable Open-Vocabulary Affordance Reasoning  for Task-Oriented Grasping>
[^3]: 2025-3-4 ICRL <3D-AffordanceLLM: Harnessing Large Language Models for Open-Vocabulary Affordance Detection in 3D Worlds>