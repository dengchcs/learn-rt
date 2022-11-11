## 构建
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```
完成后`build`下应该有构建文件, 可以使用ninja/make/Visual Studio等工具构建可执行文件

## 运行
传入场景配置文件运行: `rt <scene-path> <image-dir>`, 其中路径是相对于可执行文件的, 例如
```bash
# 假设可执行文件位于build下
mkdir images  # 存放图片的文件夹
cd build
./rt.exe ../scene.final.toml ../images
```
注意场景配置文件中的纹理图片路径和模型文件路径都是相对于配置文件的, 不是相对可执行文件的

也可以在其他位置运行可执行文件, 注意文件路径即可
