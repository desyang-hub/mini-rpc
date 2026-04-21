## Mini-Rpc 项目说明

### 日志模块

RpcServer bind TestService.add
ENABLE_ASYNC_LOGGING=on


```bash
cd build
rm -rf *
cmake ..
cmake --build .
cmake --install . --prefix ./install_test
ls install_test/include/   # 应该能看到 nlohmann 目录和你的 minirpc 头文件
```


下一步计划：添加线程池支持