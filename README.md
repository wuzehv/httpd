# httpd
本项目主要用来学习与实践web server功能

### 特性
* epoll
* 解析js、css、html、图片等静态资源
* 解析php

### usage
实验版本：gcc version 7.4.0

```bash
git clone https://github.com/wuzehv/httpd.git

cd httpd
make
./httpd port

echo web_root_path > ./httpd.conf

# 解析php
# start php-fpm in 9000 port

# http://ip:port/index.html
# http://ip:port/index.php
```
