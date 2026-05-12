sudo apt-get install libgtest-dev cmake


# 更新包列表 Ubuntu
sudo apt update
# 安装 protoc 编译器 + C++ 开发库
sudo apt install -y protobuf-compiler libprotobuf-dev libprotoc-dev
# 验证安装
protoc --version


# # 启用 EPEL CentOS
# sudo yum install -y epel-release
# # 安装（注意：CentOS 7 的 protobuf 版本较旧，通常是 2.5.0）
# sudo yum install -y protobuf protobuf-devel