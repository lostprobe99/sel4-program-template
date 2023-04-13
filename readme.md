# seL4程序模板

## 拉取子模块

```sh
git submodule init
git submodule update
```

## 如何使用

1. 将你的项目放到`projects`目录下
2. 将`project-CMakeLists.txt.template`拷贝到你的项目目录下并重命名为`CMakeLists.txt`
3. 将上一步中的`CMakeLists.txt`中的`set(project_name hello)`中的`hello`改为你项目的名称

## 编译运行

```sh
cmake -G Ninja -C./settings.cmake ./projects/${project_name} -B ${build_directory}
cd ${build_directory}
ninja
./simulate
```
