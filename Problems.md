## 注意这个初始化，必须要memset，或者是{}进行默认初始化，否则会发生意想不到的错误，比如connect一直卡住
sockaddr_in server_addr